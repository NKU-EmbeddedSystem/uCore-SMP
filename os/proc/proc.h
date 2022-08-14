#if !defined(PROC_H)
#define PROC_H

#include <ucore/ucore.h>

#include "exec.h"
#include <file/file.h>
#include <lock/lock.h>
#include <arch/timer.h>
#define NPROC (256)
#define KSTACK_SIZE (8192)
#define USTACK_SIZE (PGSIZE * 128) // must be multiple of PGSIZE
#define TRAPFRAME_SIZE (4096)
#define FD_MAX (256)
#define PROC_NAME_MAX (16)
#define MAX_PROC_SHARED_MEM_INSTANCE (32)   // every proc
#define MAX_MAPPING (128)
#define RANDOM_SIZE (16)

// for wait()
#define WNOHANG		0x00000001
#define WUNTRACED	0x00000002
#define WCONTINUED	0x00000008

// for mmap
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

// for rusage
#define	RUSAGE_SELF	0
#define	RUSAGE_CHILDREN	(-1)
#define RUSAGE_BOTH	(-2)		/* sys_wait4() uses this */
#define	RUSAGE_THREAD	1		/* only the calling thread */

// for clock_gettime
#define CLOCK_REALTIME			    0
#define CLOCK_MONOTONIC			    1
#define CLOCK_PROCESS_CPUTIME_ID	2
#define CLOCK_THREAD_CPUTIME_ID		3

// signal
#define SIGHUP    1
#define SIGINT    2
#define SIGQUIT   3
#define SIGILL    4
#define SIGTRAP   5
#define SIGABRT   6
#define SIGIOT    SIGABRT
#define SIGBUS    7
#define SIGFPE    8
#define SIGKILL   9
#define SIGUSR1   10
#define SIGSEGV   11
#define SIGUSR2   12
#define SIGPIPE   13
#define SIGALRM   14
#define SIGTERM   15
#define SIGSTKFLT 16
#define SIGCHLD   17
#define SIGCONT   18
#define SIGSTOP   19
#define SIGTSTP   20
#define SIGTTIN   21
#define SIGTTOU   22
#define SIGURG    23
#define SIGXCPU   24
#define SIGXFSZ   25
#define SIGVTALRM 26
#define SIGPROF   27
#define SIGWINCH  28
#define SIGIO     29
#define SIGPOLL   29
#define SIGPWR    30
#define SIGSYS    31
#define SIGUNUSED SIGSYS


extern char trampoline[];
extern char boot_stack_top[];
extern char boot_stack[];

enum procstate {
    UNUSED = 0,
    USED,
    SLEEPING,
    RUNNABLE,
    RUNNING,
    ZOMBIE
};

struct auxv_t {
    uint64 type;
    uint64 val;
};

struct mapping {
    uint64 va; // must be PGSIZE aligned
    uint npages;
    bool shared;
};

// Per-process state
struct proc {
    struct spinlock lock;

    // PUBLIC: p->lock must be held when using these:
    enum procstate state;  // Process state
    int pid;               // Process ID
    int killed;            // If non-zero, have been killed
    pagetable_t pagetable; // User page table
    void *waiting_target;  // used by sleep and wakeup, a pointer of anything
    uint64 exit_code;      // Exit status to be returned to parent's wait

    // proc_tree_lock must be held when using this:
    struct proc *parent; // Parent process

    // PRIVATE: these are private to the process, so p->lock need not be held.
    uint64 ustack_bottom;        // Virtual address of user stack
    uint64 kstack;               // Virtual address of kernel stack
    struct trapframe *trapframe; // data page for trampoline.S, physical address
    struct context context;      // swtch() here to run process
    uint64 total_size;           // total memory used by this process
    uint64 heap_start;           // start of heap
    uint64 heap_sz;
    uint64 stride;
    uint64 priority;
    uint64 user_time;           // us, user only
    uint64 kernel_time;         // us, kernel only
    uint64 last_start_time;     // us
    struct file *files[FD_MAX]; // Opened files
    struct inode *cwd;          // Current directory
    struct shared_mem * shmem[MAX_PROC_SHARED_MEM_INSTANCE];
    void * shmem_map_start[MAX_PROC_SHARED_MEM_INSTANCE];
    void* next_shmem_addr;
    struct mapping maps[MAX_MAPPING];
    char name[PROC_NAME_MAX]; // Process name (debugging)
};

// process CPU time
struct tms {
    long tms_utime;
    long tms_stime;
    long tms_cutime;
    long tms_cstime;
};

// rusage
struct rusage {
    struct timeval ru_utime; // user time used
    struct timeval ru_stime; // system time used
    long ru_maxrss;          // maximum resident set size
    long ru_ixrss;           // integral shared memory size
    long ru_idrss;           // integral unshared data size
    long ru_isrss;           // integral unshared stack size
    long ru_minflt;          // page reclaims
    long ru_majflt;          // page faults
    long ru_nswap;           // swaps
    long ru_inblock;         // block input operations
    long ru_oublock;         // block output operations
    long ru_msgsnd;          // messages sent
    long ru_msgrcv;          // messages received
    long ru_nsignals;        // signals received
    long ru_nvcsw;           // voluntary context switches
    long ru_nivcsw;          // involuntary context switches
};

struct proc *findproc(int pid);

struct proc *curr_proc();
// int spawn(char *filename);
extern struct proc pool[NPROC];
extern struct spinlock pool_lock;
extern struct spinlock wait_lock;
// extern struct spinlock proc_tree_lock;

void sleep(void *waiting_target, struct spinlock *lk);
void wakeup(void *waiting_target);

void print_proc(struct proc *proc);
void forkret(void);

void proc_free_mem_and_pagetable(struct proc* p);
struct proc *alloc_proc(void);
struct file *get_proc_file_by_fd(struct proc *p, int fd);
pagetable_t proc_pagetable(struct proc *p);
void freeproc(struct proc *p);
int get_cpu_time(struct proc *p, struct tms *tms);
bool the_only_proc_in_pool();
void *mmap(struct proc *p, void *start, size_t len, int prot, int flags, struct inode *ip, off_t off);
int munmap(struct proc *p, void *start, size_t len);
#endif // PROC_H
