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


#define CMDLINE 0
#define CWD 1
#define EXE 2
#define FDINFO 3
#define STATUS 4
int MAIN_INODE;
struct file *ofile[NOFILE];
int fd_count[NOFILE];
int open_files = 0;
struct proc*onlineProcesses[NPROC];
int onlineProcessContext = 0;
char *procfs_files[5] = {"cmdline", "cwd", "exe", "fdinfo", "status"};
extern int create_alive_procs(struct proc* alive_procs[NPROC]);
char* procstate[6] = {"UNUSED", "EMBRYO", "SLEEPING", "RUNNABLE", "RUNNING", "ZOMBIE"};

char* itoa(int i, char b[]) {
  char const digit[] = "0123456789";
  char* p = b;
  if (i < 0) {
    *p++ = '-';
    i *= -1;
  }
  int shifter = i;
  do { //Move to where representation ends
    ++p;
    shifter = shifter / 10;
  } while (shifter);
  *p = '\0';
  do { //Move back, inserting digits as u go
    *--p = digit[i % 10];
    i = i / 10;
  } while (i);
  return b;
}

/**
 * Add the requested info as a string to buf.
 * buf has to be at least 40 chars long
 */
void
get_fd_info(char* buf, struct file*file) {
  //cprintf("get_fd_info\n");
  int index = 0;
  if (file->type == FD_NONE) {
    strncpy(buf, "FD_NONE, ", strlen("FD_NONE, "));
    index += strlen("FD_NONE, ");
  } else if (file->type == FD_PIPE) {
    strncpy(buf, "FD_PIPE, ", strlen("FD_PIPE, "));
    index += strlen("FD_PIPE, ");
  } else if (file->type == FD_INODE) {
    strncpy(buf, "FD_INODE, ", strlen("FD_INODE, "));
    index += strlen("FD_INODE, ");
  }

  buf[index] = '\n';
  index++;

//    cprintf("1:file off %d\n", file->off);

  itoa(file->off, buf+index);
  int size=strlen(buf+index);
//     cprintf("size :%d\n",size);
  buf[index + size] = '\n'; // replace '\0' returned from itoa with '\n'
  index += size+1 ;

  // cprintf("2: %s\n\n", buf);

  if (file->readable) {
    //cprintf("read\n");
    strncpy(buf + index, "READABLE ", strlen("READABLE "));
    index += strlen("READABLE ");
  }
  if (file->writable) {
    // cprintf("write\n");
    strncpy(buf + index, "WRITEABLE ", strlen("WRITEABLE "));
    index += strlen("WRITEABLE ");
  }
  buf[index] = '\n';
  buf[index + 1] = '\0';

  //  cprintf("3: %s\n\n", buf);
}

int
procfsisdir(struct inode *ip) {

  if (ip->minor == PROCFS_MAIN|| ip->minor == PROCFS_PID || (ip->minor == PROCFS_FILE && (ip->inum % 5 == CWD || ip->inum % 5 == FDINFO))) {
    // cprintf("found procfs dir\n");
    return 1;
  }
  return 0;
}

void
procfsiread(struct inode* dp, struct inode *ip) {


  if(ip->inum>200)
  {
    ip->flags |= I_VALID;
    ip->nlink = 1;
    ip->type = T_DEV;
    ip->major = PROCFS;
    if (ip->inum == 18)
      ip->minor = PROCFS_MAIN;
    if (dp->minor == PROCFS_MAIN) {
      ip->minor = PROCFS_PID;
      dp=idup(dp);
    } else if (dp->minor == PROCFS_PID)
    {
      dp=idup(dp);
      ip->minor = PROCFS_FILE;
    }
    if (dp->minor == PROCFS_FILE)
    {
      // cprintf("duping");

      ip->minor = PROCFS_FDINFO;

    }
  }
}


void initProcessPool(struct inode *ip){
  MAIN_INODE=ip->inum;
  onlineProcessContext = createOnlineProcess(onlineProcesses);
  ip->size = (onlineProcessContext + 2) * sizeof (struct dirent);
}

void updateAndMemMove(struct inode *ip, struct dirent de, char* command, uint commandLength) {
  de.inum = ip->inum;
  memmove(de.name, command, commandLength);
}


