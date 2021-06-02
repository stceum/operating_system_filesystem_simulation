#include "file.h"
#include "fs.h"
#include "bitmap.h"
#include "inode.h"
#include "pcb.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <memory.h>
#include <string.h>
#include <fstream>
using namespace std;

#define FAIL -1

file file_table[MAX_FILE_OPEN];
int32_t get_free_slot_in_file_table(void)
{
    uint32_t fd_idx = 3;
    while (fd_idx < MAX_FILE_OPEN)
    {
        if (file_table[fd_idx].fd_inode == NULL)
        {
            break;
        }
        fd_idx++;
    }
    if (fd_idx == MAX_FILE_OPEN)
    {
        cout << "ERROR:cannot open more file";
        return FAIL;
    }
    return fd_idx;
}

int32_t pcb_fd_install(int32_t file_table_free_slot)
{
    pcb *cur = (pcb *)malloc(sizeof(pcb));
    init_pcb(cur);
    uint32_t local_fd_idx = 3;
    // for (int i = 0; i < MAX_FILES_OPEN_PER_PROC; i++)
    // {
    //     cout << cur->fd_table[i] << endl;
    // }

    while (local_fd_idx < MAX_FILES_OPEN_PER_PROC)
    {
        if (cur->fd_table[local_fd_idx] == -1)
        { // -1表示free_slot,可用
            cur->fd_table[local_fd_idx] = file_table_free_slot;
            break;
        }
        local_fd_idx++;
    }
    if (local_fd_idx == MAX_FILES_OPEN_PER_PROC)
    {
        cout << "ERROR: PCB cannot open more file";
        return FAIL;
    }
    free(cur);
    return local_fd_idx;
}

/* 创建文件,若成功则返回文件描述符,否则返回-1 */
// int32_t file_create(struct dir *parent_dir, char *filename, uint8_t flag)
// {
//     /* 后续操作的公共缓冲区 */
//     void *io_buf = malloc(1024);
//     if (io_buf == NULL)
//     {
//         cout << "in file_creat: sys_malloc for io_buf failed" << endl;
//         return -1;
//     }
//     int fd_idx = get_free_slot_in_file_table();
//     struct inode *new_file_inode = (struct inode *)malloc(sizeof(struct inode));
//     uint8_t rollback_step = 0; // 用于操作失败时回滚各资源状态

//     /* 为新文件分配inode */
//     int32_t inode_no = inode_bitmap_alloc(cur_part);
//     if (inode_no == -1)
//     {
//         cout << "in file_creat: allocate inode failed" << endl;
//         return -1;
//     }
//     /* 此inode要从堆中申请内存,不可生成局部变量(函数退出时会释放)
//  * 因为file_table数组中的文件描述符的inode指针要指向它.*/

//     if (new_file_inode == NULL)
//     {
//         cout << "file_create: sys_malloc for inode failded" << endl;
//         rollback_step = 1;
//         goto rollback;
//     }
//     inode_init(inode_no, new_file_inode); // 初始化i结点

//     /* 返回的是file_table数组的下标 */

//     if (fd_idx == -1)
//     {
//         cout << "exceed max open files" << endl;
//         rollback_step = 2;
//         goto rollback;
//     }

//     file_table[fd_idx].fd_inode = new_file_inode;
//     file_table[fd_idx].fd_pos = 0;
//     file_table[fd_idx].fd_flag = flag;
//     file_table[fd_idx].fd_inode->write_deny = false;

//     struct dir_entry new_dir_entry;
//     memset(&new_dir_entry, 0, sizeof(struct dir_entry));

//     create_dir_entry(filename, inode_no, FT_REGULAR, &new_dir_entry); // create_dir_entry只是内存操作不出意外,不会返回失败

//     /* 同步内存数据到硬盘 */
//     /* a 在目录parent_dir下安装目录项new_dir_entry, 写入硬盘后返回true,否则false */
//     if (!sync_dir_entry(parent_dir, &new_dir_entry, io_buf))
//     {
//         printk("sync dir_entry to disk failed\n");
//         rollback_step = 3;
//         goto rollback;
//     }

