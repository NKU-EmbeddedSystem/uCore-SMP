#include <arch/riscv.h>
#include <arch/timer.h>
#include <file/file.h>
#include <mem/memory_layout.h>
#include <proc/proc.h>
#include <trap/trap.h>
#include <ucore/defs.h>
#include <utils/log.h>
#include <mem/shared.h>
#include <fatfs/fftest.h>
#include <fatfs/init.h>

struct proc pool[NPROC];
volatile struct proc *creating_proc;
struct spinlock creating_lock;

__attribute__((aligned(16))) char kstack[NPROC][KSTACK_SIZE];
// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;
struct spinlock pool_lock;
// struct spinlock proc_tree_lock;
struct
{
    int pid; // the next available pid
    struct spinlock lock;
} next_pid;
struct proc *curr_proc() {
    // if the timer were to interrupt and cause the thread to yield and then move to a different CPU
    // a previously returned value would no longer be correct.
    push_off();
    struct cpu *c = mycpu();
    struct proc *p = c->proc;
    pop_off();
    return p;
}

struct proc *findproc(int pid) {
    struct proc *p = NULL;
    acquire(&pool_lock);
    for (p = pool; p < &pool[NPROC]; p++) {
        if (p->state != UNUSED && p->pid == pid) {
            break;
        }
    }
    release(&pool_lock);
    return p;
}

void procinit(void) {
    struct proc *p;
    init_spin_lock_with_name(&pool_lock, "pool_lock");
    init_spin_lock_with_name(&wait_lock, "wait_lock");
    init_spin_lock_with_name(&creating_lock, "creating_lock");
    // init_spin_lock_with_name(&proc_tree_lock, "proc_tree_lock");
    for (p = pool; p < &pool[NPROC]; p++) {
        init_spin_lock_with_name(&p->lock, "proc.lock");
        p->state = UNUSED;

    }

    next_pid.pid = 1;
    init_spin_lock_with_name(&next_pid.lock, "next_pid.lock");
}

