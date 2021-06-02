#ifndef INODE_H
#define INODE_H

#include <stdint.h>
#include "list.h"

// index node
// 目录和文件在文件系统中对于inode来说没有区别
// inode 统一管理 区分二者通过看inode指向数据还是指向目录中的目录项
typedef struct inode
{
    uint32_t i_no;      // inode 编号，即该 inode 在 inode 数组中的下标

    /* 当此inode是文件时,i_size是指文件大小,
     * 若此inode是目录,i_size是指该目录下所有目录项大小之和
     * 单位: 字节
     */
    uint32_t i_size;

    uint32_t i_open_counts;   // 记录此文件被打开的次数
    bool write_deny;	      // 写文件不能并行,进程写文件前检查此标识

    /* i_block[0-11]是直接块, i_block[12]用来存储一级间接块指针 */
    uint32_t i_block[13];
    list_elem inode_tag;
}inode;


#endif