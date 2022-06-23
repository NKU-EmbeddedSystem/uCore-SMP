#if !defined(UCORE_SYSCALL_IDS_H)
#define UCORE_SYSCALL_IDS_H

#define SYS_getcwd 17 // todo
#define SYS_dup 23  // todo
#define SYS_dup3 24 // new
#define SYS_mknod 33 
#define SYS_mkdir 34 // SYS_mkdirat
#define SYS_unlinkat 35 // new ?-> SYS_unlink 38
#define SYS_link 37 // SYS_linkat
#define SYS_unlink 38 // ?
#define SYS_umount2 39 // todo
#define SYS_mount 40 // todo
#define SYS_chdir 49 // todo
#define SYS_open 56 // todo
#define SYS_close 57 // todo
#define SYS_pipe 59 // todo
#define SYS_getdents64 61 // new 
#define SYS_read 63 // todo
#define SYS_write 64 // todo
#define SYS_fstat 80 // todo
#define SYS_exit 93 // todo
#define SYS_waitpid 95
#define SYS_nanosleep 101 // new
#define SYS_sched_yield 124 // todo
#define SYS_kill 129
#define SYS_setpriority 140
#define SYS_getpriority 141
#define SYS_time_ms 153 // todo
#define SYS_uname 160 // new
#define SYS_gettimeofday 169 // todo
#define SYS_settimeofday 170
#define SYS_getpid 172
#define SYS_getppid 173
#define SYS_sysinfo 179
#define SYS_brk 214 // todo
#define SYS_munmap 215 // todo
#define SYS_fork 220 // todo
#define SYS_execve 221 // new
#define SYS_mmap 222 // todo
#define SYS_wait4 260 // new
#define SYS_execv 281
#define SYS_sharedmem 282
#define SYS_spawn 400
#define SYS_mailread 401
#define SYS_mailwrite 402



#endif // UCORE_SYSCALL_IDS_H
