#include "fs.h"

#include <iostream>

#include "dir.h"
#include "file.h"
#include "inode.h"
#include "list.h"
#include "super_block.h"
#include "tools.h"
#define SUCCESS 1
#define FAILED 0

current_partition* cur_part;
int write_blocks_to_disk(virtual_disk* disk, uint32_t lba, char* data,
                         size_t bc) {
  std::fstream fs(disk->disk_name,
                  std::ios::binary | std::ios::out | std::ios::in);
  if (fs) {
    // std::cout << std::hex;
    // std::cout << "======" << (lba * BLOCK_SIZE) << std::endl;
    // std::cout << std::dec;
    fs.seekp(lba * BLOCK_SIZE, std::ios::beg);
    fs.write(data, bc * BLOCK_SIZE);
    fs.close();
    return SUCCESS;
  } else {
    return FAILED;
  }
}

char* read_blocks_from_disk(virtual_disk* disk, uint32_t lba, size_t bc) {
  std::fstream fs(disk->disk_name,
                  std::ios::binary | std::ios::out | std::ios::in);
  if (fs) {
    // std::cout << std::hex;
    // std::cout << "======" << (lba * BLOCK_SIZE) << std::endl;
    // std::cout << std::dec;
    fs.seekp(lba * BLOCK_SIZE, std::ios::beg);
    char* data = (char*)malloc(bc * BLOCK_SIZE);
    fs.read(data, bc * BLOCK_SIZE);
    fs.close();
    return data;
  } else {
    return (char*)(0);
  }
}

int read_blocks_from_disk_unsafe(virtual_disk* disk, uint32_t lba, char* data,
                                 size_t bc) {
  char* tmp = read_blocks_from_disk(disk, lba, bc);
  if (tmp != (char*)(0)) {
    memcpy(data, tmp, bc * BLOCK_SIZE);
    free(tmp);
    return SUCCESS;
  } else {
    return FAILED;
  }
}

