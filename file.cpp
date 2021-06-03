#include "file.h"
#include "fs.h"
#include "bitmap.h"
#include "inode.h"
#include "pcb.h"
#include "dir.h"
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
int32_t file_create(struct dir *parent_dir, char *filename, uint8_t flag)
{
    /* 后续操作的公共缓冲区 */
    void *io_buf = malloc(1024);
    if (io_buf == NULL)
    {
        cout << "in file_creat: sys_malloc for io_buf failed" << endl;
        return FAIL;
    }
    int fd_idx = get_free_slot_in_file_table();
    struct inode *new_file_inode = (struct inode *)malloc(sizeof(struct inode));
    uint8_t rollback_step = 0; // 用于操作失败时回滚各资源状态

    /* 为新文件分配inode */
    int32_t inode_no = inode_bitmap_alloc(cur_part);
    if (inode_no == -1)
    {
        cout << "in file_creat: allocate inode failed" << endl;
        return FAIL;
    }
    /* 此inode要从堆中申请内存,不可生成局部变量(函数退出时会释放)
 * 因为file_table数组中的文件描述符的inode指针要指向它.*/

    if (new_file_inode == NULL)
    {
        cout << "file_create: sys_malloc for inode failded" << endl;
        rollback_step = 1;
        goto rollback;
    }
    inode_init(inode_no, new_file_inode); // 初始化i结点

    /* 返回的是file_table数组的下标 */

    if (fd_idx == -1)
    {
        cout << "exceed max open files" << endl;
        rollback_step = 2;
        goto rollback;
    }

    file_table[fd_idx].fd_inode = new_file_inode;
    file_table[fd_idx].fd_pos = 0;
    file_table[fd_idx].fd_flag = flag;
    file_table[fd_idx].fd_inode->write_deny = false;

    struct dir_entry new_dir_entry;
    memset(&new_dir_entry, 0, sizeof(struct dir_entry));

    create_dir_entry(filename, inode_no, FT_REGULAR, &new_dir_entry); // create_dir_entry只是内存操作不出意外,不会返回失败

    /* 同步内存数据到硬盘 */
    /* a 在目录parent_dir下安装目录项new_dir_entry, 写入硬盘后返回true,否则false */
    if (!sync_dir_entry(parent_dir, &new_dir_entry, io_buf))
    {
        cout << "sync dir_entry to disk failed" << endl;
        rollback_step = 3;
        goto rollback;
    }

    memset(io_buf, 0, 1024);
    /* b 将父目录i结点的内容同步到硬盘 */
    inode_sync(cur_part, parent_dir->inode, io_buf);

    memset(io_buf, 0, 1024);
    /* c 将新创建文件的i结点内容同步到硬盘 */
    inode_sync(cur_part, new_file_inode, io_buf);

    /* d 将inode_bitmap位图同步到硬盘 */
    bitmap_sync(cur_part, inode_no, INODE_BITMAP);

    /* e 将创建的文件i结点添加到open_inodes链表 */
    list_push(&cur_part->open_inodes, &new_file_inode->inode_tag);
    new_file_inode->i_open_counts = 1;

    free(io_buf);
    return pcb_fd_install(fd_idx);

    /*创建文件需要创建相关的多个资源,若某步失败则会执行到下面的回滚步骤 */
rollback:
    switch (rollback_step)
    {
    case 3:
        /* 失败时,将file_table中的相应位清空 */
        memset(&file_table[fd_idx], 0, sizeof(struct file));
    case 2:
        free(new_file_inode);
    case 1:
        /* 如果新文件的i结点创建失败,之前位图中分配的inode_no也要恢复 */
        bitmap_set(&cur_part->inode_bitmap, inode_no, 0);
        break;
    }
    free(io_buf);
    return FAIL;
}

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
}

