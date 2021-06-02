#include "inode.h"

#include "fs.h"

/* 用来存储inode位置 */
typedef struct inode_position {
  bool two_blk;       // inode是否跨块
  uint32_t lba;       // inode所在的块号
  uint32_t off_size;  // inode在块内的 字节 偏移量
} inode_position;

/* 获取inode所在的块和块内的偏移量 */
static void inode_locate(current_partition* cp, uint32_t inode_no,
                         inode_position* inode_pos) {
  /* inode_table在硬盘上是连续的 */
  if (inode_no < 4096) {
    return;
  }
  uint32_t inode_table_lba = cp->sb->inode_table_lba;

  uint32_t inode_size = sizeof(struct inode);
  uint32_t off_size =
      inode_no *
      inode_size;  // 第inode_no号I结点相对于inode_table_lba的字节偏移量
  uint32_t off_sec =
      off_size /
      BLOCK_SIZE;  // 第inode_no号I结点相对于inode_table_lba的扇区偏移量
  uint32_t off_size_in_blk =
      off_size % BLOCK_SIZE;  // 待查找的inode所在扇区中的起始地址

  /* 判断此i结点是否跨越2个扇区 */
  uint32_t left_in_sec = 512 - off_size_in_blk;
  if (left_in_sec <
      inode_size) {  // 若扇区内剩下的空间不足以容纳一个inode,必然是I结点跨越了2个扇区
    inode_pos->two_blk = true;
  } else {  // 否则,所查找的inode未跨扇区
    inode_pos->two_blk = false;
  }
  inode_pos->lba = inode_table_lba + off_sec;
  inode_pos->off_size = off_size_in_blk;
}

/* 将inode写入到分区part */
void inode_sync(current_partition* cp, inode* inode,
                void* io_buf) {  // io_buf是用于硬盘io的缓冲区
  uint8_t inode_no = inode->i_no;
  inode_position inode_pos;
  inode_locate(cp, inode_no, &inode_pos);  // inode位置信息会存入inode_pos
  if (inode_pos.lba <=
      (cp->v_disk->partitions[cp->partition_no].start_sector_no +
       cp->v_disk->partitions[cp->partition_no].sector_count))
    return;

  /* 硬盘中的inode中的成员inode_tag和i_open_cnts是不需要的,
   * 它们只在内存中记录链表位置和被多少进程共享 */
  struct inode pure_inode;
  memcpy(&pure_inode, inode, sizeof(struct inode));

  /* 以下inode的三个成员只存在于内存中,现在将inode同步到硬盘,清掉这三项即可 */
  pure_inode.i_open_counts = 0;
  pure_inode.write_deny = false;  // 置为false,以保证在硬盘中读出时为可写
  pure_inode.inode_tag.prev = pure_inode.inode_tag.next = NULL;

  char* inode_buf = (char*)io_buf;
  if (inode_pos.two_blk) {  // 若是跨了两个扇区,就要读出两个扇区再写入两个扇区
    /* 读写硬盘是以扇区为单位,若写入的数据小于一扇区,要将原硬盘上的内容先读出来再和新数据拼成一扇区后再写入
     */
    // char* tmp = read_blocks_from_disk(cp->v_disk, inode_pos.lba, 2);
    // memcpy(inode_buf, tmp, sizeof(tmp));
    // free(tmp);
    read_blocks_from_disk_unsafe(cp->v_disk, inode_pos.lba, inode_buf, 2);

    /* 开始将待写入的inode拼入到这2个扇区中的相应位置 */
    memcpy((inode_buf + inode_pos.off_size), &pure_inode, sizeof(struct inode));

    /* 将拼接好的数据再写入磁盘 */
    write_blocks_to_disk(cp->v_disk, inode_pos.lba, inode_buf, 2);
  } else {  // 若只是一个扇区
    // char* tmp = read_blocks_from_disk(cp->v_disk, inode_pos.lba, 1);
    // memcpy(inode_buf, tmp, sizeof(tmp));
    // free(tmp);
    read_blocks_from_disk_unsafe(cp->v_disk, inode_pos.lba, inode_buf, 1);
    memcpy((inode_buf + inode_pos.off_size), &pure_inode, sizeof(struct inode));
    write_blocks_to_disk(cp->v_disk, inode_pos.lba, inode_buf, 1);
  }
}

/* 根据i结点号返回相应的i结点 */
inode* inode_open(current_partition* cp, uint32_t inode_no) {
  /* 先在已打开inode链表中找inode,此链表是为提速创建的缓冲区 */
  list_elem* elem = cp->open_inodes.head.next;
  inode* inode_found;
  while (elem != &cp->open_inodes.tail) {
    inode_found = elem2entry(inode, inode_tag, elem);
    if (inode_found->i_no == inode_no) {
      inode_found->i_open_counts++;
      return inode_found;
    }
    elem = elem->next;
  }

  /*由于open_inodes链表中找不到,下面从硬盘上读入此inode并加入到此链表 */
  inode_position inode_pos;

  /* inode位置信息会存入inode_pos, 包括inode所在扇区地址和扇区内的字节偏移量 */
  inode_locate(cp, inode_no, &inode_pos);

  inode_found = (inode*)malloc(sizeof(inode));

  char* inode_buf;
  if (inode_pos.two_blk) {  // 考虑跨扇区的情况
    inode_buf = (char*)malloc(2 * BLOCK_SIZE);

    /* i结点表是被partition_format函数连续写入扇区的,
     * 所以下面可以连续读出来 */
    read_blocks_from_disk_unsafe(cp->v_disk, inode_pos.lba, inode_buf, 2);
  } else {  // 否则,所查找的inode未跨扇区,一个扇区大小的缓冲区足够
    inode_buf = (char*)malloc(BLOCK_SIZE);
    read_blocks_from_disk_unsafe(cp->v_disk, inode_pos.lba, inode_buf, 1);
  }
  memcpy(inode_found, inode_buf + inode_pos.off_size, sizeof(inode));

  /* 因为一会很可能要用到此inode,故将其插入到队首便于提前检索到 */
  list_push(&cp->open_inodes, &inode_found->inode_tag);
  inode_found->i_open_counts = 1;

  free(inode_buf);
  return inode_found;
}

/* 关闭inode或减少inode的打开数 */
void inode_close(inode* inode) {
   /* 若没有进程再打开此文件,将此inode去掉并释放空间 */
   if (--inode->i_open_counts == 0) {
      list_remove(&inode->inode_tag);	  // 将I结点从part->open_inodes中去掉
      free(inode);
   }
}

/* 初始化new_inode */
void inode_init(uint32_t inode_no, inode* new_inode) {
   new_inode->i_no = inode_no;
   new_inode->i_size = 0;
   new_inode->i_open_counts = 0;
   new_inode->write_deny = false;

   /* 初始化块索引数组i_sector */
   uint8_t sec_idx = 0;
   while (sec_idx < 13) {
   /* i_sectors[12]为一级间接块地址 */
      new_inode->i_block[sec_idx] = 0;
      sec_idx++;
   }
}