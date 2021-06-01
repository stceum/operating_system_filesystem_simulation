#include "fs.h"

int partition_format(virtual_disk* v_disk, disk_partition * d_partition) {
    // 获取分区起始、结束扇区
    
    // 根据分区大小，计算各元信息需要的扇区数及位置
    // 在内存中创建超级块，并将元信息写入超级块
    // 将超级块写入磁盘
    // 将元信息写入磁盘中各自的位置
    // 将根目录写入磁盘
}