// 逻辑块地址是指第几块 类似扇区号
// lbc -> Logic Block Count
// lba -> Logic Block Address
int partition_format(virtual_disk* v_disk, int partition_no) {
  /* 获取分区起始、结束块地址 */
  disk_partition dp = v_disk->partitions[partition_no];
  uint32_t start_lba = dp.start_sector_no;
  uint32_t logic_lbc = dp.sector_count * (BLOCK_SIZE / SECTOR_SIZE);
  uint32_t end_lba = start_lba + logic_lbc - 1;

  /* 根据分区大小，计算各元信息需要的扇区数及位置 */
  uint32_t boot_sector_lbc = 1;  // 启动块 占位
  uint32_t super_block_lbc = 1;  // 超级块
  // inode 位图占用块数
  uint32_t inode_bitmap_lbc = DIV_ROUND_UP(
      MAX_FILES_PER_PART, (BITS_PER_SECTOR * SECTOR_SIZE / BLOCK_SIZE));
  // inode 数组占用块数
  uint32_t inode_table_lbc =
      DIV_ROUND_UP(sizeof(inode) * MAX_FILES_PER_PART, BLOCK_SIZE);
  // 已使用的块数
  uint32_t used_lbc =
      boot_sector_lbc + super_block_lbc + inode_bitmap_lbc + inode_table_lbc;
  // 空闲块数 空闲块位图占用的块数
  uint32_t free_lbc = logic_lbc - used_lbc;
  uint32_t block_bitmap_lbc =
      DIV_ROUND_UP(free_lbc, BITS_PER_SECTOR * SECTOR_SIZE / BLOCK_SIZE);
  free_lbc -= block_bitmap_lbc;
  block_bitmap_lbc =
      DIV_ROUND_UP(free_lbc, BITS_PER_SECTOR * SECTOR_SIZE / BLOCK_SIZE);

  /* 在内存中创建超级块，并将元信息写入超级块 */
  super_block sb;
  sb.magic = 0x19590318;
  sb.block_count = logic_lbc;
  sb.inode_count = MAX_FILES_PER_PART;
  sb.start_lba = start_lba;
  // 引导块 超级块 空闲位块位图 inode位图 inode数组 根目录 空闲块区域
  sb.block_bitmap_lba = start_lba + 2;
  sb.block_bitmap_lbc = block_bitmap_lbc;

  sb.inode_bitmap_lba = sb.block_bitmap_lba + sb.block_bitmap_lbc;
  sb.inode_bitmap_lbc = inode_bitmap_lbc;

  sb.inode_table_lba = sb.inode_bitmap_lba + sb.inode_bitmap_lbc;
  sb.inode_table_lbc = inode_table_lbc;

  sb.data_start_lba = sb.inode_table_lba + sb.inode_table_lbc;
  sb.root_inode_no = 0;
  sb.dir_entry_size = sizeof(dir_entry);

  std::cout << "partition info:\n"
            << "name: \t\t\t" << dp.partition_name << "\n"
            << "magic: \t\t\t" << sb.magic << std::endl
            << "block count: \t\t" << sb.block_count << std::endl
            << "inode count: \t\t" << sb.inode_count << std::endl
            << "start lba: \t\t" << sb.start_lba << std::endl
            << "block bitmap lba: \t" << sb.block_bitmap_lba << std::endl
            << "block bitmap lbc: \t" << sb.block_bitmap_lbc << std::endl
            << "inode bitmap lba: \t" << sb.inode_bitmap_lba << std::endl
            << "inode bitmap lbc: \t" << sb.inode_bitmap_lbc << std::endl
            << "inode table lba: \t" << sb.inode_table_lba << std::endl
            << "inode table lbc: \t" << sb.inode_table_lbc << std::endl
            << "data start lba: \t" << sb.data_start_lba << std::endl
            << "root inode no: \t\t" << sb.root_inode_no << std::endl
            << "dir entry size: \t" << sb.dir_entry_size << std::endl;

  /* 1 将超级块写入磁盘 */
  // 写入 第1块
  if (write_blocks_to_disk(v_disk, start_lba + 1, (char*)&sb, 1)) {
    std::cout << "Super block has been written to disk!" << std::endl;
  } else {
    std::cout << "Super block failed to write to disk!" << std::endl;
    return FAILED;
  }

  // 设置写入磁盘的缓存
  uint32_t buf_size = sb.block_bitmap_lbc > sb.inode_bitmap_lbc
                          ? sb.block_bitmap_lbc
                          : sb.inode_bitmap_lbc;
  buf_size = (buf_size > sb.inode_table_lbc ? buf_size : sb.inode_table_lbc) *
             BLOCK_SIZE;
  uint8_t* buf = (uint8_t*)malloc(buf_size);

  /* 2 将块位图写入磁盘*/
  buf[0] |= 0x01;  // 将第0块预留给根目录
  // 位图所在块
  uint32_t block_bitmap_last_byte = free_lbc / 8;
  uint8_t block_bitmap_last_bit = free_lbc % 8;
  // last_size 是位图最后一个块中，不足一块的其余部分
  uint32_t last_size = BLOCK_SIZE - (block_bitmap_last_byte % BLOCK_SIZE);
  // 最后 last_size 位置 1
  memset(&buf[last_size], 0xff, last_size);
  // 其余位置 0
  uint8_t bit_idx;
  while (bit_idx <= block_bitmap_last_bit) {
    buf[block_bitmap_last_byte] &= ~(1 << bit_idx++);
  }
  if (write_blocks_to_disk(v_disk, sb.block_bitmap_lba, (char*)buf,
                           sb.block_bitmap_lbc)) {
    std::cout << "Block bitmap has been written to disk!" << std::endl;
  } else {
    std::cout << "Block bitmap failed to write to disk!" << std::endl;
    return FAILED;
  }

  /* 3 将 inode 位图写入磁盘 */
  memset(buf, 0, buf_size);  // 清空缓存区域
  buf[0] |= 0x1;             // 第 0 个 inode 分给根目录
  // 4096 个 inode 正好一个块
  if (write_blocks_to_disk(v_disk, sb.inode_bitmap_lba, (char*)buf,
                           sb.inode_bitmap_lbc)) {
    std::cout << "Inode bitmap has been written to disk!" << std::endl;
  } else {
    std::cout << "Inode bitmap failed to write to disk!" << std::endl;
    return FAILED;
  }

  /* 4 将 inode 数组写入磁盘 */
  memset(buf, 0, buf_size);
  inode* i = (inode*)buf;
  // 根目录
  i->i_size = sb.dir_entry_size * 2;  // 根目录只有 . 和 ..
  i->i_no = 0;
  i->i_block[0] = sb.data_start_lba;
  if (write_blocks_to_disk(v_disk, sb.inode_table_lba, (char*)buf,
                           sb.inode_table_lbc)) {
    std::cout << "Inode table has been written to disk!" << std::endl;
  } else {
    std::cout << "Inode table failed to write to disk!" << std::endl;
    return FAILED;
  }

  /* 5 将根目录写入磁盘 */
  memset(buf, 0, buf_size);
  dir_entry* de_ptr = (dir_entry*)buf;

  // 当前目录 .
  memcpy(de_ptr->filename, ".", 1);
  de_ptr->i_no = 0;
  de_ptr->f_type = FT_DIRECTORY;
  de_ptr++;

  // 当前目录的父目录 ..
  memcpy(de_ptr->filename, "..", 2);
  de_ptr->i_no = 0;
  de_ptr->f_type = FT_DIRECTORY;
  if (write_blocks_to_disk(v_disk, sb.data_start_lba, (char*)buf, 1)) {
    std::cout << "Dir Root has been written to disk!" << std::endl;
  } else {
    std::cout << "Dir Root failed to write to disk!" << std::endl;
    return FAILED;
  }
  free(buf);
  return SUCCESS;
}

