#ifndef INODE_H
#define INODE_H

// index node
// 目录和文件在文件系统中对于inode来说没有区别
// inode 统一管理 区分二者通过看inode指向数据还是指向目录中的目录项

struct  inode
{
    /* data */
};


#endif