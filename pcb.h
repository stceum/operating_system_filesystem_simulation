#ifndef PCB_H
#define PCB_H
#include <stdint.h>
#define MAX_FILES_OPEN_PER_PROC 16
typedef struct pcb
{
    int32_t fd_table[MAX_FILES_OPEN_PER_PROC];  //进程打开的文件
}pcb;

void init_pcb(pcb * p);
#endif