int mount_partition(virtual_disk* v_disk, int partition_no) {
  current_partition* cp = cur_part =
      (current_partition*)malloc(sizeof(current_partition));
  cp->v_disk = v_disk;
  disk_partition dp = v_disk->partitions[partition_no];
  cp->partition_no = partition_no;
  char* buf = (char*)read_blocks_from_disk(v_disk, dp.start_sector_no + 1, 1);
  cp->sb = (super_block*)malloc(sizeof(super_block));
  memcpy(cp->sb, buf, sizeof(super_block));
  free(buf);
  cp->block_bitmap.btmp_bytes_len = cp->sb->block_bitmap_lbc * BLOCK_SIZE;
  cp->block_bitmap.bits = (uint8_t*)read_blocks_from_disk(
      v_disk, cp->sb->block_bitmap_lba, cp->sb->block_bitmap_lbc);
  cp->inode_bitmap.btmp_bytes_len = cp->sb->inode_bitmap_lbc * BLOCK_SIZE;
  cp->inode_bitmap.bits = (uint8_t*)read_blocks_from_disk(
      v_disk, cp->sb->inode_bitmap_lba, cp->sb->inode_bitmap_lbc);
  list_init(&cp->open_inodes);
  std::cout << dp.partition_name << " mounted!" << std::endl;
  return SUCCESS;
}

void fs_init(virtual_disk* v_disk, int default_partition_no) {
  std::cout << "Searching filesystem on " << v_disk->disk_name << std::endl;
  for (int i = 0; i < v_disk->current_partition_count; i++) {
    super_block* dp = (super_block*)read_blocks_from_disk(
        v_disk, v_disk->partitions[i].start_sector_no + 1, 1);
    if (dp->magic == 0x19590318) {
      std::cout << v_disk->partitions[i].partition_name << " has filesystem!"
                << std::endl;
    } else {
      std::cout << v_disk->partitions[i].partition_name
                << " has not filesystem!" << std::endl;
      std::cout << "Initialing..." << std::endl;
      partition_format(v_disk, i);
    }
    free(dp);
  }
  mount_partition(v_disk, default_partition_no);
  open_root_dir(cur_part);
  uint32_t fd_idx = 0;
  while (fd_idx < MAX_FILE_OPEN) {
    file_table[fd_idx++].fd_inode = NULL;
  }
}

/* 将最上层路径名称解析出来 */
static char* path_parse(char* pathname, char* name_store) {
  if (pathname[0] == '/') {  // 根目录不需要单独解析
    /* 路径中出现1个或多个连续的字符'/',将这些'/'跳过,如"///a/b" */
    while (*(++pathname) == '/')
      ;
  }

  /* 开始一般的路径解析 */
  while (*pathname != '/' && *pathname != 0) {
    *name_store++ = *pathname++;
  }

  if (pathname[0] == 0) {  // 若路径字符串为空则返回NULL
    return NULL;
  }
  return pathname;
}

/* 返回路径深度,比如/a/b/c,深度为3 */
int32_t path_depth_cnt(char* pathname) {
  if (!(pathname != NULL)) return -1;
  char* p = pathname;
  char name[MAX_FILENAME_LEN];  // 用于path_parse的参数做路径解析
  uint32_t depth = 0;

  /* 解析路径,从中拆分出各级名称 */
  p = path_parse(p, name);
  while (name[0]) {
    depth++;
    memset(name, 0, MAX_FILENAME_LEN);
    if (p) {  // 如果p不等于NULL,继续分析路径
      p = path_parse(p, name);
    }
  }
  return depth;
}

/* 搜索文件pathname,若找到则返回其inode号,否则返回-1 */
static int search_file(const char* pathname,
                       struct path_search_record* searched_record) {
  /* 如果待查找的是根目录,为避免下面无用的查找,直接返回已知根目录信息 */
  if (!strcmp(pathname, "/") || !strcmp(pathname, "/.") ||
      !strcmp(pathname, "/..")) {
    searched_record->parent_dir = &root_dir;
    searched_record->f_type = FT_DIRECTORY;
    searched_record->searched_path[0] = 0;  // 搜索路径置空
    return 0;
  }

  uint32_t path_len = strlen(pathname);
  /* 保证pathname至少是这样的路径/x且小于最大长度 */
  if (!(pathname[0] == '/' && path_len > 1 && path_len < MAX_PATH_LEN)) {
    std::cout << "error kind of the pathname" << std::endl;
    return -1;
  }
  char* sub_path = (char*)pathname;
  struct dir* parent_dir = &root_dir;
  struct dir_entry dir_e;

  /* 记录路径解析出来的各级名称,如路径"/a/b/c",
   * 数组name每次的值分别是"a","b","c" */
  char name[MAX_FILENAME_LEN] = {0};

  searched_record->parent_dir = parent_dir;
  searched_record->f_type = FT_UNKNOWN;
  uint32_t parent_inode_no = 0;  // 父目录的inode号

  sub_path = path_parse(sub_path, name);
  while (name[0]) {  // 若第一个字符就是结束符,结束循环
    /* 记录查找过的路径,但不能超过searched_path的长度512字节 */
    if (!(strlen(searched_record->searched_path) < 512)) {
      std::cout << "Searched path is more than 512 Bytes!" << std::endl;
      return -1;
    }

    /* 记录已存在的父目录 */
    strcat(searched_record->searched_path, "/");
    strcat(searched_record->searched_path, name);

    /* 在所给的目录中查找文件 */
    if (search_dir_entry(cur_part, parent_dir, name, &dir_e)) {
      memset(name, 0, MAX_FILENAME_LEN);
      /* 若sub_path不等于NULL,也就是未结束时继续拆分路径 */
      if (sub_path) {
        sub_path = path_parse(sub_path, name);
      }

      if (FT_DIRECTORY == dir_e.f_type) {  // 如果被打开的是目录
        parent_inode_no = parent_dir->inode->i_no;
        dir_close(parent_dir);
        parent_dir = dir_open(cur_part, dir_e.i_no);  // 更新父目录
        searched_record->parent_dir = parent_dir;
        continue;
      } else if (FT_REGULAR == dir_e.f_type) {  // 若是普通文件
        searched_record->f_type = FT_REGULAR;
        return dir_e.i_no;
      }
    } else {  //若找不到,则返回-1
      /* 找不到目录项时,要留着parent_dir不要关闭,
       * 若是创建新文件的话需要在parent_dir中创建 */
      return -1;
    }
  }

  /* 执行到此,必然是遍历了完整路径并且查找的文件或目录只有同名目录存在 */
  dir_close(searched_record->parent_dir);

  /* 保存被查找目录的直接父目录 */
  searched_record->parent_dir = dir_open(cur_part, parent_inode_no);
  searched_record->f_type = FT_DIRECTORY;
  return dir_e.i_no;
}

