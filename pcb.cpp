#include "pcb.h"
#include <cstring>
#include <cstdlib>
using namespace std;

pcb *cur = (pcb*)malloc(sizeof(pcb));
char *cwd_buf = (char*)malloc(32);
char cur_user[64];
void init_pcb(pcb * p)
{
    memset(p->fd_table,-1,sizeof(p->fd_table));
}