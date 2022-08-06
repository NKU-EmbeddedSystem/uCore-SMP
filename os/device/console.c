#include <proc/proc.h>
#include <sbi/sbi.h>
#include <utils/log.h>
#include "console.h"
void console_init(){
    device_handler[CONSOLE].read = console_read;
    device_handler[CONSOLE].write = console_write;
}


int64 console_write(char *src, int64 len, int from_user) {
    char copybuf[len + 1];
    if (from_user) {
        struct proc *p = curr_proc();
        acquire(&p->lock);
        if (copyin(p->pagetable, copybuf, (uint64)src, len) < 0) {
            release(&p->lock);
            return 0;
        }
        release(&p->lock);
    } else {
        memmove(copybuf, src, len);
    }
    copybuf[len] = '\0';
    printf("%s", copybuf);
    return len;
}

int64 console_read(char *dst, int64 len, int to_user) {
    int64 i;
    for (i = 0; i < len; i++) {
        int c;
        do {
            c = sbi_console_getchar();
        } while (c == -1);

        if (either_copyout(dst + i, &c, 1, to_user) == -1){
            infof("console_read either_copyout failed");
            break;
        }
    }
    return i;
}
