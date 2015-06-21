#include <stddef.h>
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


struct proc*onlineProcesses[NPROC];
int onlineProcessContext = 0;
extern int createOnlineProcess(struct proc* alive_procs[NPROC]);
int globalNode = 0;

char *strrev(char *str)
{
  char *p1, *p2;

  if (! str || ! *str)
    return str;
  for (p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2)
  {
    *p1 ^= *p2;
    *p2 ^= *p1;
    *p1 ^= *p2;
  }
  return str;
}

void convertToChar(int x, char buffer[], int radix)
{
  int i = 0;
  int n = x;
  int s = x;
  while (n > 0)
  {
    s = n%radix;
    n = n/radix;
    buffer[i++] = '0' + s;
  }
  buffer[i] = '\0';
  strrev(buffer);
}

int
procfsisdir(struct inode *ip) {
  return (ip->type != T_DIR) ? 0 : 1;
}

void
procfsiread(struct inode* dp, struct inode *ip) {


  if(ip->inum>NUMBER_OF_INODES)
  {
    ip->flags |= I_VALID;
    ip->nlink = 1;
    ip->type = T_DEV;
    ip->major = PROCFS;
    if (ip->inum == MAGIC_NUMBER_OF_FILES)
      ip->minor = ROOT_DIR;
  }
}



void initProcessPool(struct inode *ip){
  globalNode=ip->inum;
  onlineProcessContext = createOnlineProcess(onlineProcesses);
  ip->size = (onlineProcessContext + FIRST_TWO_FILES) * sizeof (struct dirent);
}

void updateAndMemMove(struct inode *ip, struct dirent de, char* command, uint commandLength, uint inodeNumber) {
  de.inum = inodeNumber;
  memmove(de.name, command, commandLength);
}


void update(struct proc *p, int off, struct dirent de){
  p = onlineProcesses[off / sizeof(struct dirent) - FIRST_TWO_FILES];
  de.inum = p->pid + NUMBER_OF_INODES;
  convertToChar(p->pid, de.name, 10);
}


int
procfsread(struct inode *ip, char *dst, int off, int n) {
  struct dirent de;
  char stringBuff[BUFF_LENGTH];
  struct proc *p;


  if (ip->minor == ROOT_DIR) {
    if (off == 0) {
      initProcessPool(ip);
    }
    p = NULL;

    int determineNum = off / n;

    switch (determineNum) {
      case 0:
        updateAndMemMove(ip, de, ".", 2, ip->inum);
            break;
      case 1:
        updateAndMemMove(ip, de, "..", 3, 1);
            break;
      default:
        (off / n < onlineProcessContext + FIRST_TWO_FILES) ? update(p, off, de) : 0;
            break;
    }
    memmove(dst, &de, sizeof(struct dirent));
    return sizeof(struct dirent);

  }
}

int
procfswrite(struct inode *ip, char *buf, int n) {
  return 0;
}

void
procfsinit(void) {
  devsw[PROCFS].isdir = procfsisdir;
  devsw[PROCFS].iread = procfsiread;
  devsw[PROCFS].write = procfswrite;
  devsw[PROCFS].read = procfsread;
}
