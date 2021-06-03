#ifndef PCB_H
#define PCB_H
#include <stdint.h>
#define MAX_FILES_OPEN_PER_PROC 16
typedef struct pcb
{
    uint32_t cwd_inode_nr; // 进程所在的工作目录的 inode 编号
    int32_t fd_table[MAX_FILES_OPEN_PER_PROC];  //进程打开的文件
}pcb;
extern pcb *cur;
void init_pcb(pcb * p);
#endif