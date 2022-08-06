#ifndef __FILE_H__
#define __FILE_H__

#include <fs/fs.h>
#include <lock/lock.h>
#include <ucore/types.h>
// pipe.h
#define PIPESIZE 512
// in-memory copy of an inode
#define major(dev) ((dev) >> 16 & 0xFFFF)
#define minor(dev) ((dev)&0xFFFF)
#define mkdev(m, n) ((uint)((m) << 16 | (n)))

// for seek seting
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

// map major device number to device functions.
struct device_handler {
    int64 (*read)(char *dst, int64 len, int to_user);
    int64 (*write)(char *src, int64 len, int from_user);
    int (*ioctl)(struct file* f, int is_user, int cmd, void *arg);
};

extern struct device_handler device_handler[];

struct pipe {
    char data[PIPESIZE];
    uint nread;    // number of bytes read
    uint nwrite;   // number of bytes written
    int readopen;  // read fd is still open
    int writeopen; // write fd is still open
    struct spinlock lock;
};

// file.h
struct file {
    enum {
        FD_NONE = 0,
        FD_PIPE,
        FD_INODE,
        FD_DEVICE
    } type;

    int ref; // reference count
    char readable;
    char writable;
    struct pipe *pipe; // FD_PIPE
    struct inode *ip;  // FD_INODE
    uint off;          // FD_INODE
    short major;       // FD_DEVICE
};

struct iovec {
    uint64 iov_base;
    uint64 iov_len;
};

// typedef char mail_t[256];
// #define MAX_MAIL_IN_BOX (16)
// struct mailbox {
//     mail_t *mailbuf; // 4KB, 16 mail
//     int valid[MAX_MAIL_IN_BOX];
//     int length[MAX_MAIL_IN_BOX];
//     int head;
//     struct spinlock lock;
// };

// int init_mailbox(struct mailbox *mb);
struct inode *create(char *path, short type, short major, short minor);
void fileinit();
void device_init();
void fileclose(struct file *f);
ssize_t fileread(struct file *f, void *dst_va, size_t len);
ssize_t filewrite(struct file *f, void *src_va, size_t len);
struct file *filealloc();
int fileopen(char *path, int flags);
int fileopenat(int dirfd, char *filename, int flags);
struct file *filedup(struct file *f);
int filestat(struct file *f, uint64 addr);
int getdents(struct file *f, char *buf, unsigned long len);
int filelink(struct file *oldfile, struct file *newfile);
int fileunlink(struct file *file);
void fileclear(struct file *f);
int filelseek(struct file *f, off_t offset, int whence);
int filepath(struct file *file, char *path);
int filerename(struct file *file, char *new_path);
int fileioctl(struct file *f, int cmd, void *arg);
#define FILE_MAX (128 * 16)

#define CONSOLE 1
#define CPU_DEVICE 2
#define MEM_DEVICE 3
#define PROC_DEVICE 4
#define NULL_DEVICE 5
#define ZERO_DEVICE 6
#define MOUNT_DEVICE 7
#define MEMINFO_DEVICE 8
#define RTC_DEVICE 9

#endif //!__FILE_H__