#include "pcb.h"
#include <cstring>
#include <cstdlib>
using namespace std;

pcb *cur = (pcb *)malloc(sizeof(pcb));
void init_pcb(pcb * p)
{
    memset(p->fd_table,-1,sizeof(p->fd_table));
}