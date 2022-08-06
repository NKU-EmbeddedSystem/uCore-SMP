#include <proc/proc.h>
int kill(int pid) {
    // pid < 0 is not supported
    if(pid < 0) {
        return -1;
    }

    // search the process with pid
    struct proc *p;
    acquire(&pool_lock);
    for (p = pool; p < &pool[NPROC]; p++) {
        if (p->state == UNUSED && p->pid == pid) {
            acquire(&p->lock);
            release(&pool_lock);
            goto found;
        }
    }
    release(&pool_lock);
//    return -3; // -ESRCH, means no such process
    return 0; // we think it is success

found:
    // kill the process
    p->killed = 1;
    release(&p->lock);
    return 0;
}