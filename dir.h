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

void open_root_dir(current_partition *part);

/* 在分区part上打开i结点为inode_no的目录并返回目录指针 */
struct dir *dir_open(struct current_partition *part, uint32_t inode_no);

/* 在part分区内的pdir目录内寻找名为name的文件或目录,
 * 找到后返回true并将其目录项存入dir_e,否则返回false */
bool search_dir_entry(struct current_partition *part, struct dir *pdir,
                      const char *name, struct dir_entry *dir_e);

/* 关闭目录 */
void dir_close(struct dir *dir);

/* 在内存中初始化目录项p_de */
void create_dir_entry(char *filename, uint32_t inode_no, uint8_t file_type, struct dir_entry *p_de);

/* 将目录项p_de写入父目录parent_dir中,io_buf由主调函数提供 */
bool sync_dir_entry(struct dir *parent_dir, struct dir_entry *p_de, void *io_buf);

/* 把分区part目录pdir中编号为inode_no的目录项删除 */
bool delete_dir_entry(struct current_partition *part, struct dir *pdir, uint32_t inode_no, void *io_buf);

/* 读取目录,成功返回1个目录项,失败返回NULL */
struct dir_entry *dir_read(struct dir *dir);

/* 判断目录是否为空 */
bool dir_is_empty(struct dir *dir);

/* 在父目录parent_dir中删除child_dir */
int32_t dir_remove(struct dir *parent_dir, struct dir *child_dir);




#endif