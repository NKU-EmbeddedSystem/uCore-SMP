#include "syscall_ids.h"
#include "syscall_impl.h"
#include <arch/riscv.h>
#include <arch/timer.h>
#include <file/fcntl.h>
#include <file/file.h>
#include <fs/fs.h>
#include <proc/proc.h>
#include <trap/trap.h>
#include <ucore/defs.h>
#include <utils/log.h>

char *syscall_names(int id)
{
    switch (id)
    {
    case SYS_getcwd:
        return "SYS_getcwd";
    case SYS_dup:
        return "SYS_dup";
    case SYS_dup3:
        return "SYS_dup3";
    case SYS_mknod:
        return "SYS_mknod";
    case SYS_mkdirat:
        return "SYS_mkdirat";
    case SYS_link:
        return "SYS_link";
    case SYS_unlink:
        return "SYS_unlink";
    case SYS_chdir:
        return "SYS_chdir";
    case SYS_openat:
        return "SYS_openat";
    case SYS_close:
        return "SYS_close";
    case SYS_pipe2:
        return "SYS_pipe2";
    case SYS_getdents64:
        return "SYS_getdents64";
    case SYS_read:
        return "SYS_read";
    case SYS_write:
        return "SYS_write";
    case SYS_fstat:
        return "SYS_fstat";
    case SYS_exit:
        return "SYS_exit";
    case SYS_wait4:
        return "SYS_wait4";
    case SYS_sched_yield:
        return "SYS_sched_yield";
    case SYS_kill:
        return "SYS_kill";
    case SYS_setpriority:
        return "SYS_setpriority";
    case SYS_getpriority:
        return "SYS_getpriority";
    case SYS_times:
        return "SYS_times";
    case SYS_uname:
        return "SYS_uname";
    case SYS_gettimeofday:
        return "SYS_gettimeofday";
    case SYS_settimeofday:
        return "SYS_settimeofday";
    case SYS_getpid:
        return "SYS_getpid";
    case SYS_getppid:
        return "SYS_getppid";
    case SYS_sysinfo:
        return "SYS_sysinfo";
    case SYS_brk:
        return "SYS_brk";
    case SYS_munmap:
        return "SYS_munmap";
    case SYS_clone:
        return "SYS_clone";
    case SYS_mmap:
        return "SYS_mmap";
    case SYS_execve:
        return "SYS_execve";
    case SYS_spawn:
        return "SYS_spawn";
    case SYS_mailread:
        return "SYS_mailread";
    case SYS_mailwrite:
        return "SYS_mailwrite";
    case SYS_sharedmem:
        return "SYS_sharedmem";
    case SYS_nanosleep:
        return "SYS_nanosleep";
    case SYS_rt_sigtimedwait:
        return "SYS_rt_sigtimedwait";
    case SYS_prlimit64:
        return "SYS_prlimit64";
    default:
        return "?";
    }
}

// dispatch syscalls to different functions
void syscall()
{
    struct proc *p = curr_proc();
    struct trapframe *trapframe = p->trapframe;
    uint64 id = trapframe->a7, ret;
    uint64 args[7] = {trapframe->a0, trapframe->a1, trapframe->a2, trapframe->a3, trapframe->a4, trapframe->a5, trapframe->a6};
    
    // ignore read and write so that shell command don't get interrupted
    if (id != SYS_write && id != SYS_read)
    {
        char *name=syscall_names(id);
        (void) name;
        tracecore("syscall %d (%s) args:%p %p %p %p %p %p %p", (int)id, name ,args[0] , args[1], args[2], args[3], args[4], args[5], args[6]);
    }
    pushtrace(id);
    switch (id) {
    case SYS_write:
        ret = sys_write(args[0], (void *)args[1], args[2]);
        break;
    case SYS_read:
        ret = sys_read(args[0], (void *)args[1], args[2]);
        break;
    case SYS_openat:
        ret = sys_openat(args[0], (char *)args[1], args[2], args[3]);
        break;
    case SYS_close:
        ret = sys_close(args[0]);
        break;
    case SYS_exit:
        ret = sys_exit(args[0]);
        break;
    case SYS_sched_yield:
        ret = sys_sched_yield();
        break;
    case SYS_setpriority:
        ret = sys_setpriority(args[0]);
        break;
    case SYS_getpriority:
        ret = sys_getpriority(args[0]);
        break;
    case SYS_getpid:
        ret = sys_getpid();
        break;
    case SYS_getppid:
        ret = sys_getppid();
        break;
    case SYS_dup:
        ret = sys_dup((int)args[0]);
        break;
    case SYS_dup3:
        ret = sys_dup3((int)args[0], (int)args[1], (int)args[2]);
        break;
    case SYS_clone:
        ret = sys_clone((unsigned long)args[0], (void *)args[1], (void *)args[2], (void *)args[3], (void *)args[4]);
        break;
    case SYS_execve:
        ret = sys_execve((char *)args[0], (char **)args[1], (char **)args[2]);
        break;
    case SYS_wait4:
        ret = sys_wait4((int)args[0], (int *)args[1], (int)args[2], (void *)args[3]);
        break;
    case SYS_times:
        ret = sys_times((struct tms *)args[0]);
        break;
    case SYS_uname:
        ret = sys_uname((struct utsname *)args[0]);
        break;
    case SYS_gettimeofday:
        ret = sys_gettimeofday((struct timeval *)args[0], (struct timezone *)args[1]);
        break;
    case SYS_mknod:
        ret = sys_mknod((char *)args[0], args[1], args[2]);
        break;
    case SYS_pipe2:
        ret = sys_pipe((int(*)[2])args[0], args[1]);
        break;
    case SYS_fstat:
        ret = sys_fstat((int)args[0], (void *)args[1]);
        break;
    case SYS_chdir:
        ret = sys_chdir((char *)args[0]);
        break;
    case SYS_mkdirat:
        ret = sys_mkdirat(args[0], (char *)args[1], args[2]);
        break;
    case SYS_link:
        ret = sys_link((char *)args[0], (char *)args[1]);
        break;
    case SYS_unlink:
        ret = sys_unlink((char *)args[0]);
        break;
    case SYS_sharedmem:
        ret = (uint64)sys_sharedmem((char *)args[0], args[1]);
        break;
    case SYS_getcwd:
        ret = (uint64)sys_getcwd((char *)args[0], args[1]);
        break;
    case SYS_getdents64:
        ret = sys_getdents((int)args[0], (void *)args[1], args[2]);
        break;
    case SYS_nanosleep:
        ret = sys_nanosleep((struct timeval *)args[0], (struct timeval *)args[1]);
        break;
    case SYS_brk:
        ret = sys_brk((void *)args[0]);
        break;
    case SYS_mmap:
        ret = sys_mmap((void *)args[0], args[1], args[2], args[3], args[4], args[5]);
        break;
    case SYS_munmap:
        ret = sys_munmap((void *)args[0], args[1]);
        break;
    case SYS_rt_sigtimedwait:
        ret = sys_dummy_success();
        break;
    case SYS_prlimit64:
        ret = sys_dummy_success();
        break;
    default:
        ret = -1;
        warnf("unknown syscall %d", (int)id);
    }
    if(id != SYS_execve)
        trapframe->a0 = ret; // return value
    if (id != SYS_write && id != SYS_read)
    {
        tracecore("syscall %d ret %l", (int)id, ret);
    }
    pushtrace(0x3033);
}