/* 打开或创建文件成功后,返回文件描述符,否则返回-1 */
int32_t sys_open(const char* pathname, uint8_t flags) {
  /* 对目录要用dir_open,这里只有open文件 */
  if (pathname[strlen(pathname) - 1] == '/') {
    std::cout << "can`t open a directory pathname\n";
    return -1;
  }
  if (!(flags <= 7)) {
    std::cout << "flag > 7!" << std::endl;
    return -1;
  }
  int32_t fd = -1;  // 默认为找不到

  struct path_search_record searched_record;
  memset(&searched_record, 0, sizeof(struct path_search_record));

  /* 记录目录深度.帮助判断中间某个目录不存在的情况 */
  uint32_t pathname_depth = path_depth_cnt((char*)pathname);

  /* 先检查文件是否存在 */
  int inode_no = search_file(pathname, &searched_record);
  bool found = inode_no != -1 ? true : false;

  if (searched_record.f_type == FT_DIRECTORY) {
    std::cout
        << "can`t open a direcotry with open(), use opendir() to instead\n";
    dir_close(searched_record.parent_dir);
    return -1;
  }

  uint32_t path_searched_depth = path_depth_cnt(searched_record.searched_path);

  /* 先判断是否把pathname的各层目录都访问到了,即是否在某个中间目录就失败了 */
  if (pathname_depth !=
      path_searched_depth) {  // 说明并没有访问到全部的路径,某个中间目录是不存在的
    std::cout << "cannot access " << pathname << ": Not a directory, subpath "
              << searched_record.searched_path << " is`t exist\n";
    dir_close(searched_record.parent_dir);
    return -1;
  }

  /* 若是在最后一个路径上没找到,并且并不是要创建文件,直接返回-1 */
  if (!found && !(flags & O_CREAT)) {
    std::cout << "in path " << searched_record.searched_path << ", file "
              << (strrchr(searched_record.searched_path, '/') + 1)
              << " is exist" << std::endl;
    dir_close(searched_record.parent_dir);
    return -1;
  } else if (found && flags & O_CREAT) {  // 若要创建的文件已存在
    std::cout << pathname << " has already exist!\n" << std::endl;
    dir_close(searched_record.parent_dir);
    return -1;
  }

  switch (flags & O_CREAT) {
    case O_CREAT:
      std::cout << ("creating file\n");
      fd = file_create(searched_record.parent_dir,
                       (char*)(strrchr(pathname, '/') + 1), flags);
      dir_close(searched_record.parent_dir);
      break;
    default:
      /* 其余情况均为打开已存在文件:
       * O_RDONLY,O_WRONLY,O_RDWR */
      fd = file_open(inode_no, flags);
  }

  /* 此fd是指任务pcb->fd_table数组中的元素下标,
   * 并不是指全局file_table中的下标 */
  return fd;
}

/* 将文件描述符转化为文件表的下标 */
static uint32_t fd_local2global(uint32_t local_fd) {
  int32_t global_fd = cur->fd_table[local_fd];
  if (!(global_fd >= 0 && global_fd < MAX_FILE_OPEN)) {
    std::cout << "!(global_fd >= 0 && global_fd < MAX_FILE_OPEN)!" << std::endl;
  }
  return (uint32_t)global_fd;
}

/* 关闭文件描述符fd指向的文件,成功返回0,否则返回-1 */
int32_t sys_close(int32_t fd) {
  int32_t ret = -1;  // 返回值默认为-1,即失败
  if (fd > 2) {
    uint32_t _fd = fd_local2global(fd);
    ret = file_close(&file_table[_fd]);
    cur->fd_table[fd] = -1;  // 使该文件描述符位可用
  }
  return ret;
}

