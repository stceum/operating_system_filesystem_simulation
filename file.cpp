#include "file.h"
#include "fs.h"
#include "bitmap.h"
#include "pcb.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <fstream>
using namespace std;

#define FAIL -1

file file_table[MAX_FILE_OPEN];
int32_t get_free_slot_in_file_table(void)
{
    uint32_t fd_idx = 3;
    while (fd_idx < MAX_FILE_OPEN) {
        if (file_table[fd_idx].fd_inode == NULL) {
        break;
        }
        fd_idx++;
    }
    if (fd_idx == MAX_FILE_OPEN) {
        cout << "ERROR:cannot open more file";
        return FAIL;
    }
    return fd_idx;
}

int32_t pcb_fd_install(int32_t file_table_free_slot)
{
    pcb* cur = (pcb*)malloc(sizeof(pcb));
    init_pcb(cur);
    uint32_t local_fd_idx = 3; 
    // for (int i = 0; i < MAX_FILES_OPEN_PER_PROC; i++)
    // {
    //     cout << cur->fd_table[i] << endl;
    // }
    
    while (local_fd_idx < MAX_FILES_OPEN_PER_PROC) {
        if (cur->fd_table[local_fd_idx] == -1) {	// -1表示free_slot,可用
        cur->fd_table[local_fd_idx] = file_table_free_slot;
        break;
        }
        local_fd_idx++;
    }
    if (local_fd_idx == MAX_FILES_OPEN_PER_PROC) {
        cout << "ERROR: PCB cannot open more file";
        return FAIL;
    }
    free(cur);
    return local_fd_idx;
}

int32_t inode_bitmap_alloc(struct current_partition* part)
{
    int32_t bit_idx = bitmap_scan(&part->inode_bitmap, 1);
    if (bit_idx == -1) {
        cout << "ERROR:no empty inode block"<<endl;
        return FAIL;
    }
    bitmap_set(&part->inode_bitmap, bit_idx, 1);
    return bit_idx;
}

int32_t block_bitmap_alloc(struct current_partition* part)
{
    int32_t bit_idx = bitmap_scan(&part->block_bitmap, 1);
    if (bit_idx == -1) {
        cout << "ERROR:no empty block"<<endl;
        return FAIL;
    }
    bitmap_set(&part->block_bitmap, bit_idx, 1);
    return (part->sb->data_start_lba + bit_idx);
}

void bitmap_sync(struct current_partition* part, uint32_t bit_idx, uint8_t btmp_type)
{
    uint32_t off_sec = bit_idx / 4096;  // 本i结点索引相对于位图的扇区偏移量
    uint32_t off_size = off_sec * BLOCK_SIZE;  // 本i结点索引相对于位图的字节偏移量
    uint32_t sec_lba;
    uint8_t* bitmap_off = (uint8_t*)malloc(sizeof(bitmap_off));
    // cout << "coe" << endl;

    /* 需要被同步到硬盘的位图只有inode_bitmap和block_bitmap */
    switch (btmp_type) {
        case INODE_BITMAP:
        
        sec_lba = part->sb->inode_bitmap_lba + off_sec;
        bitmap_off = part->inode_bitmap.bits + off_size;
        break;

        case BLOCK_BITMAP: 
        sec_lba = part->sb->block_bitmap_lba + off_sec;
        bitmap_off = part->block_bitmap.bits + off_size;
        break;
    }

    write_blocks_to_disk(part->v_disk, sec_lba,(char*)bitmap_off, 1);
    // read_blocks_from_disk_unsafe(part->v_disk, sec_lba,(char*)bitmap_off, 1);
    // cout << bitmap_off <<endl;

}

int main()
{
    // int32_t fd_index = get_free_slot_in_file_table();
    // if(fd_index!=-1){
    //     int local_fd_idx = pcb_fd_install(fd_index);
    //     if(local_fd_idx != -1){
    //         virtual_disk test_d = read_disk((char*)"test_disk");
    //         // partition_format(&test_d, 1);
    //         fs_init(&test_d);
    //         current_partition tmp = mount_partition(&test_d, 1);
    //         current_partition* test_partition = &tmp;
    //         int bit_idx = inode_bitmap_alloc(test_partition);
    //         int btmp_type = block_bitmap_alloc(test_partition);
    //         if(bit_idx==-1 || btmp_type == -1)
    //         {
    //             cout<<"ERROR:inode or block is full"<<endl;
    //         }
    //         else
    //         {
    //             // cout<<"correct"<<endl;
    //             bitmap_sync(test_partition,bit_idx,btmp_type);
    //         }
            
    //     }
    // }
    // else{
    //     cout<<"first error"<<endl;
    // }
}
