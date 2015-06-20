#include "types.h"
#include "stat.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

int 
procfsisdir(struct inode *ip) {
  return (ip->type != T_DIR) ? 0 : 1;
}

void 
procfsiread(struct inode* dp, struct inode *ip) {
  if (ip->inum > NUMBER_OF_INODES){
    ip->type = T_DEV;
    ip->flags = ip->flags || I_VALID;
    ip->nlink = 1;
  }
}

int
procfsread(struct inode *ip, char *dst, int off, int n) {
  return 0;
}

int
procfswrite(struct inode *ip, char *buf, int n)
{
  return 0;
}

void
procfsinit(void)
{
  devsw[PROCFS].isdir = procfsisdir;
  devsw[PROCFS].iread = procfsiread;
  devsw[PROCFS].write = procfswrite;
  devsw[PROCFS].read = procfsread;
}