/* 将buf中连续count个字节写入文件描述符fd,成功则返回写入的字节数,失败返回-1 */
int32_t sys_write(int32_t fd, const void* buf, uint32_t count) {
  if (fd < 0) {
    std::cout << "sys_write: fd error\n";
    return -1;
  }
  if (fd == stdout_no) {
    char tmp_buf[1024] = {0};
    memcpy(tmp_buf, buf, count);
    std::cin >> (tmp_buf);
    return count;
  }
  uint32_t _fd = fd_local2global(fd);
  struct file* wr_file = &file_table[_fd];
  if (wr_file->fd_flag & O_WRONLY || wr_file->fd_flag & O_RDWR) {
    uint32_t bytes_written = file_write(wr_file, buf, count);
    return bytes_written;
  } else {
    std::cout << "sys_write: not allowed to write file without flag O_RDWR or "
                 "O_WRONLY\n";
    return -1;
  }
}

/* 从文件描述符fd指向的文件中读取count个字节到buf,若成功则返回读出的字节数,到文件尾则返回-1
 */
int32_t sys_read(int32_t fd, void* buf, uint32_t count) {
  if (fd < 0) {
    std::cout << "sys_read: fd error\n";
    return -1;
  }
  if (!(buf != NULL)) return -1;
  uint32_t _fd = fd_local2global(fd);
  return file_read(&file_table[_fd], buf, count);
}

/* 重置用于文件读写操作的偏移指针,成功时返回新的偏移量,出错时返回-1 */
int32_t sys_lseek(int32_t fd, int32_t offset, uint8_t whence) {
  if (fd < 0) {
    std::cout << "sys_lseek: fd error\n";
    return -1;
  }
  if (!(whence > 0 && whence < 4)) return -1;
  uint32_t _fd = fd_local2global(fd);
  struct file* pf = &file_table[_fd];
  int32_t new_pos = 0;  //新的偏移量必须位于文件大小之内
  int32_t file_size = (int32_t)pf->fd_inode->i_size;
  switch (whence) {
    /* SEEK_SET 新的读写位置是相对于文件开头再增加offset个位移量 */
    case F_SEEK_SET:
      new_pos = offset;
      break;

    /* SEEK_CUR 新的读写位置是相对于当前的位置增加offset个位移量 */
    case F_SEEK_CUR:  // offse可正可负
      new_pos = (int32_t)pf->fd_pos + offset;
      break;

    /* SEEK_END 新的读写位置是相对于文件尺寸再增加offset个位移量 */
    case F_SEEK_END:  // 此情况下,offset应该为负值
      new_pos = file_size + offset;
  }
  if (new_pos < 0 || new_pos > (file_size - 1)) {
    return -1;
  }
  pf->fd_pos = new_pos;
  return pf->fd_pos;
}

/* 删除文件(非目录),成功返回0,失败返回-1 */
int32_t sys_unlink(const char* pathname) {
  if (!(strlen(pathname) < MAX_PATH_LEN)) return -1;

  /* 先检查待删除的文件是否存在 */
  struct path_search_record searched_record;
  memset(&searched_record, 0, sizeof(struct path_search_record));
  int inode_no = search_file(pathname, &searched_record);
  if (!(inode_no != 0)) return -1;
  if (inode_no == -1) {
    std::cout << "file " << pathname << " not found!\n";
    dir_close(searched_record.parent_dir);
    return -1;
  }
  if (searched_record.f_type == FT_DIRECTORY) {
    std::cout
        << "can`t delete a direcotry with unlink(), use rmdir() to instead\n";
    dir_close(searched_record.parent_dir);
    return -1;
  }

  /* 检查是否在已打开文件列表(文件表)中 */
  uint32_t file_idx = 0;
  while (file_idx < MAX_FILE_OPEN) {
    if (file_table[file_idx].fd_inode != NULL &&
        (uint32_t)inode_no == file_table[file_idx].fd_inode->i_no) {
      break;
    }
    file_idx++;
  }
  if (file_idx < MAX_FILE_OPEN) {
    dir_close(searched_record.parent_dir);
    std::cout << "file " << pathname << " is in use, not allow to delete!\n";
    return -1;
  }
  if (!(file_idx == MAX_FILE_OPEN)) return -1;

  /* 为delete_dir_entry申请缓冲区 */
  void* io_buf = (void*)malloc(SECTOR_SIZE + SECTOR_SIZE);
  if (io_buf == NULL) {
    dir_close(searched_record.parent_dir);
    std::cout << "sys_unlink: malloc for io_buf failed\n";
    return -1;
  }

  struct dir* parent_dir = searched_record.parent_dir;
  delete_dir_entry(cur_part, parent_dir, inode_no, io_buf);
  inode_release(cur_part, inode_no);
  free(io_buf);
  dir_close(searched_record.parent_dir);
  return 0;  // 成功删除文件
}

