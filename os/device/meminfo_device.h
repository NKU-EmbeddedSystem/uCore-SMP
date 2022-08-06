#if !defined(MEMINFO_DEVICE_H)
#define MEMINFO_DEVICE_H

#include <ucore/ucore.h>

int64 meminfo_write(char *src, int64 len, int from_user);

int64 meminfo_read(char *dst, int64 len, int to_user);

#endif // MEMINFO_DEVICE_H


