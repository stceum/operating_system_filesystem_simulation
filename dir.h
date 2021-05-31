#ifndef DIR_H
#define DIR_H
//目录 目录项 用于用户/程序查找文件
#include "inode.h"
#define MAX_FILENAME_LEN 16

struct dir {

};

struct dir_entry {
    char name[MAX_FILENAME_LEN];    // 文件名
    inode * inode_ptr;              // inode 指针
    file_type ft;                   // 文件类型
};

enum file_type {
    FT_UNKNOWN, //其他
    FT_REGULAR, //文件
    FT_DIRCTORY //目录
}

#endif