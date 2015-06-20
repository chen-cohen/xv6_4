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


struct proc* onlineProcessContext[NPROC];
int onlineProcessCounter = 0;


void
initProcess(struct inode *ip) {
  createProccesPool(onlineProcessContext);
  ip->size = onlineProcessCounter * sizeof(struct dirent);
}

void initDirecotryEntry(struct dirent *directoryEntry, struct inode *ip, char* command, uint commandLength){
  directoryEntry->inum = ip->inum;
  memmove(directoryEntry->name, command, commandLength);
}

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
    ip->major = PROCFS;
  }
}

int
procfsread(struct inode *ip, char *dst, int off, int n) {
  int file_descriptor;
  struct proc* proc;
  struct dirent directoryEntry;

  //
  if(ip->minor == ROOT_DIR){
    if (off == 0){
      initProcess(ip);
    }
    if(off / n == 0){
      initDirecotryEntry(directoryEntry, ip, ".", 2);
    }
  }
  return 0;
};

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