//     memset(io_buf, 0, 1024);
//     /* b 将父目录i结点的内容同步到硬盘 */
//     inode_sync(cur_part, parent_dir->inode, io_buf);

//     memset(io_buf, 0, 1024);
//     /* c 将新创建文件的i结点内容同步到硬盘 */
//     inode_sync(cur_part, new_file_inode, io_buf);

//     /* d 将inode_bitmap位图同步到硬盘 */
//     bitmap_sync(cur_part, inode_no, INODE_BITMAP);

//     /* e 将创建的文件i结点添加到open_inodes链表 */
//     list_push(&cur_part->open_inodes, &new_file_inode->inode_tag);
//     new_file_inode->i_open_cnts = 1;

//     sys_free(io_buf);
//     return pcb_fd_install(fd_idx);

// /*创建文件需要创建相关的多个资源,若某步失败则会执行到下面的回滚步骤 */
// rollback:
//     switch (rollback_step)
//     {
//     case 3:
//         /* 失败时,将file_table中的相应位清空 */
//         memset(&file_table[fd_idx], 0, sizeof(struct file));
//     case 2:
//         sys_free(new_file_inode);
//     case 1:
//         /* 如果新文件的i结点创建失败,之前位图中分配的inode_no也要恢复 */
//         bitmap_set(&cur_part->inode_bitmap, inode_no, 0);
//         break;
//     }
//     sys_free(io_buf);
//     return -1;
// }

int32_t inode_bitmap_alloc(struct current_partition *part)
{
    int32_t bit_idx = bitmap_scan(&part->inode_bitmap, 1);
    if (bit_idx == -1)
    {
        cout << "ERROR:no empty inode block" << endl;
        return FAIL;
    }
    bitmap_set(&part->inode_bitmap, bit_idx, 1);
    return bit_idx;
}

int32_t block_bitmap_alloc(struct current_partition *part)
{
    int32_t bit_idx = bitmap_scan(&part->block_bitmap, 1);
    if (bit_idx == -1)
    {
        cout << "ERROR:no empty block" << endl;
        return FAIL;
    }
    bitmap_set(&part->block_bitmap, bit_idx, 1);
    return (part->sb->data_start_lba + bit_idx);
}

void bitmap_sync(struct current_partition *part, uint32_t bit_idx, uint8_t btmp_type)
{
    uint32_t off_sec = bit_idx / 4096;        // 本i结点索引相对于位图的扇区偏移量
    uint32_t off_size = off_sec * BLOCK_SIZE; // 本i结点索引相对于位图的字节偏移量
    uint32_t sec_lba;
    uint8_t *bitmap_off = (uint8_t *)malloc(sizeof(bitmap_off));
    // cout << "coe" << endl;

    /* 需要被同步到硬盘的位图只有inode_bitmap和block_bitmap */
    switch (btmp_type)
    {
    case INODE_BITMAP:

        sec_lba = part->sb->inode_bitmap_lba + off_sec;
        bitmap_off = part->inode_bitmap.bits + off_size;
        break;

    case BLOCK_BITMAP:
        sec_lba = part->sb->block_bitmap_lba + off_sec;
        bitmap_off = part->block_bitmap.bits + off_size;
        break;
    }

    write_blocks_to_disk(part->v_disk, sec_lba, (char *)bitmap_off, 1);
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
    //         mount_partition(&test_d, 1);
    //         int bit_idx = inode_bitmap_alloc(cur_part);
    //         int btmp_type = block_bitmap_alloc(cur_part);
    //         if(bit_idx==-1 || btmp_type == -1)
    //         {
    //             cout<<"ERROR:inode or block is full"<<endl;
    //         }
    //         else
    //         {
    //             // cout<<"correct"<<endl;
    //             bitmap_sync(cur_part,bit_idx,btmp_type);
    //         }

    //     }
    // }
    // else{
    //     cout<<"first error"<<endl;
    // }
}
