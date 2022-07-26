#if !defined(UCORE_SYSCALL_H)
#define UCORE_SYSCALL_H
#include "stddef.h"

extern int __clone(int (*func)(void *), void *stack, int flags, void *arg, ...);

int execv(const char *pathname, char *const argv[]);

int exec(const char *pathname);

void exit(int status);

ssize_t read(int fd, void *dst, size_t len);

ssize_t write(int fd, void *src, size_t len);

pid_t getpid(void);

pid_t getppid(void);

int open(const char *pathname, int flags);

int mknod(const char *pathname, short major, short minor);

int dup(int oldfd);

int sched_yield(void);

pid_t waitpid(pid_t pid, int *wstatus,int n);//temporary

pid_t wait(int *wstatus);

int mkdir(const char *pathname,unsigned int mode);//temporary

int close(int fd);

pid_t fork(void);

uint64 time_ms();

int sleep(unsigned long long time_in_ms);

int pipe(int pipefd[2]);

int fstat(int fd, struct kstat *statbuf);

int stat(const char *pathname, struct kstat *statbuf);

int chdir(const char *path);

int link(char *old_path, char *new_path);

int unlink(char *path);

int64 setpriority(int64 priority);

int64 getpriority();

void* sharedmem(char* name, size_t len);

//////////////////////[NEW] 

char *getcwd(char *, size_t);

int dup2(int old, int _new);

int getdents(int fd, struct linux_dirent64 *dirp64, unsigned long len);

int mount(const char *special, const char *dir, const char *fstype, unsigned long flags, const void *data);

int umount(const char *special);

int openat(int fd, const char *pathname, int flags);

int64 get_time();

int brk(void *addr);

void *mmap(void *start, size_t len, int prot, int flags, int fd, off_t off);

int munmap(void *start, size_t len);

pid_t clone(int (*fn)(void *arg), void *arg, size_t *stack, size_t stack_size, unsigned long flags);

int execve(const char *name, char *const argv[], char *const argp[]);

int waitpid(int pid, int *code, int options);

int times(void *mytimes);

int uname(void *buf);

#endif // UCORE_SYSCALL_H
