#include "fs.h"
#include "tools.h"
#include "inode.h"
#include "dir.h"
#include "super_block.h"

#include <iostream>
#define SUCCESS 1
#define FAIL 0

int write_blocks_to_disk(virtual_disk *disk, uint32_t lba, char* data,size_t bc) {
    std::fstream fs(disk->disk_name, std::ios::binary | std::ios::out | std::ios::in);
    if (fs) {
        // std::cout << std::hex;
        // std::cout << "======" << (lba * BLOCK_SIZE) << std::endl;
        fs.seekp(lba * BLOCK_SIZE, std::ios::beg);
    	fs.write(data, bc * BLOCK_SIZE);
    	fs.close();
        return SUCCESS;
    } else {
        return FAIL;
    }
}

char* read_blocks_from_disk(virtual_disk *disk, uint32_t lba, size_t bc) {
    std::fstream fs(disk->disk_name, std::ios::binary | std::ios::out | std::ios::in);
    if (fs) {
        // std::cout << std::hex;
        // std::cout << "======" << (lba * BLOCK_SIZE) << std::endl;
        fs.seekp(lba * BLOCK_SIZE, std::ios::beg);
        char* data = (char*)malloc(bc * BLOCK_SIZE);
    	fs.read(data, bc * BLOCK_SIZE);
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
    sb.block_bitmap_lbc = block_bitmap_lbc;

    sb.inode_bitmap_lba = sb.block_bitmap_lba + sb.block_bitmap_lbc;
    sb.inode_bitmap_lbc = inode_bitmap_lbc;

    sb.inode_table_lba = sb.inode_bitmap_lba + sb.inode_bitmap_lbc;
    sb.inode_table_lbc = inode_table_lbc;

    sb.data_start_lba = sb.inode_table_lba + sb.inode_table_lbc;
    sb.root_inode_no = 0;
    sb.dir_entry_size = sizeof(dir_entry);

    std::cout << "partition info:\n" << "name: \t\t\t" << dp.partition_name << "\n" 
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
    if(write_blocks_to_disk(v_disk, start_lba + 1, (char *)&sb, sizeof(sb))) {
        std::cout << "Super block has been written to disk!" << std::endl;
    } else {
        std::cout << "Super block failed to write to disk!" << std::endl;
        return FAIL;
    }

    // 设置写入磁盘的缓存
    uint32_t buf_size = sb.block_bitmap_lbc > sb.inode_bitmap_lbc ? \
        sb.block_bitmap_lbc : sb.inode_bitmap_lbc;
    buf_size = (buf_size > sb.inode_table_lbc ? \
        buf_size : sb.inode_table_lbc) * BLOCK_SIZE;
    uint8_t* buf = (uint8_t *)malloc(buf_size);

    /* 2 将块位图写入磁盘*/
    buf[0] |= 0x01; // 将第0块预留给根目录
    // 位图所在块
    uint32_t block_bitmap_last_byte = free_lbc / 8;
    uint8_t block_bitmap_last_bit = free_lbc % 8;
    // last_size 是位图最后一个块中，不足一块的其余部分
    uint32_t last_size = BLOCK_SIZE - \
        (block_bitmap_last_byte % BLOCK_SIZE);
    // 最后 last_size 位置 1
    memset(&buf[last_size], 0xff, last_size);
    // 其余位置 0 
    uint8_t bit_idx;
    while (bit_idx <= block_bitmap_last_bit) {
        buf[block_bitmap_last_byte] &= ~ (1 << bit_idx++);
    }
    if(write_blocks_to_disk(v_disk, sb.block_bitmap_lba, (char *)buf, sb.block_bitmap_lbc)) {
        std::cout << "Block bitmap has been written to disk!" << std::endl;
    } else {
        std::cout << "Block bitmap failed to write to disk!" << std::endl;
        return FAIL;
    }
    
    /* 3 将 inode 位图写入磁盘 */
    memset(buf, 0, buf_size);  // 清空缓存区域
    buf[0] |= 0x1;             // 第 0 个 inode 分给根目录
    // 4096 个 inode 正好一个块
    if(write_blocks_to_disk(v_disk, sb.inode_bitmap_lba, (char *)buf, sb.inode_bitmap_lbc)) {
        std::cout << "Inode bitmap has been written to disk!" << std::endl;
    } else {
        std::cout << "Inode bitmap failed to write to disk!" << std::endl;
        return FAIL;
    }

    /* 4 将 inode 数组写入磁盘 */
    memset(buf, 0, buf_size);
    inode *i = (inode *)buf;
    // 根目录
    i->i_size = sb.dir_entry_size * 2; // 根目录只有 . 和 ..
    i->i_no = 0;
    i->i_block[0] = sb.data_start_lba;
    if(write_blocks_to_disk(v_disk, sb.inode_table_lba, (char *)buf, sb.inode_table_lbc)) {
        std::cout << "Inode table has been written to disk!" << std::endl;
    } else {
        std::cout << "Inode table failed to write to disk!" << std::endl;
        return FAIL;
    }

    /* 5 将根目录写入磁盘 */
    memset(buf, 0, buf_size);
    dir_entry * de_ptr = (dir_entry *)buf;

    // 当前目录 .
    memcpy(de_ptr->name, ".", 1);
    de_ptr->i_no = 0;
    de_ptr->ft = FT_DIRCTORY;
    de_ptr++;

    // 当前目录的父目录 ..
    memcpy(de_ptr->name, "..", 2);
    de_ptr->i_no = 0;
    de_ptr->ft = FT_DIRCTORY;
    if(write_blocks_to_disk(v_disk, sb.data_start_lba, (char *)buf, 1)) {
        std::cout << "Dir Root has been written to disk!" << std::endl;
    } else {
        std::cout << "Dir Root failed to write to disk!" << std::endl;
        return FAIL;
    }
    free(buf);
    return SUCCESS;
}

void fs_init(virtual_disk* v_disk) {
    std::cout << "Searching filesystem on " << v_disk->disk_name << std::endl;
    for (int i = 0; i < v_disk->current_partition_count; i++) {
        super_block* dp =(super_block*)read_blocks_from_disk(v_disk, v_disk->partitions[i].start_sector_no + 1, 1); 
        if (dp->magic == 0x19590318) {
            std::cout << v_disk->partitions[i].partition_name << " has filesystem!" << std::endl;
        } else {
            std::cout << v_disk->partitions[i].partition_name << " has not filesystem!" << std::endl;
            std::cout << "Initialing..." << std::endl;
            partition_format(v_disk, i);
        }
        free(dp);
    }
    
}

// int main() {
//     // create_disk((char*)"test_disk", 512 * (1048576));
//     // int sizes[2] = {268173304, 268173304};
//     // char *names[8] = {(char*)"p1", (char*)"p2"};
//     // create_partitions((char*)"test_disk", 2, sizes, names);

//     virtual_disk test_d = read_disk((char*)"test_disk");
//     // partition_format(&test_d, 1);
//     fs_init(&test_d);
// }