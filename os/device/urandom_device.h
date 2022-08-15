#if !defined(URANDOM_DEVICE_H)
#define URANDOM_DEVICE_H

#include <ucore/ucore.h>

int64 urandom_write(char *src, int64 len, int from_user);

int64 urandom_read(char *dst, int64 len, int to_user);

#endif // URANDOM_DEVICE_H