/* 创建目录pathname,成功返回0,失败返回-1 */
int32_t sys_mkdir(const char* pathname) {
  uint8_t rollback_step = 0;  // 用于操作失败时回滚各资源状态
  void* io_buf = (void*)malloc(SECTOR_SIZE * 2);
  uint32_t pathname_depth;
  uint32_t path_searched_depth;
  struct inode new_dir_inode;
  uint32_t block_bitmap_idx;
  int32_t block_lba;
  struct dir_entry* p_de;
  struct dir_entry new_dir_entry;
  struct dir* parent_dir;
  char* dirname;
  if (io_buf == NULL) {
    std::cout << "sys_mkdir: sys_malloc for io_buf failed\n";
    return -1;
  }

  struct path_search_record searched_record;
  memset(&searched_record, 0, sizeof(struct path_search_record));
  int inode_no = -1;
  inode_no = search_file(pathname, &searched_record);
  if (inode_no != -1) {  // 如果找到了同名目录或文件,失败返回
    std::cout << "sys_mkdir: file or directory " << pathname << " exist!\n";
    rollback_step = 1;
    goto rollback;
  } else {  // 若未找到,也要判断是在最终目录没找到还是某个中间目录不存在
    pathname_depth = path_depth_cnt((char*)pathname);
    path_searched_depth = path_depth_cnt(searched_record.searched_path);
    /* 先判断是否把pathname的各层目录都访问到了,即是否在某个中间目录就失败了 */
    if (pathname_depth !=
        path_searched_depth) {  // 说明并没有访问到全部的路径,某个中间目录是不存在的
      std::cout << "sys_mkdir: can`t access " << pathname << ", subpath "
                << searched_record.searched_path << " is`t exist\n";
      rollback_step = 1;
      goto rollback;
    }
  }

  parent_dir = searched_record.parent_dir;
  /* 目录名称后可能会有字符'/',所以最好直接用searched_record.searched_path,无'/'
   */
  dirname = strrchr(searched_record.searched_path, '/') + 1;

  inode_no = inode_bitmap_alloc(cur_part);
  if (inode_no == -1) {
    std::cout << "sys_mkdir: allocate inode failed\n";
    rollback_step = 1;
    goto rollback;
  }

  inode_init(inode_no, &new_dir_inode);  // 初始化i结点

  block_bitmap_idx = 0;  // 用来记录block对应于block_bitmap中的索引
  block_lba = -1;
  /* 为目录分配一个块,用来写入目录.和.. */
  block_lba = block_bitmap_alloc(cur_part);
  if (block_lba == -1) {
    std::cout << "sys_mkdir: block_bitmap_alloc for create directory failed\n";
    rollback_step = 2;
    goto rollback;
  }
  new_dir_inode.i_block[0] = block_lba;
  /* 每分配一个块就将位图同步到硬盘 */
  block_bitmap_idx = block_lba - cur_part->sb->data_start_lba;
  if (!(block_bitmap_idx != 0)) return -1;
  bitmap_sync(cur_part, block_bitmap_idx, BLOCK_BITMAP);

  /* 将当前目录的目录项'.'和'..'写入目录 */
  memset(io_buf, 0, SECTOR_SIZE * 2);  // 清空io_buf
  p_de = (struct dir_entry*)io_buf;

  /* 初始化当前目录"." */
  memcpy(p_de->filename, ".", 1);
  p_de->i_no = inode_no;
  p_de->f_type = FT_DIRECTORY;

  p_de++;
  /* 初始化当前目录".." */
  memcpy(p_de->filename, "..", 2);
  p_de->i_no = parent_dir->inode->i_no;
  p_de->f_type = FT_DIRECTORY;
  write_blocks_to_disk(cur_part->v_disk, new_dir_inode.i_block[0],
                       (char*)io_buf, 1);

  new_dir_inode.i_size = 2 * cur_part->sb->dir_entry_size;

  /* 在父目录中添加自己的目录项 */
  memset(&new_dir_entry, 0, sizeof(struct dir_entry));
  create_dir_entry(dirname, inode_no, FT_DIRECTORY, &new_dir_entry);
  memset(io_buf, 0, SECTOR_SIZE * 2);  // 清空io_buf
  if (!sync_dir_entry(
          parent_dir, &new_dir_entry,
          io_buf)) {  // sync_dir_entry中将block_bitmap通过bitmap_sync同步到硬盘
    std::cout << "sys_mkdir: sync_dir_entry to disk failed!\n";
    rollback_step = 2;
    goto rollback;
  }

  /* 父目录的inode同步到硬盘 */
  memset(io_buf, 0, SECTOR_SIZE * 2);
  inode_sync(cur_part, parent_dir->inode, io_buf);

  /* 将新创建目录的inode同步到硬盘 */
  memset(io_buf, 0, SECTOR_SIZE * 2);
  inode_sync(cur_part, &new_dir_inode, io_buf);

  /* 将inode位图同步到硬盘 */
  bitmap_sync(cur_part, inode_no, INODE_BITMAP);

  free(io_buf);

  /* 关闭所创建目录的父目录 */
  dir_close(searched_record.parent_dir);
  return 0;

/*创建文件或目录需要创建相关的多个资源,若某步失败则会执行到下面的回滚步骤 */
rollback:  // 因为某步骤操作失败而回滚
  switch (rollback_step) {
    case 2:
      bitmap_set(
          &cur_part->inode_bitmap, inode_no,
          0);  // 如果新文件的inode创建失败,之前位图中分配的inode_no也要恢复
    case 1:
      /* 关闭所创建目录的父目录 */
      dir_close(searched_record.parent_dir);
      break;
  }
  free(io_buf);
  return -1;
}

