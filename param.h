#define NPROC        64  // maximum number of processes
#define KSTACKSIZE 4096  // size of per-process kernel stack
#define NCPU          8  // maximum number of CPUs
#define NOFILE       16  // open files per process
#define NFILE       100  // open files per system
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         10  // maximum major device number
#define ROOTDEV       1  // device number of file system root disk
#define MAXARG       32  // max exec arguments
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define LOGSIZE      (MAXOPBLOCKS*3)  // max data sectors in on-disk log
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache
#define NUMBER_OF_INODES   200
#define MAGIC_NUMBER_OF_FILES       18
#define FIRST_TWO_FILES 2
#define ROOT_DIR 0
#define PID_DIR 1
#define FILE_DIR 2
#define BUFF_LENGTH 256