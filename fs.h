#ifndef FS_H
#define FS_H
// 该头文件存储基本磁盘信息

#define MAX_FILES_PER_PART 4096  // 每个分区最大文件数4k
#define BITS_PER_SECTOR 4096     // 每个扇区的二进制位数 512B
#define SECTOR_SIZE 512          // 每个扇区512字节
#define BLOCK_SIZE SECTOR_SIZE   // 一个扇区一个块

#define MAX_PATH_LEN 512  // 路径最大长度

#include <cstring>
#include <fstream>

#include "bitmap.h"
#include "dir.h"
#include "disk.h"
#include "list.h"
#include "super_block.h"

// 用于放在内存中 用于"挂载"当前的分区
typedef struct current_partition {
  virtual_disk* v_disk;  // 磁盘名
  int partition_no;      // 第几个分区
  super_block* sb;
  bitmap block_bitmap;
  bitmap inode_bitmap;
  list open_inodes;  // 本分区打开的i结点队列
} current_partition;

extern current_partition* cur_part;

/* 打开文件的选项 */
enum oflags {
  O_RDONLY,    // 只读
  O_WRONLY,    // 只写
  O_RDWR,      // 读写
  O_CREAT = 4  // 创建
};

/* 文件读写位置偏移量 */
enum whence { F_SEEK_SET = 1, F_SEEK_CUR, F_SEEK_END };

/* 用来记录查找文件过程中已找到的上级路径,也就是查找文件过程中"走过的地方" */
typedef struct path_search_record {
  char searched_path[MAX_PATH_LEN];  // 查找过程中的父路径
  struct dir* parent_dir;            // 文件或目录所在的直接父目录
  file_type f_type;  // 找到的是普通文件还是目录,找不到将为未知类型(FT_UNKNOWN)
} path_search_record;

/* 文件属性结构体 */
struct stat {
  uint32_t st_ino;      // inode编号
  uint32_t st_size;     // 尺寸
  file_type st_f_type;  // 文件类型
};

/* 向 某个磁盘 的 某个地址 写入 数据 */
int write_blocks_to_disk(virtual_disk* disk, uint32_t lba, char* data,
                         size_t bc);
/* 从 某个磁盘 的 某个块号 读出 size 大小的数据 并返回指针 */
char* read_blocks_from_disk(virtual_disk* disk, uint32_t lba, size_t bc);
/*
 * 功能和 read blocks from disk 函数
 * 相同，不过此函数不分配空间，需要提前分配好空间，并传入指针
 * 由于可能传入一个空指针，因此"unsafe"
 */
int read_blocks_from_disk_unsafe(virtual_disk* disk, uint32_t lba, char* data,
                                 size_t bc);
/* 将 v_disk 的第 partition_no 个分区格式化 (从0开始计数)*/
int partition_format(virtual_disk* v_disk, int partition_no);
void fs_init(virtual_disk* v_disk); /* 文件系统初始化 */
int mount_partition(virtual_disk* v_disk,
                    int partition_no); /* "挂载"某个分区 */

int32_t path_depth_cnt(char* pathname);

#endif