/* 目录打开成功后返回目录指针,失败返回NULL */
struct dir* sys_opendir(const char* name) {
  if (!(strlen(name) < MAX_PATH_LEN)) return NULL;
  /* 如果是根目录'/',直接返回&root_dir */
  if (name[0] == '/' && (name[1] == 0 || name[0] == '.')) {
    return &root_dir;
  }

  /* 先检查待打开的目录是否存在 */
  struct path_search_record searched_record;
  memset(&searched_record, 0, sizeof(struct path_search_record));
  int inode_no = search_file(name, &searched_record);
  struct dir* ret = NULL;
  if (inode_no == -1) {  // 如果找不到目录,提示不存在的路径
    std::cout << "In " << name << ", sub path " << searched_record.searched_path
              << " not exist\n";
  } else {
    if (searched_record.f_type == FT_REGULAR) {
      std::cout << name << " is regular file!\n";
    } else if (searched_record.f_type == FT_DIRECTORY) {
      ret = dir_open(cur_part, inode_no);
    }
  }
  dir_close(searched_record.parent_dir);
  return ret;
}

/* 成功关闭目录dir返回0,失败返回-1 */
int32_t sys_closedir(struct dir* dir) {
  int32_t ret = -1;
  if (dir != NULL) {
    dir_close(dir);
    ret = 0;
  }
  return ret;
}

/* 读取目录dir的1个目录项,成功后返回其目录项地址,到目录尾时或出错时返回NULL */
struct dir_entry* sys_readdir(struct dir* dir) {
  if (!(dir != NULL)) return NULL;
  return dir_read(dir);
}

/* 把目录dir的指针dir_pos置0 */
void sys_rewinddir(struct dir* dir) { dir->dir_pos = 0; }

/* 删除空目录,成功时返回0,失败时返回-1*/
int32_t sys_rmdir(const char* pathname) {
  /* 先检查待删除的文件是否存在 */
  struct path_search_record searched_record;
  memset(&searched_record, 0, sizeof(struct path_search_record));
  int inode_no = search_file(pathname, &searched_record);
  if (!(inode_no != 0)) return -1;
  int retval = -1;  // 默认返回值
  if (inode_no == -1) {
    std::cout << "In " << pathname << ", sub path "
              << searched_record.searched_path << " not exist\n";
  } else {
    if (searched_record.f_type == FT_REGULAR) {
      std::cout << pathname << " is regular file!\n";
    } else {
      struct dir* dir = dir_open(cur_part, inode_no);
      if (!dir_is_empty(dir)) {  // 非空目录不可删除
        std::cout << "dir " << pathname
                  << " is not empty, it is not allowed to delete a nonempty "
                     "directory!\n";
      } else {
        if (!dir_remove(searched_record.parent_dir, dir)) {
          retval = 0;
        }
      }
      dir_close(dir);
    }
  }
  dir_close(searched_record.parent_dir);
  return retval;
}

/* 获得父目录的inode编号 */
static uint32_t get_parent_dir_inode_nr(uint32_t child_inode_nr, void* io_buf) {
  struct inode* child_dir_inode = inode_open(cur_part, child_inode_nr);
  /* 目录中的目录项".."中包括父目录inode编号,".."位于目录的第0块 */
  uint32_t block_lba = child_dir_inode->i_block[0];
  if (!(block_lba >= cur_part->sb->data_start_lba)) return -1;
  inode_close(child_dir_inode);
  read_blocks_from_disk_unsafe(cur_part->v_disk, block_lba, (char*)io_buf, 1);
  struct dir_entry* dir_e = (struct dir_entry*)io_buf;
  /* 第0个目录项是".",第1个目录项是".." */
  if (!(dir_e[1].i_no < 4096 && dir_e[1].f_type == FT_DIRECTORY)) return -1;
  return dir_e[1].i_no;  // 返回..即父目录的inode编号
}

/* 在inode编号为p_inode_nr的目录中查找inode编号为c_inode_nr的子目录的名字,
 * 将名字存入缓冲区path.成功返回0,失败返-1 */
static int get_child_dir_name(uint32_t p_inode_nr, uint32_t c_inode_nr,
                              char* path, void* io_buf) {
  struct inode* parent_dir_inode = inode_open(cur_part, p_inode_nr);
  /* 填充all_blocks,将该目录的所占扇区地址全部写入all_blocks */
  uint8_t block_idx = 0;
  uint32_t all_blocks[140] = {0}, block_cnt = 12;
  while (block_idx < 12) {
    all_blocks[block_idx] = parent_dir_inode->i_block[block_idx];
    block_idx++;
  }
  if (parent_dir_inode
          ->i_block[12]) {  // 若包含了一级间接块表,将共读入all_blocks.
    read_blocks_from_disk_unsafe(cur_part->v_disk,
                                 parent_dir_inode->i_block[12],
                                 (char*)(all_blocks + 12), 1);
    block_cnt = 140;
  }
  inode_close(parent_dir_inode);

  struct dir_entry* dir_e = (struct dir_entry*)io_buf;
  uint32_t dir_entry_size = cur_part->sb->dir_entry_size;
  uint32_t dir_entrys_per_sec = (512 / dir_entry_size);
  block_idx = 0;
  /* 遍历所有块 */
  while (block_idx < block_cnt) {
    if (all_blocks[block_idx]) {  // 如果相应块不为空则读入相应块
      read_blocks_from_disk_unsafe(cur_part->v_disk, all_blocks[block_idx],
                                   (char*)io_buf, 1);
      uint8_t dir_e_idx = 0;
      /* 遍历每个目录项 */
      while (dir_e_idx < dir_entrys_per_sec) {
        if ((dir_e + dir_e_idx)->i_no == c_inode_nr) {
          strcat(path, "/");
          strcat(path, (dir_e + dir_e_idx)->filename);
          return 0;
        }
        dir_e_idx++;
      }
    }
    block_idx++;
  }
  return -1;
}

