#include <proc/proc.h>
#include "zero_device.h"

void zero_device_init() {
    device_rw_handler[ZERO_DEVICE].read = zero_read;
    device_rw_handler[ZERO_DEVICE].write = zero_write;
}

int64 zero_write(char *src, int64 len, int from_user) {
    return 0;
}

int64 zero_read(char *dst, int64 len, int to_user) {
    return either_memset(dst, 0, len, to_user) == -1 ? 0 : len;
}
