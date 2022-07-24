#if !defined(SYSCALL_IMPL_H)
#define SYSCALL_IMPL_H

#include <ucore/ucore.h>

#define MAP_FAILED ((void *) -1)

#define MAP_SHARED     0x01
#define MAP_PRIVATE    0x02
#define MAP_SHARED_VALIDATE 0x03
#define MAP_TYPE       0x0f
#define MAP_FIXED      0x10
#define MAP_ANON       0x20
#define MAP_ANONYMOUS  MAP_ANON
#define MAP_NORESERVE  0x4000
#define MAP_GROWSDOWN  0x0100
#define MAP_DENYWRITE  0x0800
#define MAP_EXECUTABLE 0x1000
#define MAP_LOCKED     0x2000
#define MAP_POPULATE   0x8000
#define MAP_NONBLOCK   0x10000
#define MAP_STACK      0x20000
#define MAP_HUGETLB    0x40000
#define MAP_SYNC       0x80000
#define MAP_FIXED_NOREPLACE 0x100000
#define MAP_FILE       0

#define MAP_HUGE_SHIFT 26
#define MAP_HUGE_MASK  0x3f
#define MAP_HUGE_16KB  (14 << 26)
#define MAP_HUGE_64KB  (16 << 26)
#define MAP_HUGE_512KB (19 << 26)
#define MAP_HUGE_1MB   (20 << 26)
#define MAP_HUGE_2MB   (21 << 26)
#define MAP_HUGE_8MB   (23 << 26)
#define MAP_HUGE_16MB  (24 << 26)
#define MAP_HUGE_32MB  (25 << 26)
#define MAP_HUGE_256MB (28 << 26)
#define MAP_HUGE_512MB (29 << 26)
#define MAP_HUGE_1GB   (30 << 26)
#define MAP_HUGE_2GB   (31 << 26)
#define MAP_HUGE_16GB  (34U << 26)

#define PROT_NONE      0
#define PROT_READ      1
#define PROT_WRITE     2
#define PROT_EXEC      4
#define PROT_GROWSDOWN 0x01000000
#define PROT_GROWSUP   0x02000000


struct stat;

struct linux_dirent64;

struct kstat;

struct tms;

struct utsname;

struct timeval;

struct timezone;

int sys_execve( char *pathname_va, char * argv_va[], char * envp_va[]);

int sys_exit(int status);

ssize_t sys_read(int fd, void *dst_va, size_t len);

ssize_t sys_write(int fd, void *src_va, size_t len);

pid_t sys_getpid(void);

pid_t sys_getppid();

//int sys_open( char *pathname_va, int flags);

int sys_openat(int dirfd, char *filename, int flags, int mode);

int sys_mknod( char *pathname_va, short major, short minor);

int sys_dup(int oldfd);

int sys_sched_yield(void);

pid_t sys_wait4(pid_t pid, int *wstatus_va, int options, void *rusage);

//int sys_mkdir(char *pathname_va);

int sys_mkdirat(int dirfd, char *path_va, unsigned int mode);

int sys_close(int fd);

pid_t sys_clone(unsigned long flags, void *child_stack, void *ptid, void *tls, void *ctid);

uint64 sys_times(struct tms *tms_va);

int sys_sleep(unsigned long long time_in_ms);

int sys_pipe(int (*pipefd_va)[2], int flags);

int sys_fstat(int fd, struct kstat *statbuf_va);

int sys_chdir(char *path_va);

int sys_link( char *oldpath_va,  char *newpath_va);

int sys_unlink( char *pathname_va);

int64 sys_setpriority(int64 priority);

int64 sys_getpriority();

void* sys_sharedmem(char* name_va, size_t len);

char * sys_getcwd(char *buf, size_t size);

int sys_dup3(int oldfd, int newfd, int flags);

int sys_getdents(int fd, struct linux_dirent64 *dirp64, unsigned long len);

int sys_uname(struct utsname *utsname_va);

int sys_gettimeofday(struct timeval *tv_va, struct timezone *tz_va);

int sys_nanosleep(struct timeval *req_va, struct timeval *rem_va);

int sys_brk(void* addr);

void *sys_mmap(void *start, size_t len, int prot, int flags, int fd, long off);

int sys_munmap(void *start, size_t len);

int sys_dummy_success(void);

int sys_dummy_failure(void);

#endif // SYSCALL_IMPL_H
