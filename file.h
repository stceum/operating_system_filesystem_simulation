#ifndef FILE_H
#define FILE_H
#include "pcb.h"
#include "fs.h"
#include "bitmap.h"
#include <stdint.h>

#define MAX_FILE_OPEN 16
// 文件结构 
typedef struct file
{
    uint32_t fd_pos;    //当前文件的偏移地址，0~文件大小-1
    uint32_t fd_flag;   //文件操作标识
    struct inode* fd_inode; //inode指针
}file;
// 标准输入输出描述符
enum std_fd {
   stdin_no,   // 0 标准输入
   stdout_no,  // 1 标准输出
   stderr_no   // 2 标准错误
};
// 位图类型 
enum bitmap_type {
   INODE_BITMAP,     // inode位图
   BLOCK_BITMAP	     // 块位图
};

/*文件表（文件结构数组），最多可以同时打开MAX_FILE_OPEN次文件，前三个成员预留给标准输入输出描述符*/
extern struct file file_table[MAX_FILE_OPEN];//

/*从文件表获取空闲的位置，成功返回空闲位置下标，失败返回-1*/
int32_t get_free_slot_in_file_table(void);

/*把文件表中空闲的位置放入pcb中*/
int32_t pcb_fd_install(int32_t file_table_free_slot);

/*都是输入分区partiton，返回分配的i结点或者扇区地址*/
int32_t inode_bitmap_alloc(struct current_partition* part);
int32_t block_bitmap_alloc(struct current_partition* part);

/*同步位图到硬盘，同步bit_idx处的512字节*/
void bitmap_sync(struct current_partition* part, uint32_t bit_idx, uint8_t btmp_type);

/*创建文件，输入父目录，文件名，文件标志，返回文件标识符*/
int32_t file_create(struct dir* parent_dir, char* filename, uint8_t flag);

/* 从文件file中读取count个字节写入buf, 返回读出的字节数,若到文件尾则返回-1 */
int32_t file_read(struct file* file, void* buf, uint32_t count);

/* 关闭文件 */
int32_t file_close(struct file *file);

/* 打开编号为inode_no的inode对应的文件,若成功则返回文件描述符,否则返回-1 */
int32_t file_open(uint32_t inode_no, uint8_t flag);


#endif