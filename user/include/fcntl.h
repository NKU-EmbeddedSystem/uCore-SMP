#define O_RDONLY  0x000
#define O_WRONLY  0x001
#define O_RDWR    0x002

#define O_CREAT        0100
#define O_CREATE    O_CREAT
#define O_EXCL         0200
#define O_NOCTTY       0400
#define O_TRUNC       01000
#define O_APPEND      02000
#define O_NONBLOCK    04000
#define O_DSYNC      010000
#define O_SYNC     04010000
#define O_RSYNC    04010000
#define O_DIRECTORY 0200000
#define O_NOFOLLOW  0400000
#define O_CLOEXEC  02000000

#define AT_FDCWD -100