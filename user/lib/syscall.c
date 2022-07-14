#include <stddef.h>
#include <ucore.h>
#include <fcntl.h>
#include "syscall.h"

#include <ucore.h>
#include <ucore_syscall_ids.h>

int execv(const char *pathname, char *const argv[])
{
    return syscall(SYS_execve, pathname, argv, NULL);
}

int exec(const char *pathname)
{
    return syscall(SYS_execve, pathname, NULL, NULL);
}

void exit(int status)
{
    syscall(SYS_exit, status);
}
ssize_t read(int fd, void *dst, size_t len)
{
    return syscall(SYS_read, fd, dst, len);
}
ssize_t write(int fd, void *src, size_t len)
{
    return syscall(SYS_write, fd, src, len);
}

pid_t getpid(void)
{
    return syscall(SYS_getpid);
}

pid_t getppid(void)
{
    return syscall(SYS_getppid);
}

int open(const char *path, int flags)
{
    return syscall(SYS_openat, AT_FDCWD, path, flags, O_RDWR);
}

int mknod(const char *pathname, short major, short minor)
{
    return syscall(SYS_mknod, pathname, major, minor);
}

int dup(int oldfd)
{
    return syscall(SYS_dup, oldfd);
}

int sched_yield(void)
{
    return syscall(SYS_sched_yield);
}

pid_t waitpid(pid_t pid, int *wstatus,int n) //temporary
{
    return syscall(SYS_wait4, pid, wstatus, n, 0);
}

pid_t wait(int *wstatus)
{
    // return waitpid(-1, wstatus);//temporary
    return waitpid(-1, wstatus,0);//temporary
}

int mkdir(const char *pathname,unsigned int mode)
{
    return syscall(SYS_mkdirat, AT_FDCWD, pathname, mode);
}

int close(int fd)
{
    return syscall(SYS_close, fd);
}

pid_t fork(void)
{
    return syscall(SYS_clone, SIGCHLD, 0);
}

int sleep(unsigned long long time)
{
    TimeVal tv = {.sec = time, .usec = 0};
    if (syscall(SYS_nanosleep, &tv, &tv)) return tv.sec;
    return 0;
}

int pipe(int pipefd[2])
{
    return syscall(SYS_pipe2, pipefd, 0);
}

int fstat(int fd, struct kstat *statbuf)
{
    return syscall(SYS_fstat, fd, statbuf);
}

int stat(const char *pathname, struct kstat *statbuf)
{
    int fd;
    int r;

    fd = open(pathname, O_RDONLY);
    if (fd < 0)
        return -1;
    r = fstat(fd, statbuf);
    close(fd);
    return r;
}

int chdir(const char *path){
    return syscall(SYS_chdir, path);
}


int link(const char *oldpath, const char *newpath){
    return syscall(SYS_link, oldpath, newpath);

}

int unlink(const char *pathname){
    return syscall(SYS_unlink, pathname);
}

int64 setpriority(int64 priority){
    return syscall(SYS_setpriority, priority);
}

int64 getpriority(){
    return syscall(SYS_getpriority);
}

void* sharedmem(char* name, size_t len){
    return (void*) syscall(SYS_sharedmem, name, len);
}
// =============================================================
// =============================================================
// [NEW]

char *getcwd(char *buf, size_t size){
     return (char*)syscall(SYS_getcwd, buf, size);
}

int dup2(int old, int _new){
     return syscall(SYS_dup3, old, _new, 0);
}

int getdents(int fd, struct linux_dirent64 *dirp64, unsigned long len){
     return syscall(SYS_getdents64, fd, dirp64, len);
}

int mount(const char *special, const char *dir, const char *fstype, unsigned long flags, const void *data)
{
    return 0;
        // return syscall(SYS_mount, special, dir, fstype, flags, data);
}

int umount(const char *special)
{
    return 0;
        // return syscall(SYS_umount2, special, 0);
}

// int open(const char *path, int flags)
// {
//     return 0;
//     // return syscall(SYS_openat, AT_FDCWD, path, flags);
// }

int openat(int dirfd,const char *path, int flags)
{
    return syscall(SYS_openat, dirfd, path, flags, 0600);
}

int brk(void *addr)
{
    return syscall(SYS_brk, addr);
}

void *mmap(void *start, size_t len, int prot, int flags, int fd, off_t off)
{
    return 0;
    // return syscall(SYS_mmap, start, len, prot, flags, fd, off);
}

int munmap(void *start, size_t len)
{
    return 0;
    // return syscall(SYS_munmap, start, len);
}

pid_t clone(int (*fn)(void *arg), void *arg, size_t *stack, size_t stack_size, unsigned long flags)
{
    if (stack)
        stack += stack_size;

    return __clone(fn, stack, flags, NULL, NULL, NULL);
    //return syscall(SYS_clone, fn, stack, flags, NULL, NULL, NULL);
}

int execve(const char *name, char *const argv[], char *const argp[])
{
     return syscall(SYS_execve, name, argv, argp);
}

int times(void *mytimes)
{
    return syscall(SYS_times, mytimes);
}

int sys_get_time(TimeVal *ts, int tz)
{
    return syscall(SYS_gettimeofday, ts, tz);
}

int64 get_time()
{
    TimeVal time;
    int err = sys_get_time(&time, 0);
    if (err == 0)
    {
        return ((time.sec & 0xffff) * 1000 + time.usec / 1000);
    }
    else
    {
        return -1;
    }
}

int uname(void *buf)
{
    return syscall(SYS_uname, buf);
}