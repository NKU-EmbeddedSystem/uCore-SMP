#include <proc/proc.h>
#include "null_device.h"

void null_device_init() {
    device_rw_handler[NULL_DEVICE].read = null_read;
    device_rw_handler[NULL_DEVICE].write = null_write;
}

int64 null_write(char *src, int64 len, int from_user) {
    return len;
}

int64 null_read(char *dst, int64 len, int to_user) {
    return 0;
}