int
procfsread(struct inode *ip, char *dst, int off, int n) {
  struct dirent de;
  struct proc* p;
  int i;
  char buffer[100];
  int fd;
  if (ip->minor == PROCFS_MAIN) {
    if (off == 0)//first time use
    {
      initProcessPool(ip);
    }
    if (off / n == 0) {
      updateAndMemMove(ip, de, ".",2);
    } else
    if (off / n == 1) {
      de.inum = ROOTINO;
      memmove(de.name, "..", 3);
    } else if (off / n < onlineProcessContext + 2) {
      p = onlineProcesses[off / sizeof (struct dirent) - 2];
      de.inum = p->pid + 200;
      itoa(p->pid, de.name);
    } else {
      return 0;
    }
    memmove(dst, &de, sizeof (struct dirent));
    return sizeof (struct dirent);

  }
  if (ip->minor == PROCFS_PID) {
    int file_type = off / n;
    if (off == 0)
      ip->size = 7 * n;
    if (file_type > 6)
      return 0;
    if (file_type == 0) {
      de.inum = ip->inum;
      memmove(de.name, ".", 2);
    } else
    if (file_type == 1) {
      de.inum = MAIN_INODE;
      memmove(de.name, "..", 3);
    } else {
      if (file_type != 3 && file_type != 4)
        de.inum = ip->inum * 10 + file_type - 2;
      else {
        for (i = 0; i < onlineProcessContext; i++) {
          p = onlineProcesses[i];
          if (p->pid == (ip->inum - 200)) {
            if (file_type == 3) {
              de.inum = p->cwd->inum;
//                            cprintf("inummm :%d", de.inum);
            } else {
//                            cprintf("inummm :%d", de.inum);

              de.inum = p->exe_num;
            }
            break;
          }

        }
      }



      memmove(de.name, procfs_files[file_type - 2], strlen(procfs_files[file_type - 2]) + 1);
    }
    memmove(dst, &de, sizeof (struct dirent));
    return sizeof (struct dirent);

  }
  if (ip->minor == PROCFS_FILE) {
    //  cprintf("\nreading ip->num%d\n", ip->inum);
    int op_number = ip->inum % 5;
    for (i = 0; i < onlineProcessContext; i++) {
      p = onlineProcesses[i];
      if (p->pid == (((ip->inum - (op_number)) / 10) - 200)) {

        break;
      }

    }
    //   cprintf("op_number%d,ip->num%d\n",op_number,ip->inum);
    switch (op_number) {
      case CMDLINE:
        if (off == 0) {
          //
          int size = strlen(p->cmdline);
          memset(dst, 0, n);
          memmove(dst, p->cmdline, size + 1);
          dst[size] = '\n';
          return size + 1;
        } else {
          return 0;
        }
            break;
      case CWD:
        memmove(dst, p->cwd, sizeof (struct dirent));
            cprintf("cwd cmd\n");
            return n;
            break;

      case EXE:
        cprintf("cwd exec\n");
            ip->flags |= ~I_VALID;
            return n;
            break;
      case FDINFO:
        if (off == 0) {
          open_files = 0;
          for (fd = 0; fd < NOFILE; fd++) {
            if (p->ofile[fd]) {
              ofile[open_files] = p->ofile[fd];
              //    cprintf("found fd :%d for file %p\n",fd,p->ofile[fd]);
              fd_count[open_files++] = fd;
            }
          }
        }

            int real_offset = off / n;
            if (real_offset == 0) {
              de.inum = ip->inum;
              memmove(de.name, ".", 2);
            } else
            if (real_offset == 1) {

              de.inum = ip->inum/10;
              //  cprintf("de.inum:%d",de.inum);
              memmove(de.name, "..", 3);
            }

            else if (real_offset > open_files + 1)// real_offset starts from 0
            {
              return 0;
            }
            else {

              //  cprintf("found fd :%d for file %p\n",fd_count[real_offset - 2],ofile[real_offset - 2]);
              int num=ip->inum/10;
              de.inum = num * 100 + fd_count[real_offset - 2];
              itoa(fd_count[real_offset - 2], buffer);

              memmove(de.name, buffer, strlen(buffer) + 1);
//                    cprintf("filename :%s\n",de.name);
            }
            memmove(dst, &de, sizeof (struct dirent));
//            cprintf("entring");
            return sizeof (struct dirent);

      case STATUS:
        if (off == 0) {
          //
          int size = strlen(procstate[p->state]);
          memset(dst, 0, n);

          memmove(dst, procstate[p->state], size + 1);
          dst[size] = '\n';
          itoa(p->sz, buffer);
          int proc_sz_size = strlen(buffer);
          memmove(dst + size + 1, buffer, proc_sz_size + 1);
          dst[size + proc_sz_size] = '\n';
          return size + proc_sz_size + 1;
        } else {
          return 0;
        }

    }
  }
  if (ip->minor == PROCFS_FDINFO) {//read fd info
    if(off == 0){
      for (i = 0; i < onlineProcessContext; i++) {
        p = onlineProcesses[i];
        if (p->pid == ((ip->inum / 100) - 200)) {
          break;
        }
      }
      //   cprintf("number of open files %d ipnum is %d\n",open_files,ip->inum);
      int fd_num = ip->inum % 10;

      struct file * f = p->ofile[fd_num]; //type, position, flags
      //cprintf("found fd :%d for file %p\n",fd_num,f);

      get_fd_info(buffer, f);
      memmove(dst , buffer, strlen(buffer) + 1);
      return strlen(buffer);
    }
    else{
      return 0;
    }
  }



  return 0;
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
