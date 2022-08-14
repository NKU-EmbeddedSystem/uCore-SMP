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
        acquire(&p->lock);
        if (p->state != UNUSED && p->state != ZOMBIE && p->pid == pid) {
            release(&pool_lock);
            goto found;
        }
        release(&p->lock);
    }
    release(&pool_lock);
    infof ("kill: no such pid %d", pid);
//    return -3; // -ESRCH, means no such process
    return 0; // we think it is success

found:
    infof("kill: pid %d found", pid);
    // kill the process
    p->killed = 1;
    if (p->state == SLEEPING) {
        p->state = RUNNABLE;
    }
    release(&p->lock);
    return 0;
}