int alloc_pid() {
    acquire(&next_pid.lock);
    int pid = next_pid.pid;
    next_pid.pid++;
    release(&next_pid.lock);
    return pid;
}
pagetable_t proc_pagetable(struct proc *p) {
    pagetable_t pagetable;

    // An empty page table.
    pagetable = create_empty_user_pagetable();
    if (pagetable == NULL)
        panic("cannot create empty user pagetable");

    if (mappages(pagetable, TRAMPOLINE, PGSIZE,
                 (uint64)trampoline, PTE_R | PTE_X) < 0) {
        free_pagetable_pages(pagetable);
        return 0;
    }

    if ((p->trapframe = (struct trapframe *)alloc_physical_page()) == 0) {
        panic("alloc trapframe page failed\n");
    }

    // map the trapframe just below TRAMPOLINE, for trampoline.S.
    if (mappages(pagetable, TRAPFRAME, PGSIZE,
                 (uint64)(p->trapframe), PTE_R | PTE_W) < 0) {
        ;
        panic("Can not map TRAPFRAME");
    }

    return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void proc_free_mem_and_pagetable(struct proc* p) {
    uvmunmap(p->pagetable, TRAMPOLINE, 1, FALSE);  // unmap, don't recycle physical, shared
    uvmunmap(p->pagetable, TRAPFRAME, 1, TRUE);   // unmap, should recycle physical
    p->trapframe = NULL;

    // unmap shared memory
    for (int i = 0; i < MAX_PROC_SHARED_MEM_INSTANCE; i++)
    {
        if (p->shmem[i])
        { // active shared memory
            debugcore("free shared mem");
            uvmunmap(p->pagetable, (uint64)p->shmem_map_start[i], p->shmem[i]->page_cnt, FALSE);
            // debugcore("free page = %d", get_free_page_count());
            drop_shared_mem(p->shmem[i]);
            p->shmem[i]=NULL;
            p->shmem_map_start[i] = 0;
            // debugcore("free page = %d", get_free_page_count());

        }
    }

    // unmap mapping pages
    for (int i = 0; i < MAX_MAPPING; i++)
    {
        if (p->maps[i].va == NULL) {
            break;
        }
        uvmunmap(p->pagetable, (uint64)p->maps[i].va, p->maps[i].npages, TRUE);
        memset(&p->maps[i], 0, sizeof(struct mapping));
    }

    // total_size sanity check, avoid memory leak
    KERNEL_ASSERT(
            p->total_size ==
            (p->heap_start - USER_TEXT_START) + // bin size
            p->heap_sz + // heap size
            USTACK_SIZE, // stack size
            "proc_free_mem_and_pagetable: total_size sanity check failed"
    );
    free_user_mem_and_pagetables(p->pagetable, p->total_size);
    p->pagetable = NULL;
    p->total_size = 0;
    p->heap_start = 0;
    p->heap_sz = 0;
}



/**
 * @brief clean a process struct
 * The only useful action is "p->state = UNUSED"
 * the others are just for safety
 * should hold p->lock
 * 
 * @param p the proc
 */
void freeproc(struct proc *p) {
    KERNEL_ASSERT(holding(&p->lock), "should lock the process to free");

    KERNEL_ASSERT(p->trapframe == NULL, "p->trapfram is pointing somewhere, did you forget to free trapframe?");
    KERNEL_ASSERT(p->pagetable == NULL, "p->pagetable is pointing somewhere, did you forget to free pagetable?");
    KERNEL_ASSERT(p->cwd == NULL, "p->cwd is not NULL, did you forget to release the inode?");
    KERNEL_ASSERT(p->waiting_target == NULL, "p->cwd is waiting something");
    KERNEL_ASSERT(p->total_size == 0, "memory not freed");
    KERNEL_ASSERT(p->heap_sz == 0, "heap not freed");


    p->heap_start = 0;
    p->state = UNUSED;  // very important
    p->pid = 0;
    p->killed = FALSE;
    p->parent = NULL;
    p->exit_code = 0;
    p->parent = NULL;
    p->ustack_bottom = 0;
    p->kstack = 0;
    memset(&p->context, 0, sizeof(p->context));
    p->stride = 0;
    p->priority = 0;
    p->kernel_time = 0;
    p->user_time = 0;
    p->last_start_time = 0;
    for (int i = 0; i < FD_MAX; i++)
    {
        KERNEL_ASSERT(p->files[i] == NULL, "some file is not closed");
    }
    memset(p->name, 0, PROC_NAME_MAX);
    

    // now everything should be clean
    // still holding the lock
}

/**
 * @brief Allocate a unused proc in the pool
 * and it's initialized to some extend
 * 
 * these are not initialized or you should set them:
 * 
 * parent           NULL
 * ustack_bottom    0
 * total_size       0
 * cwd              NULL
 * name             ""
 * next_shmem_addr  0
 * 
 * @return struct proc* p with lock 
 */
struct proc *alloc_proc(void) {
    struct proc *p;
    acquire(&pool_lock);
    for (p = pool; p < &pool[NPROC]; p++) {
        if (p->state == UNUSED) {
            acquire(&p->lock);
            release(&pool_lock);
            goto found;
        }
    }
    release(&pool_lock);
    return NULL;

found:
    // set creating proc to p
    for (;;) {
        acquire(&creating_lock);
        if (creating_proc == NULL) {
            creating_proc = p;
            release(&creating_lock);
            break;
        }
        release(&creating_lock);
    }

    p->pid = alloc_pid();
    p->state = USED;
    p->total_size = 0;
    p->heap_start = 0;
    p->heap_sz = 0;
    p->killed = FALSE;
    p->waiting_target = NULL;
    p->exit_code = -1;
    p->parent = NULL;
    p->ustack_bottom = 0;
    p->pagetable = proc_pagetable(p);
    if (p->pagetable == NULL) {
        errorf("failed to create user pagetable");
        freeproc(p);
        release(&p->lock);
        return NULL;
    }
    memset(&p->context, 0, sizeof(p->context));
    p->kstack = (uint64)kstack[p - pool];
    memset((void *)p->kstack, 0, KSTACK_SIZE);

    p->context.ra = (uint64)forkret; // used in swtch()
    p->context.sp = p->kstack + KSTACK_SIZE;

    p->stride = 0;
    p->priority = 16;
    p->user_time = 0;
    p->kernel_time = 0;
    p->last_start_time = 0;
    for (int i = 0; i < FD_MAX; i++) {
        p->files[i] = NULL;
    }
    p->cwd = NULL;
    p->name[0] = '\0';
    for (int i = 0; i < MAX_PROC_SHARED_MEM_INSTANCE; i++)
    {
       p->shmem[i] = NULL;
       p->shmem_map_start[i]= 0;
    }
    p->next_shmem_addr = 0;

    // clear mapping fields
    for (int i = 0; i < MAX_MAPPING; i++)
    {
        p->maps[i].va = 0;
        p->maps[i].npages = 0;
        p->maps[i].shared = FALSE;
    }

    return p;
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void forkret(void) {
    pushtrace(0x3200);
    static int first = TRUE;
    // reset creating_proc to NULL
    acquire(&creating_lock);
    creating_proc = NULL;
    release(&creating_lock);
    // Still holding p->lock from scheduler.
    release(&curr_proc()->lock);

    if (first) {
        // File system initialization must be run in the context of a
        // regular process (e.g., because it calls sleep), and thus cannot
        // be run from main().
        first = FALSE;
//        fftest_qemu();
        ffinit();
        printf("init file system\n");
//        inode_test();
    }

    usertrapret();
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void switch_to_scheduler(void) {
    int base_interrupt_status;
    uint64 old_ra = r_ra();
    pushtrace(0x3006);
    struct proc *p = curr_proc();
    KERNEL_ASSERT(p->state != RUNNING, "current proc shouldn't be running");
    KERNEL_ASSERT(holding(&p->lock), "should hold currernt proc's lock"); // holding currernt proc's lock
    KERNEL_ASSERT(mycpu()->noff == 1, "");                                // and it's the only lock
    KERNEL_ASSERT(!intr_get(), "interrput should be off");                // interrput is off

    base_interrupt_status = mycpu()->base_interrupt_status;
    // debugcore("in switch_to_scheduler before swtch base_interrupt_status=%d", base_interrupt_status);
    uint64 old_sp = r_sp();
    swtch(&p->context, &mycpu()->context); // will goto scheduler()
    uint64 new_sp = r_sp();
    // debugcore("in switch_to_scheduler after swtch");
    mycpu()->base_interrupt_status = base_interrupt_status;
    pushtrace(0x3020);
    pushtrace(old_sp);
    pushtrace(new_sp);
    pushtrace(old_ra);
    pushtrace(*(uint64*)(r_sp()+40));
}

/**
 * @brief For debugging.
 * 
 * @param proc 
 */
void print_proc(struct proc *proc) {
    printf_k("* ---------- PROC INFO ----------\n");
    printf_k("* pid:                %d\n", proc->pid);
    printf_k("* status:             ");
    if (proc->state == UNUSED) {
        printf_k("UNUSED\n");
    } else if (proc->state == USED) {
        printf_k("USED\n");
    } else if (proc->state == SLEEPING) {
        printf_k("SLEEPING\n");
    } else if (proc->state == RUNNING) {
        printf_k("RUNNING\n");
    } else if (proc->state == ZOMBIE) {
        printf_k("ZOMBIE\n");
    } else if (proc->state == RUNNABLE) {
        printf_k("RUNNABLE\n");
    } else {
        printf_k("UNKNOWN\n");
    }
    printf_k("* locked:             %d\n", proc->lock.locked);
    printf_k("* killed:             %d\n", proc->killed);
    printf_k("* pagetable:          %p\n", proc->pagetable);
    printf_k("* waiting target:     %p\n", proc->waiting_target);
    printf_k("* exit_code:          %d\n", proc->exit_code);
    printf_k("*\n");

    printf_k("* parent:             %p\n", proc->parent);
    printf_k("* ustack_bottom:      %p\n", proc->ustack_bottom);
    printf_k("* kstack:             %p\n", proc->kstack);
    printf_k("* trapframe:          %p\n", proc->trapframe);
    if(proc->trapframe){
        printf_k("*     ra:             %p\n", proc->trapframe->ra);
        printf_k("*     sp:             %p\n", proc->trapframe->sp);
        printf_k("*     epc:            %p\n", proc->trapframe->epc);
    }
    printf_k("* context:            \n");
    printf_k("*     ra:             %p\n", proc->context.ra);
    printf_k("*     sp:             %p\n", proc->context.sp);
    printf_k("* total_size:         %p\n", proc->total_size);
    printf_k("* heap_start:         %p\n", proc->heap_start);
    printf_k("* heap_sz:            %p\n", proc->heap_sz);
    printf_k("* stride:             %p\n", proc->stride);
    printf_k("* priority:           %p\n", proc->priority);
    printf_k("* kernel_time:        %p\n", proc->kernel_time);
    printf_k("* user_time:          %p\n", proc->user_time);
    printf_k("* last_time:          %p\n", proc->last_start_time);
    printf_k("* files:              \n");
    for (int i = 0; i < FD_MAX; i++) {
        if (proc->files[i] != NULL) {
            if (i < 10) {
                printf_k("*     files[ %d]:      %p\n", i, proc->files[i]);
            } else {
                printf_k("*     files[%d]:      %p\n", i, proc->files[i]);
            }
        }
    }
    printf_k("* files:              \n");
    printf_k("* cwd:                %p\n", proc->cwd);
    printf_k("* name:               %s\n", proc->name);

    printf_k("* -------------------------------\n");
    printf_k("\n");
}


/**
 * Allocate a file descriptor of this process for the given file
 */
int fdalloc(struct file *f) {
    struct proc *p = curr_proc();

    for (int i = 0; i < FD_MAX; ++i) {
        if (p->files[i] == 0) {
            p->files[i] = f;
            return i;
        }
    }
    return -1;
}

int fdalloc2(struct file *f, int fd) {
    struct proc *p = curr_proc();

    if (fd < 0 || fd >= FD_MAX) {
        return -1;
    }
    if (p->files[fd] != 0) {
        return -1;
    }
    p->files[fd] = f;
    return fd;
}

void wakeup(void *waiting_target) {
    struct proc *p;

    acquire(&creating_lock);
    for (p = pool; p < &pool[NPROC]; p++) {
        if (p != curr_proc() && p != creating_proc) {
            acquire(&p->lock);
            if (p->state == SLEEPING && p->waiting_target == waiting_target) {
                p->state = RUNNABLE;
            }
            release(&p->lock);
        }
    }
    release(&creating_lock);
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void sleep(void *waiting_target, struct spinlock *lk) {
    
    struct proc *p = curr_proc();

//    tracecore("sleep");
    // Must acquire p->lock in order to
    // change p->state and then call switch_to_scheduler.
    // Once we hold p->lock, we can be
    // guaranteed that we won't miss any wakeup
    // (wakeup locks p->lock),
    // so it's okay to release lk.
    // print_proc(p);

    acquire(&p->lock); 

    release(lk);

    // Go to sleep.
    p->waiting_target = waiting_target;
    p->state = SLEEPING;

    switch_to_scheduler();
    pushtrace(0x3031);

    // Tidy up.
    p->waiting_target = NULL;

    // Reacquire original lock.

    release(&p->lock);

    acquire(lk);

    // debugcore("sleep end");
}

struct file *get_proc_file_by_fd(struct proc *p, int fd) {
    if (p == NULL) {
        panic("get_proc_file_by_fd: p is NULL");
    }

    if (fd < 0 || fd >= FD_MAX) {
        return NULL;
    }

    return p->files[fd];
}

// get the given process and its child processes running time in ticks
// include kernel time and user time
int get_cpu_time(struct proc *p, struct tms *tms) {
    if (p == NULL) {
        infof("get_cpu_time: p is NULL");
        return -1;
    }
    if (tms == NULL) {
        infof("get_cpu_time: tms is NULL");
        return -1;
    }

    tms->tms_utime = p->user_time;
    tms->tms_stime = p->kernel_time;
    tms->tms_cutime = 0;
    tms->tms_cstime = 0;

    acquire(&pool_lock);
    struct proc *child;
    for (child = pool; child < &pool[NPROC]; child++)
    {
        if (child != p) // avoid deadlock
        {
            if (child->state != UNUSED && child->parent == p)
            {
                // found a child
                acquire(&child->lock);
                tms->tms_cutime += child->user_time;
                tms->tms_cstime += child->kernel_time;
                release(&child->lock);
            }
        }
    }
    release(&pool_lock);
    return 0;
}

bool the_only_proc_in_pool() {
    acquire(&pool_lock);
    int count = 0;
    struct proc *p;
    for (p = pool; p < &pool[NPROC]; p++) {
        if (p->state != UNUSED) {
            count++;
        }
    }
    release(&pool_lock);
    return count == 1;
}

static bool is_free_range(pagetable_t pagetable, uint64 start, uint npages) {
    uint64 va;
    pte_t *pte;
    for (va = start; va < start + npages * PGSIZE; va += PGSIZE) {
        if ((pte = walk(pagetable, va, TRUE)) == 0) {
            return FALSE;
        }
        if ((*pte & PTE_V) != 0) {
            return FALSE;
        }
    }
    return TRUE;
}

static void *get_free_range(pagetable_t pagetable, uint npages, uint64 hint_address) {
    infof("get_free_range: npages: %d, hint_address: %p", npages, hint_address);
    uint64 va;
    if (hint_address) {
        for (va = hint_address; va <= USER_STACK_BOTTOM - USTACK_SIZE - (npages + 1) * PGSIZE; va += PGSIZE) {
            if (is_free_range(pagetable, va, npages)) {
                return (void *)va;
            }
        }
    } else {
        for (va = USER_STACK_BOTTOM - USTACK_SIZE - npages * PGSIZE; va > USER_TEXT_START; va -= PGSIZE) {
            if (is_free_range(pagetable, va, npages)) {
                return (void *) va;
            }
        }
    }
    return NULL;
}

static int mapping_add(struct proc *p, uint64 va, uint npages, bool shared) {
    KERNEL_ASSERT(p->maps[MAX_MAPPING - 1].va == NULL, "mapping_add: too many mappings");

    // find a entry to insert
    int i;
    for (i = 0; i < MAX_MAPPING; i++) {
        if (p->maps[i].va > va || p->maps[i].va == NULL) {
            break;
        }
    }

    // make sure there's no overlap
    if (p->maps[i].va && va + npages * PGSIZE > p->maps[i].va) {
        infof("mapping_add: overlap");
        return -1;
    }

    // move all mappings after i to the right
    for (int j = MAX_MAPPING - 1; j > i; j--) {
        p->maps[j] = p->maps[j - 1];
    }

    // insert the new mapping
    p->maps[i].va = va;
    p->maps[i].npages = npages;
    p->maps[i].shared = shared;
    return 0;
}

static int mapping_remove_fixed(struct proc *p, uint64 va, uint npages) {
    // find the mapping
    int i;
    for (i = 0; i < MAX_MAPPING; i++) {
        if (p->maps[i].va == va && p->maps[i].npages == npages) {
            break;
        }
    }
    if (i == MAX_MAPPING) {
        infof("mapping_remove: not found");
        return -1;
    }

    // move all mappings after i to the left
    for (int j = i; j < MAX_MAPPING - 1; j++) {
        p->maps[j] = p->maps[j + 1];
    }
    memset(&p->maps[MAX_MAPPING - 1], 0, sizeof(struct mapping));
    return 0;
}

static int mapping_try_remove_page(struct proc *p, uint64 check_va) {
    KERNEL_ASSERT(check_va % PGSIZE == 0, "mapping_get_range: check_va is not page aligned");
    uint64 left_va, right_va;
    uint left_npages, right_npages;

    // find a entry contains check_va
    int i;
    uint64 begin, end;
    bool shared;
    for (i = 0; i < MAX_MAPPING; i++) {
        begin = p->maps[i].va;
        end = begin + p->maps[i].npages * PGSIZE;
        shared = p->maps[i].shared;
        if (check_va >= begin && check_va < end) {
            goto range_found;
        }
    }
    // not found
    return -1;

range_found:
    left_va = begin;
    left_npages = (check_va - begin) / PGSIZE;
    right_va = check_va + PGSIZE;
    right_npages = (end - right_va) / PGSIZE;

    // remove the mapping
    if (mapping_remove_fixed(p, begin, (end - begin) / PGSIZE) < 0) {
        panic("mapping_try_remove_page: mapping_remove_fixed failed");
    }
    // add the left and right mappings
    if (left_npages > 0 && mapping_add(p, left_va, left_npages, shared) < 0) {
        panic("mapping_try_remove_page: mapping_add left failed");
    }
    if (right_npages > 0 && mapping_add(p, right_va, right_npages, shared) < 0) {
        panic("mapping_try_remove_page: mapping_add right failed");
    }
    return 0;
}

void *mmap(struct proc *p, void *start, size_t len, int prot, int flags, struct inode *ip, off_t off) {
    if (p->maps[MAX_MAPPING - 1].va != 0) {
        infof("sys_mmap: too many mappings");
        return MAP_FAILED;
    }

    // length sanity check and do alignment
    if (len == 0) {
        infof("sys_mmap: len cannot be 0");
        return MAP_FAILED;
    }
    len = PGROUNDUP(len);
    uint npages = len / PGSIZE;

    // get valid start address
    if (flags & MAP_FIXED) {
        // MAP_FIXED sanity check
        if (start == NULL) {
            infof("MAP_FIXED: start cannot be NULL");
            return MAP_FAILED;
        }
        if (((uint64)start % PGSIZE) != 0) {
            infof("MAP_FIXED: start must be page aligned");
            return MAP_FAILED;
        }

        if (!is_free_range(p->pagetable, (uint64)start, npages)) {
            // try to remove existing mapping overlapping with the new one
            for (uint64 va = (uint64)start; va < (uint64)start + npages * PGSIZE; va += PGSIZE) {
                if (mapping_try_remove_page(p, va) == 0) {
                    uvmunmap(p->pagetable, va, 1, TRUE);
                }
            }
            // check again
            if (!is_free_range(p->pagetable, (uint64)start, npages)) {
                infof("MAP_FIXED: start is not free");
                return MAP_FAILED;
            }
        }
    } else {
        start = get_free_range(p->pagetable, npages, PGROUNDUP((uint64)start));
        if (start == NULL) {
            infof("sys_mmap: no free range");
            return MAP_FAILED;
        }
    }

    // calculate page protection
//    int page_prot = PTE_U;
//    if (prot & PROT_READ) {
//        page_prot |= PTE_R;
//    }
//    if (prot & PROT_WRITE) {
//        page_prot |= PTE_W;
//    }
//    if (prot & PROT_EXEC) {
//        page_prot |= PTE_X;
//    }
    int page_prot = PTE_U | PTE_R | PTE_W | PTE_X;

    // do mmap
    void *pa_arr[npages];
    memset(pa_arr, 0, sizeof(pa_arr));

    if (flags & MAP_ANONYMOUS) {
        // allocate physical pages
        for (uint i = 0; i < npages; i++) {
            void *pa = alloc_physical_page();
            if (pa == NULL) {
                infof("sys_mmap: no free physical page");
                goto free_pages;
            }
            memset(pa, 0, PGSIZE);
            pa_arr[i] = pa;
        }
    } else {
        if (PGROUNDUP(f_size(&ip->file)) < off + len) {
            infof("sys_mmap: file is too small, so expand it");
            f_lseek(&ip->file, off + len);
        }
        struct page_cache *cache;
        void *pa;
        for (uint i = 0; i < npages; i++) {
            cache = ctable_acquire(ip, off + i * PGSIZE);
            if (cache == NULL) {
                infof("sys_mmap: file is too small");
                goto free_pages;
            }
            if (flags & MAP_SHARED) {
                pa = cache->page;
                dup_physical_page(cache->page);
            } else {
                pa = alloc_physical_page();
                if (pa == NULL) {
                    infof("sys_mmap: no free physical page");
                    goto free_pages;
                }
                memmove(pa, cache->page, PGSIZE);
            }
            pa_arr[i] = pa;
            release_mutex_sleep(&cache->lock);
        }
    }

    // map pages
    for (uint i = 0; i < npages; i++) {
        if (map1page(p->pagetable, (uint64)start + i * PGSIZE,
                     (uint64)pa_arr[i], page_prot) < 0) {
            panic("sys_mmap: map1page failed, should not happen");
        }
    }

    // record mapping info
    if (mapping_add(p, (uint64)start, npages, !!(flags & MAP_SHARED)) < 0) {
        panic("sys_mmap: mapping_add failed, found data inconsistent");
    }
    return start;

    free_pages:
    for (uint i = 0; i < npages; i++) {
        if (pa_arr[i] == NULL) {
            break;
        }
        put_physical_page(pa_arr[i]);
    }
    return MAP_FAILED;
}

int munmap(struct proc *p, void *start, size_t len) {
    if ((uint64)start % PGSIZE != 0) {
        infof("sys_munmap: start is not page aligned");
        return -1;
    }
    len = PGROUNDUP(len);
    uint npages = len / PGSIZE;

    bool removed = FALSE;
    for (uint64 va = (uint64)start; va < (uint64)start + npages * PGSIZE; va += PGSIZE) {
        if (mapping_try_remove_page(p, va) == 0) {
            removed = TRUE;
            uvmunmap(p->pagetable, va, 1, TRUE);
        }
    }

    return removed ? 0 : -1;
}