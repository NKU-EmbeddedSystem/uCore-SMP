#if !defined(INODE_H)
#define INODE_H
#include <ucore/ucore.h>
#include <lock/lock.h>
#include <fatfs/ff.h>

#define NDIRECT 12
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

#define MAXPATH 128

// for kstat::st_mode
#define S_IFMT     0170000   // bit mask for the file type bit field
#define S_IFSOCK   0140000   // socket
#define S_IFLNK    0120000   // symbolic link
#define S_IFREG    0100000   // regular file
#define S_IFBLK    0060000   // block device
#define S_IFDIR    0040000   // directory
#define S_IFCHR    0020000   // character device
#define S_IFIFO    0010000   // FIFO

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
    bool unlinked;      // has been unlinked
};

struct page_cache {
    struct mutex lock;
    struct inode *host;
    uint offset;
    uint dirty;
    uint valid;
    void *page;
};

struct device {
    int magic;
    int major;
    int minor;
};

struct symlink {
    int magic;
    char path[MAXPATH];
};

#define DEVICE_MAGIC 'devx'
#define SYMLINK_MAGIC 'slnk'

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

void print_inode(struct inode *ip);

struct inode *
dirlookup(struct inode *dp, char *name);

struct inode *
icreate(struct inode *dp, char *name, int type, int major, int minor);

void inode_test();



int igetdents(struct inode *dp, char *buf, unsigned long len);

int stati(struct inode *ip, struct kstat *st);

int ilink(struct inode *oldip, struct inode *newip);

int iunlink(struct inode *ip);

#endif // INODE_H
