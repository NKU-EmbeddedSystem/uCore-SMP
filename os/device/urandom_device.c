#include <proc/proc.h>
#include "urandom_device.h"

void urandom_device_init() {
    device_handler[URANDOM_DEVICE].read = urandom_read;
    device_handler[URANDOM_DEVICE].write = urandom_write;
}

int64 urandom_write(char *src, int64 len, int from_user) {
    return 0;
}

int64 urandom_read(char *dst, int64 len, int to_user) {
    char urandom_buf[len];
    for (int i = 0; i < len; i++) {
        urandom_buf[i] = r_time() & 0xff;
    }
    if (either_copyout(dst, urandom_buf, len, to_user) < 0) {
        return 0;
    }
    return len;
}
