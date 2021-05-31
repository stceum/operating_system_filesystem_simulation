#ifndef DISK_H
#define DISK_H

#include <stdint.h>
#define MAX_DISK_NAME_LEN 16    // 最长磁盘名长度(模拟磁盘的文件名长度)
#define MAX_SECTOR_NAME_LEN 8   //最长分区名
#define MAX_PARTIOTION_NUMBER 8 //最大分区数

typedef struct disk_partition
{
    uint32_t start_sector_no;               // 起始扇区号
    uint32_t sector_count_per_partition;    // 扇区长度
    char partition_name[MAX_SECTOR_NAME_LEN];
}disk_partition;

typedef struct virtual_disk
{
    char disk_name[MAX_DISK_NAME_LEN];
    uint32_t disk_volumn;                           // 磁盘容量 字节数
    int current_partition_count;                    // 当前磁盘分区数
    disk_partition partitions[MAX_PARTIOTION_NUMBER];
}virtual_disk;

/*
 * 创建磁盘的函数
 * 该函数新建一个 disk_name 的文件作为虚拟磁盘，并分配 disk_volumn 大小的空间
 * 并在磁盘/文件的最前端预留数目为 MAX_PARTIOTION_NUMBER 的 partition 结构体
 * 成功返回 1/true 失败返回 0/false
 */
int create_disk(char* disk_name, uint32_t disk_volumn); 

/* 
 * 对磁盘进行分区
 * 传入硬盘名，分区数，每个分区的大小的数组，以及每个分区的名字的数组
 * 将"分区表"写入create_disk的预留区域
 * 成功返回 1/true 失败返回 0/false
 */
int create_partitions(char* disk_name, int partition_num, int** partitions_size, char** partitions_name);

#endif