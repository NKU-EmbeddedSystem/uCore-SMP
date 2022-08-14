#include <arch/riscv.h>
#include <ucore/defs.h>
#include <proc/proc.h>
#include <file/file.h>

int pipealloc(struct file **f0, struct file **f1) {
    struct pipe *pi;

    pi = NULL;
    *f0 = *f1 = NULL;
    if ((*f0 = filealloc()) == NULL || (*f1 = filealloc()) == NULL)
        goto bad;
    if ((pi = (struct pipe *)alloc_physical_page()) == NULL)
        goto bad;
    pi->readopen = 1;
    pi->writeopen = 1;
    pi->nwrite = 0;
    pi->nread = 0;
    init_spin_lock_with_name(&pi->lock, "pipe.lock");
    (*f0)->type = FD_PIPE;
    (*f0)->readable = 1;
    (*f0)->writable = 0;
    (*f0)->pipe = pi;
    (*f1)->type = FD_PIPE;
    (*f1)->readable = 0;
    (*f1)->writable = 1;
    (*f1)->pipe = pi;
    return 0;

bad:
    if (pi)
        recycle_physical_page((char *)pi);
    if (*f0)
        fileclose(*f0);
    if (*f1)
        fileclose(*f1);
    return -1;
}

void
pipeclose(struct pipe *pi, int writable)
{
    acquire(&pi->lock);
    infof("pipeclose at %p: writable=%d readopen=%d writeopen=%d nread=%d "
          "nwrite=%d",
          pi, writable, pi->readopen, pi->writeopen, pi->nread, pi->nwrite);
    if(writable){
        pi->writeopen = 0;
        wakeup(&pi->nread);
    } else {
        pi->readopen = 0;
        wakeup(&pi->nwrite);
    }
    if(pi->readopen == 0 && pi->writeopen == 0){
        release(&pi->lock);
        recycle_physical_page((char*)pi);
    }else
        release(&pi->lock);
}

int pipewrite(struct pipe *pi, uint64 addr, int n) {
    int i = 0;
    struct proc *pr = curr_proc();

    infof("pipewrite at %p: pid = %d try", pi, pr->pid);
    acquire(&pi->lock);
    while (i < n) {
        if (pi->readopen == 0 || pr->killed) {
            release(&pi->lock);
            return -1;
        }
        if (pi->nwrite == pi->nread + PIPESIZE) { 
            wakeup(&pi->nread);
            infof("pipewrite at %p: pid = %d pipe is full", pi, pr->pid);
            sleep(&pi->nwrite, &pi->lock);
            infof("pipewrite at %p: pid = %d woke up", pi, pr->pid);
        } else {
            char ch;
            if (copyin(pr->pagetable, &ch, addr + i, 1) == -1)
                break;
            pi->data[pi->nwrite++ % PIPESIZE] = ch;
            i++;
//            int write_size = MIN(n - i, PIPESIZE - (pi->nwrite - pi->nread));
//            char write_buf[write_size];
//            if (copyin(pr->pagetable, write_buf, addr + i, write_size) == -1) {
//                infof("pipewrite: copyin error");
//                break;
//            }
//            for (int j = 0; j < write_size; j++) {
//                pi->data[(pi->nwrite + j) % PIPESIZE] = write_buf[j];
//            }
//            pi->nwrite += write_size;
//            i += write_size;
        }
    }
    infof("pipewrite at %p: pid = %d want to write %d bytes, finally write %d "
          "bytes", pi, pr->pid, n, i);
    wakeup(&pi->nread);
    release(&pi->lock);

    return i;
}

int piperead(struct pipe *pi, uint64 addr, int n) {
    int i;
    struct proc *pr = curr_proc();
    char ch;

    infof("piperead at  %p: pid = %d try", pi, pr->pid);
    acquire(&pi->lock);
    while (pi->nread == pi->nwrite && pi->writeopen) { 
        if (pr->killed) {
            release(&pi->lock);
            return -1;
        }
        infof("piperead at  %p: pid = %d listen to nread", pi, pr->pid);
        sleep(&pi->nread, &pi->lock);
        infof("piperead at  %p: pid = %d woke up from nread", pi, pr->pid);
    }
    for (i = 0; i < n; i++) {
        if (pi->nread == pi->nwrite)
            break;
        ch = pi->data[pi->nread++ % PIPESIZE];
        if (copyout(pr->pagetable, addr + i, &ch, 1) == -1)
            break;
    }
    infof("piperead at  %p: pid = %d want to read %d bytes, finally read %d "
          "bytes", pi, pr->pid, n, i);
//    int read_size = MIN(n, pi->nwrite - pi->nread);
//    char read_buf[read_size];
//    for (i = 0; i < read_size; i++) {
//        read_buf[i] = pi->data[pi->nread++ % PIPESIZE];
//    }
//    if (copyout(pr->pagetable, addr, read_buf, read_size) == -1) {
//        infof("piperead: copyout error");
//        i = 0;
//    }
    wakeup(&pi->nwrite); 
    release(&pi->lock);
    return i;
}

bool pipe_readable(struct pipe *pi) {
    acquire(&pi->lock);
    bool ret = (pi->nread < pi->nwrite);
    release(&pi->lock);
    return ret;
}

bool pipe_writeable(struct pipe *pi) {
    acquire(&pi->lock);
    bool ret = !(pi->nwrite == pi->nread + PIPESIZE);
    release(&pi->lock);
    return ret;
}
