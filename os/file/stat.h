#if !defined(STAT_H)
#define STAT_H
#include <ucore/types.h>
struct stat {
    int dev;     // File system's disk device
    uint ino;    // Inode number
    short type;  // Type of file
    short nlink; // Number of links to file
    size_t size; // Size of file in bytes
};

enum
{
    DT_UNKNOWN = 0,         // Unknown file type
# define DT_UNKNOWN DT_UNKNOWN
    DT_FIFO = 1,            // Named pipe (fifo)
# define DT_FIFO DT_FIFO
    DT_CHR = 2,             // Character device
# define DT_CHR DT_CHR
    DT_DIR = 4,             // Directory
# define DT_DIR DT_DIR
    DT_BLK = 6,             // Block device
# define DT_BLK DT_BLK
    DT_REG = 8,             // Regular file
# define DT_REG DT_REG
    DT_LNK = 10,            // Symbolic link
# define DT_LNK DT_LNK
    DT_SOCK = 12,           // Socket
# define DT_SOCK DT_SOCK
    DT_WHT = 14             // Whiteout
# define DT_WHT DT_WHT
};

struct linux_dirent64 {
    uint64        d_ino;
    int64         d_off;
    unsigned short  d_reclen;
    unsigned char   d_type;
    char            d_name[];
};

#endif // STAT_H
