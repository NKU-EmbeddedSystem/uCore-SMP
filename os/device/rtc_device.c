#include <proc/proc.h>
#include <ucore/defs.h>
#include <arch/timer.h>
#include "rtc_device.h"

void rtc_device_init() {
    device_handler[RTC_DEVICE].read = rtc_read;
    device_handler[RTC_DEVICE].write = rtc_write;
    device_handler[RTC_DEVICE].ioctl = rtc_ioctl;
}

int64 rtc_write(char *src, int64 len, int from_user) {
    return 0;
}

int64 rtc_read(char *dst, int64 len, int to_user) {
    return 0;
}

int rtc_ioctl(struct file* f, int is_user, int cmd, void *arg) {
    uint64 timestamp = get_time_ms() / MSEC_PER_SEC;
    struct tm t;
    t.tm_sec = timestamp % 60;
    t.tm_min = (timestamp / 60) % 60;
    t.tm_hour = (timestamp / 3600) % 24;
    t.tm_mday = (timestamp / 86400) % 31 + 1;
    t.tm_mon = (timestamp / 2592000) % 12 + 1;
    t.tm_year = (timestamp / 31536000) + 1900;
    t.tm_wday = (timestamp / 86400) % 7;
    t.tm_yday = (timestamp / 86400) % 365;
    t.tm_isdst = 0;

    return either_copyout((void *)arg, &t, sizeof(struct tm), is_user) < 0 ? -1 : 0;
}