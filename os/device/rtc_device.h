#if !defined(RTC_DEVICE_H)
#define RTC_DEVICE_H

#include <ucore/ucore.h>

int64 rtc_write(char *src, int64 len, int from_user);

int64 rtc_read(char *dst, int64 len, int to_user);

int rtc_ioctl(struct file* f, int is_user, int cmd, void *arg);

#endif // RTC_DEVICE_H