int32_t file_open(uint32_t inode_no, uint8_t flag)
{
    int fd_idx = get_free_slot_in_file_table();
    if (fd_idx == -1)
    {
        cout << "exceed max open files" << endl;
        return FAIL;
    }
    file_table[fd_idx].fd_inode = inode_open(cur_part, inode_no);
    file_table[fd_idx].fd_pos = 0; // 每次打开文件,要将fd_pos还原为0,即让文件内的指针指向开头
    file_table[fd_idx].fd_flag = flag;
    bool *write_deny = &file_table[fd_idx].fd_inode->write_deny;

    // if (flag & O_WRONLY || flag & O_RDWR)
    // { // 只要是关于写文件,判断是否有其它进程正写此文件
    //     // 若是读文件,不考虑write_deny
    //     /* 以下进入临界区前先关中断 */
    //     enum intr_status old_status = intr_disable();
    //     if (!(*write_deny))
    //     {                                // 若当前没有其它进程写该文件,将其占用.
    //         *write_deny = true;          // 置为true,避免多个进程同时写此文件
    //         intr_set_status(old_status); // 恢复中断
    //     }
    //     else
    //     { // 直接失败返回
    //         intr_set_status(old_status);
    //         printk("file can`t be write now, try again later\n");
    //         return -1;
    //     }
    // } // 若是读文件或创建文件,不用理会write_deny,保持默认
    return pcb_fd_install(fd_idx);
}

/* 关闭文件 */
int32_t file_close(file *f)
{
    if (f == NULL)
    {
        return -1;
    }
    f->fd_inode->write_deny = false;
    inode_close(f->fd_inode);
    f->fd_inode = NULL; // 使文件结构可用
    return 0;
}