/* 把当前工作目录绝对路径写入buf, size是buf的大小.
 当buf为NULL时,由操作系统分配存储工作路径的空间并返回地址
 失败则返回NULL */
char* sys_getcwd(char* buf, uint32_t size) {
  /* 确保buf不为空,若用户进程提供的buf为NULL,
  系统调用getcwd中要为用户进程通过malloc分配内存 */
  if (!(buf != NULL)) return NULL;
  void* io_buf = (void*)malloc(SECTOR_SIZE);
  if (io_buf == NULL) {
    return NULL;
  }

  int32_t parent_inode_nr = 0;
  int32_t child_inode_nr = cur->cwd_inode_nr;
  if (!(child_inode_nr >= 0 && child_inode_nr < 4096))
    return NULL;  // 最大支持4096个inode
  /* 若当前目录是根目录,直接返回'/' */
  if (child_inode_nr == 0) {
    buf[0] = '/';
    buf[1] = 0;
    return buf;
  }

  memset(buf, 0, size);
  char full_path_reverse[MAX_PATH_LEN] = {0};  // 用来做全路径缓冲区

  /* 从下往上逐层找父目录,直到找到根目录为止.
   * 当child_inode_nr为根目录的inode编号(0)时停止,
   * 即已经查看完根目录中的目录项 */
  while ((child_inode_nr)) {
    parent_inode_nr = get_parent_dir_inode_nr(child_inode_nr, io_buf);
    if (get_child_dir_name(parent_inode_nr, child_inode_nr, full_path_reverse,
                           io_buf) == -1) {  // 或未找到名字,失败退出
      free(io_buf);
      return NULL;
    }
    child_inode_nr = parent_inode_nr;
  }
  if (!(strlen(full_path_reverse) <= size)) return NULL;
  /* 至此full_path_reverse中的路径是反着的,
   * 即子目录在前(左),父目录在后(右) ,
   * 现将full_path_reverse中的路径反置 */
  char* last_slash;  // 用于记录字符串中最后一个斜杠地址
  while ((last_slash = strrchr(full_path_reverse, '/'))) {
    uint16_t len = strlen(buf);
    strcpy(buf + len, last_slash);
    /* 在full_path_reverse中添加结束字符,做为下一次执行strcpy中last_slash的边界
     */
    *last_slash = 0;
  }
  free(io_buf);
  return buf;
}

/* 更改当前工作目录为绝对路径path,成功则返回0,失败返回-1 */
int32_t sys_chdir(const char* path) {
  int32_t ret = -1;
  struct path_search_record searched_record;
  memset(&searched_record, 0, sizeof(struct path_search_record));
  int inode_no = search_file(path, &searched_record);
  if (inode_no != -1) {
    if (searched_record.f_type == FT_DIRECTORY) {
      cur->cwd_inode_nr = inode_no;
      ret = 0;
    } else {
      std::cout << "sys_chdir: " << path << " is regular file or other!\n";
    }
  }
  dir_close(searched_record.parent_dir);
  return ret;
}

/* 在buf中填充文件结构相关信息,成功时返回0,失败返回-1 */
int32_t sys_stat(const char* path, struct stat* buf) {
  /* 若直接查看根目录'/' */
  if (!strcmp(path, "/") || !strcmp(path, "/.") || !strcmp(path, "/..")) {
    buf->st_f_type = FT_DIRECTORY;
    buf->st_ino = 0;
    buf->st_size = root_dir.inode->i_size;
    return 0;
  }

  int32_t ret = -1;  // 默认返回值
  struct path_search_record searched_record;
  memset(
      &searched_record, 0,
      sizeof(struct
             path_search_record));  // 记得初始化或清0,否则栈中信息不知道是什么
  int inode_no = search_file(path, &searched_record);
  if (inode_no != -1) {
    struct inode* obj_inode =
        inode_open(cur_part, inode_no);  // 只为获得文件大小
    buf->st_size = obj_inode->i_size;
    inode_close(obj_inode);
    buf->st_f_type = searched_record.f_type;
    buf->st_ino = inode_no;
    ret = 0;
  } else {
    std::cout << "sys_stat: " << path << " not found\n";
  }
  dir_close(searched_record.parent_dir);
  return ret;
}
