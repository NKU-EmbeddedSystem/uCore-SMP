#include <proc/proc.h>
#include "mount_device.h"

void mount_device_init() {
    device_rw_handler[MOUNT_DEVICE].read = mount_read;
    device_rw_handler[MOUNT_DEVICE].write = mount_write;
}

int64 mount_write(char *src, int64 len, int from_user) {
    return 0;
}

int64 mount_read(char *dst, int64 len, int to_user) {
    char buf[] = "fatfs / fat32 rw,relatime 0 0";
    return either_copyout(dst, buf, len, to_user);
}
