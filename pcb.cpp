#include "pcb.h"
#include <cstring>
using namespace std;

void init_pcb(pcb * p)
{
    memset(p->fd_table,-1,sizeof(p->fd_table));
}