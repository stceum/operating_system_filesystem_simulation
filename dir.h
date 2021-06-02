#ifndef DIR_H
#define DIR_H

#include "inode.h"
#define MAX_FILENAME_LEN 16

// 目录 目录项 用于用户/程序查找文件 该 struct 仅放在内存中
typedef struct dir {
    struct inode* inode;  // 内存中保存的 inode 指针
    uint32_t dir_pos;	  // 记录在目录内的偏移
    uint8_t dir_buf[512]; // 目录的数据缓存
}dir;

enum file_type {
    FT_UNKNOWN, //其他
    FT_REGULAR, //文件
    FT_DIRCTORY //目录
};

typedef struct dir_entry {
    char filename[MAX_FILENAME_LEN];    // 文件名
    uint32_t i_no;                  // inode 指针
    file_type f_type;                   // 文件类型
}dir_entry;

#endif