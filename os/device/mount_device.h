#if !defined(MOUNT_DEVICE_H)
#define MOUNT_DEVICE_H

#include <ucore/ucore.h>

int64 mount_write(char *src, int64 len, int from_user);

int64 mount_read(char *dst, int64 len, int to_user);

#endif // MOUNT_DEVICE_H


