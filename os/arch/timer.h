#if !defined(TIMER_H)
#define TIMER_H

#include <ucore/defs.h>
#include <lock/lock.h>
#define TIME_SLICE_PER_SEC 100    // 10 ms
#define MSEC_PER_SEC 1000    // 1s = 1000 ms
#define USEC_PER_SEC 1000000 // 1s = 1000000 us

#define TICK_FREQ 1000000    // 1 MHz   FUF740-C000 for csr time
#define TICK_TO_MS(tick) ((tick) / (TICK_FREQ / MSEC_PER_SEC))
#define TICK_TO_US(tick) ((tick) / (TICK_FREQ / USEC_PER_SEC)) // not accurate
#define US_TO_TICK(us) ((us) * (TICK_FREQ / USEC_PER_SEC))
#define MS_TO_TICK(ms) ((ms) * (TICK_FREQ / MSEC_PER_SEC))
#define SECOND_TO_TICK(sec) ((sec)*TICK_FREQ)

#define CYCLE_FREQ 3000000000    // 3 GHz I guess  for csr cycle
#define CYCLE_TO_MS(cycle) ((cycle) / (CYCLE_FREQ / MSEC_PER_SEC))
#define CYCLE_TO_US(cycle) ((cycle) / (CYCLE_FREQ / USEC_PER_SEC)) 
#define MS_TO_CYCLE(ms) ((ms) * (CYCLE_FREQ / MSEC_PER_SEC))
#define SECOND_TO_CYCLE(sec) ((sec)*CYCLE_FREQ)

#define NTIMER 10

struct timeval {
    uint64 tv_sec;
    uint64 tv_usec;
};

struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};

struct timer {
    uint64 wakeup_tick;
    bool valid;
    struct spinlock guard_lock;
};

uint64 get_time_ms();
uint64 get_time_us();
uint64 get_tick();

void start_timer_interrupt();

void stop_timer_interrupt();

struct timer *add_timer(uint64 expires_us);
int del_timer(struct timer *timer);
void try_wakeup_timer();
uint64 get_min_wakeup_tick();

#endif // TIMER_H
