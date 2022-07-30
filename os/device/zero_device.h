#if !defined(ZERO_DEVICE_H)
#define ZERO_DEVICE_H

#include <ucore/ucore.h>

int64 zero_write(char *src, int64 len, int from_user);

int64 zero_read(char *dst, int64 len, int to_user);

#endif // ZERO_DEVICE_H


