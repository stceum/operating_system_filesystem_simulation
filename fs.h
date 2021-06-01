#ifndef FS_H 
#define FS_H
// 该头文件存储基本磁盘信息

#define MAX_FILES_PER_PART 4096 // 每个分区最大文件数4k
#define BITS_PER_SECTOR 4096    // 每个扇区的二进制位数 512B
#define SECTOR_SIZE 512         // 每个扇区512字节 
#define BLOCK_SIZE SECTOR_SIZE  // 一个扇区一个块

#include "disk.h"
#include <fstream>
#include <cstring>

/* 向 某个磁盘 的 某个地址 写入 数据 */
int write_blocks_to_disk(virtual_disk *disk,  uint32_t lba, char* data, size_t bc);

/* 从 某个磁盘 的 某个块号 读出 size 大小的数据 并返回指针 */
char* read_blocks_from_disk(virtual_disk *disk,  uint32_t lba, size_t bc);

/* 将 v_disk 的第 partition_no 个分区格式化 (从0开始计数)*/
int partition_format(virtual_disk* v_disk, int partition_no);

/* 文件系统初始化 */
void fs_init(virtual_disk* v_disk);
#endif