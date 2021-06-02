#include "disk.h"
#include "super_block.h"
#include "bitmap.h"
#include "list.h"
#include <stdint.h>

struct file
{
    uint32_t fd_pos;    //当前文件的偏移地址，0~文件大小-1
    
};
