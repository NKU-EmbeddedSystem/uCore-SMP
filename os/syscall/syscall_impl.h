#if !defined(SYSCALL_IMPL_H)
#define SYSCALL_IMPL_H

#include <ucore/ucore.h>

struct stat;

struct linux_dirent64;

struct kstat;

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

uint64 sys_time_ms();

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

#endif // SYSCALL_IMPL_H
