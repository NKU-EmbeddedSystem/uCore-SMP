#if !defined(INODE_H)
#define INODE_H
#include <ucore/ucore.h>
#include <lock/lock.h>
#include <fatfs/ff.h>

#define NDIRECT 12
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

#define MAXPATH 128

struct inode {
    uint dev;  // Device number
    int ref;   // Reference count
    struct mutex lock;

    short type; // enum of T_FILE, T_DIR, T_DEVICE
    struct {
        short major;
        short minor;
    } device;
    FIL file;
    DIR dir;

    char path[MAXPATH]; // absolute path
};

struct device {
    int magic;
    int major;
    int minor;
};

#define DEVICE_MAGIC 'devx'

void inode_table_init();

//struct inode *iget(uint dev, uint inum);
struct inode *iget_root();
void iput(struct inode *ip);

struct inode * idup(struct inode *ip);

int readi(struct inode *ip, int user_dst, void *dst, uint off, uint n);
int writei(struct inode *ip, int user_src, void *src, uint off, uint n);

void ilock(struct inode *ip);
void iunlock(struct inode *ip);

struct inode *inode_by_name(char *path);
struct inode *inode_parent_by_name(char *path, char *name);

void itrunc(struct inode *ip);

struct inode *
dirlookup(struct inode *dp, char *name);

struct inode *
icreate(struct inode *dp, char *name, int type, int major, int minor);

void inode_test();

int igetdents(struct inode *dp, char *buf, unsigned long len);

#endif // INODE_H