/* 从文件file中读取count个字节写入buf, 返回读出的字节数,若到文件尾则返回-1 */
int32_t file_read(struct file *file, void *buf, uint32_t count)
{
    uint8_t *buf_dst = (uint8_t *)buf;
    uint32_t size = count, size_left = size;

    /* 若要读取的字节数超过了文件可读的剩余量, 就用剩余量做为待读取的字节数 */
    if ((file->fd_pos + count) > file->fd_inode->i_size)
    {
        size = file->fd_inode->i_size - file->fd_pos;
        size_left = size;
        if (size == 0)
        { // 若到文件尾则返回-1
            return FAIL;
        }
    }

    uint8_t *io_buf = (uint8_t *)malloc(BLOCK_SIZE);
    if (io_buf == NULL)
    {
        cout << "file_read: malloc for io_buf failed" << endl;
    }
    uint32_t *all_blocks = (uint32_t *)malloc(BLOCK_SIZE + 48); // 用来记录文件所有的块地址
    if (all_blocks == NULL)
    {
        cout << "file_read: malloc for all_blocks failed" << endl;
        return FAIL;
    }

    uint32_t block_read_start_idx = file->fd_pos / BLOCK_SIZE;        // 数据所在块的起始地址
    uint32_t block_read_end_idx = (file->fd_pos + size) / BLOCK_SIZE; // 数据所在块的终止地址
    uint32_t read_blocks = block_read_start_idx - block_read_end_idx; // 如增量为0,表示数据在同一扇区
    if (!(block_read_start_idx < 139 && block_read_end_idx < 139))
    {
        cout << "ERROR:!(block_read_start_idx < 139 && block_read_end_idx < 139)" << endl;
        return FAIL;
    }

    int32_t indirect_block_table; // 用来获取一级间接表地址
    uint32_t block_idx;           // 获取待读的块地址

    /* 以下开始构建all_blocks块地址数组,专门存储用到的块地址(本程序中块大小同扇区大小) */
    if (read_blocks == 0)
    { // 在同一扇区内读数据,不涉及到跨扇区读取
        if (!(block_read_end_idx == block_read_start_idx))
        {
            cout << "ERROR:!(block_read_end_idx == block_read_start_idx)" << endl;
            return FAIL;
        }
        if (block_read_end_idx < 12)
        { // 待读的数据在12个直接块之内
            block_idx = block_read_end_idx;
            all_blocks[block_idx] = file->fd_inode->i_block[block_idx];
        }
        else
        { // 若用到了一级间接块表,需要将表中间接块读进来
            indirect_block_table = file->fd_inode->i_block[12];
            read_blocks_from_disk_unsafe(cur_part->v_disk, indirect_block_table, (char *)all_blocks + 12, 1);
        }
    }
    else
    {   // 若要读多个块
        /* 第一种情况: 起始块和终止块属于直接块*/
        if (block_read_end_idx < 12)
        { // 数据结束所在的块属于直接块
            block_idx = block_read_start_idx;
            while (block_idx <= block_read_end_idx)
            {
                all_blocks[block_idx] = file->fd_inode->i_block[block_idx];
                block_idx++;
            }
        }
        else if (block_read_start_idx < 12 && block_read_end_idx >= 12)
        {
            /* 第二种情况: 待读入的数据跨越直接块和间接块两类*/
            /* 先将直接块地址写入all_blocks */
            block_idx = block_read_start_idx;
            while (block_idx < 12)
            {
                all_blocks[block_idx] = file->fd_inode->i_block[block_idx];
                block_idx++;
            }
            if (!(file->fd_inode->i_block[12] != 0))
            { // 确保已经分配了一级间接块表
                cout << "ERROR:!(file->fd_inode->i_block[12] != 0)" << endl;
                return FAIL;
            }

            /* 再将间接块地址写入all_blocks */
            indirect_block_table = file->fd_inode->i_block[12];
            read_blocks_from_disk_unsafe(cur_part->v_disk, indirect_block_table, (char *)all_blocks + 12, 1); // 将一级间接块表读进来写入到第13个块的位置之后
        }
        else
        {
            /* 第三种情况: 数据在间接块中*/
            if (!(file->fd_inode->i_block[12] != 0))
            { // 确保已经分配了一级间接块表
                cout << "ERROR:!(file->fd_inode->i_block[12] != 0)" << endl;
                return FAIL;
            }
            indirect_block_table = file->fd_inode->i_block[12];                                               // 获取一级间接表地址
            read_blocks_from_disk_unsafe(cur_part->v_disk, indirect_block_table, (char *)all_blocks + 12, 1); // 将一级间接块表读进来写入到第13个块的位置之后
        }
    }

    /* 用到的块地址已经收集到all_blocks中,下面开始读数据 */
    uint32_t sec_idx, sec_lba, sec_off_bytes, sec_left_bytes, chunk_size;
    uint32_t bytes_read = 0;
    while (bytes_read < size)
    { // 直到读完为止
        sec_idx = file->fd_pos / BLOCK_SIZE;
        sec_lba = all_blocks[sec_idx];
        sec_off_bytes = file->fd_pos % BLOCK_SIZE;
        sec_left_bytes = BLOCK_SIZE - sec_off_bytes;
        chunk_size = size_left < sec_left_bytes ? size_left : sec_left_bytes; // 待读入的数据大小

        memset(io_buf, 0, BLOCK_SIZE);
        read_blocks_from_disk_unsafe(cur_part->v_disk, sec_lba, (char *)io_buf, 1);
        memcpy(buf_dst, io_buf + sec_off_bytes, chunk_size);

        buf_dst += chunk_size;
        file->fd_pos += chunk_size;
        bytes_read += chunk_size;
        size_left -= chunk_size;
    }
    free(all_blocks);
    free(io_buf);
    return bytes_read;
}

// int main()
// {
    // int32_t fd_index = get_free_slot_in_file_table();
    // if(fd_index!=FAIL){
    //     int local_fd_idx = pcb_fd_install(fd_index);
    //     if(local_fd_idx != FAIL){
    //         virtual_disk test_d = read_disk((char*)"test_disk");
    //         // partition_format(&test_d, 1);
    //         fs_init(&test_d);
    //         mount_partition(&test_d, 1);
    //         int bit_idx = inode_bitmap_alloc(cur_part);
    //         int btmp_type = block_bitmap_alloc(cur_part);
    //         if(bit_idx==FAIL || btmp_type == FAIL)
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
// }
