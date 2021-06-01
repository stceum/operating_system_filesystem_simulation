#include "fs.h"
#include "tools.h"
#include "inode.h"
#include "dir.h"
#include "super_block.h"

#include <iostream>
#define SUCCESS 1
#define FAIL 0

int write_data_to_disk(virtual_disk *disk, uint32_t address, char* data) {
    std::fstream fs(disk->disk_name, std::ios::binary | std::ios::out | std::ios::in);
    if (fs) {
        fs.seekp(address, std::ios::beg);
    	fs.write(data, sizeof(data));
    	fs.close();
        return SUCCESS;
    } else {
        return FAIL;
    }
}

char* read_data_from_disk(virtual_disk *disk, uint32_t address, size_t size) {
    std::fstream fs(disk->disk_name, std::ios::binary | std::ios::out | std::ios::in);
    if (fs) {
        fs.seekp(address, std::ios::beg);
        char* data = (char*)malloc(size);
    	fs.read(data, size);
    	fs.close();
        return data;
    } else {
        return (char*)(0);
    }
}

// 逻辑块地址是指第几块 类似扇区号
// lbc -> Logic Block Count
// lba -> Logic Block Address
int partition_format(virtual_disk* v_disk, int partition_no) {
    /* 获取分区起始、结束块地址 */
    disk_partition dp = v_disk->partitions[partition_no];
    uint32_t start_lba = dp.start_sector_no ;
    uint32_t logic_lbc = dp.sector_count * (BLOCK_SIZE / SECTOR_SIZE);
    uint32_t end_lba = start_lba + logic_lbc - 1;

    /* 根据分区大小，计算各元信息需要的扇区数及位置 */
    uint32_t boot_sector_lbc = 1; // 启动块 占位
    uint32_t super_block_lbc = 1; // 超级块
    // inode 位图占用块数
    uint32_t inode_bitmap_lbc = 
        DIV_ROUND_UP(MAX_FILES_PER_PART ,(BITS_PER_SECTOR * SECTOR_SIZE / BLOCK_SIZE));
    // inode 数组占用块数
    uint32_t inode_table_lbc = 
        DIV_ROUND_UP(sizeof(inode) * MAX_FILES_PER_PART, BLOCK_SIZE);
    // 已使用的块数
    uint32_t used_lbc = boot_sector_lbc + super_block_lbc 
        + inode_bitmap_lbc + inode_table_lbc;
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
    sb.block_bitmap_lbc = ;

    sb.inode_bitmap_lba = sb.block_bitmap_lba + sb.block_bitmap_lbc;
    sb.inode_bitmap_lbc = inode_bitmap_lbc;

    sb.inode_table_lba = sb.inode_bitmap_lba + sb.inode_bitmap_lbc;
    sb.inode_table_lbc = inode_table_lbc;

    sb.data_start_lba = sb.inode_table_lba + sb.inode_table_lbc;
    sb.root_inode_no = 0;
    sb.dir_entry_size = sizeof(dir_entry);

    std::cout << "partition info:\n" << "name: " << dp.partition_name << "\n" 
        << "magic: \t" << sb.magic << std::endl
        << "block count: \t" << sb.block_count << std::endl
        << "inode count: \t" << sb.inode_count << std::endl
        << "start lba: \t" << sb.start_lba << std::endl
        << "block bitmap lba: \t" << sb.block_bitmap_lba << std::endl
        << "block bitmap bc: \t" << sb.block_bitmap_lbc << std::endl
        << "inode bitmap lba: \t" << sb.inode_bitmap_lba << std::endl
        << "inode bitmap bc: \t" << sb.inode_bitmap_lbc << std::endl
        << "inode table lba: \t" << sb.inode_table_lba << std::endl
        << "inode table bc: \t" << sb.inode_table_lbc << std::endl
        << "data start lba: \t" << sb.data_start_lba << std::endl
        << "root inode no: \t" << sb.root_inode_no << std::endl
        << "dir entry size: \t" << sb.dir_entry_size << std::endl;

    /* 将超级块写入磁盘 */
    // 写入 第1块
    
    /* 将元信息写入磁盘中各自的位置 */
    /* 将根目录写入磁盘 */
}

int main() {
    virtual_disk test_d;
    test_d.disk_volumn = 536870912;
    strcpy(test_d.disk_name, "helloDisk");
    test_d.current_partition_count = 2;
    strcpy(test_d.partitions[1].partition_name, "helloP");
    test_d.partitions[1].sector_count = 102400;
    test_d.partitions[1].start_sector_no = 2048;
    partition_format(&test_d, 1);
}