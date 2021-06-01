#ifndef FS_H 
#define FS_H
// 该头文件存储基本磁盘信息

#define MAX_FILES_PER_PART 4096 // 每个分区最大文件数4k
#define BITS_PER_SECTOR 4096    // 每个扇区的二进制位数 512B
#define SECTOR_SIZE 512         // 每个扇区512字节 
#define BLOCK_SIZE SECTOR_SIZE  // 一个扇区一个块

#include "disk.h"
/* 将 v_disk 的 d_partition 分区*/
int partition_format(virtual_disk* v_disk, disk_partition * d_partition);
#endif