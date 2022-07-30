#if !defined(NULL_DEVICE_H)
#define NULL_DEVICE_H

#include <ucore/ucore.h>

int64 null_write(char *src, int64 len, int from_user);

int64 null_read(char *dst, int64 len, int to_user);

#endif // NULL_DEVICE_H


