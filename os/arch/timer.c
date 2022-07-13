#include <arch/riscv.h>
#include <ucore/defs.h>
#include <ucore/ucore.h>
#include <proc/proc.h>

#include "timer.h"

struct timer timer_pool[NTIMER];
struct spinlock timers_lock;

void timerinit() {
    init_spin_lock_with_name(&timers_lock, "timer");
    for (int i = 0; i < NTIMER; i++) {
        timer_pool[i].wakeup_tick = 0;
        timer_pool[i].valid = 0;
        init_spin_lock(&timer_pool[i].guard_lock);
    }
}

struct timer *add_timer(uint64 expires_us) {
    struct timer *timer = NULL;
    acquire(&timers_lock);
    for (int i = 0; i < NTIMER; i++) {
        acquire(&timer_pool[i].guard_lock);
        if (!timer_pool[i].valid) {
            timer = &timer_pool[i];
            timer->wakeup_tick = get_tick() + US_TO_TICK(expires_us);
            timer->valid = TRUE;
            // don't release the guard lock here
            break;
        }
        release(&timer_pool[i].guard_lock);
    }
    release(&timers_lock);
    return timer;
}

int del_timer(struct timer *timer) {
    acquire(&timer->guard_lock);
    if (!timer->valid) {
        infof("del_timer: timer %p is not valid\n", timer);
        release(&timer->guard_lock);
        return -1;
    }
    timer->valid = FALSE;
    release(&timer->guard_lock);
    return 0;
}

void try_wakeup_timer() {
    uint64 tick = get_tick();
    acquire(&timers_lock);
    for (int i = 0; i < NTIMER; i++) {
        acquire(&timer_pool[i].guard_lock);
        if (timer_pool[i].valid && tick >= timer_pool[i].wakeup_tick) {
            wakeup(&timer_pool[i]);
        }
        release(&timer_pool[i].guard_lock);
    }
    release(&timers_lock);
}

uint64 get_min_wakeup_tick() {
    uint64 min_tick = ~0ULL;
    acquire(&timers_lock);
    for (int i = 0; i < NTIMER; i++) {
        acquire(&timer_pool[i].guard_lock);
        if (timer_pool[i].valid && timer_pool[i].wakeup_tick < min_tick) {
            min_tick = timer_pool[i].wakeup_tick;
        }
        release(&timer_pool[i].guard_lock);
    }
    release(&timers_lock);
    return min_tick;
}

void start_timer_interrupt(){
    w_sie(r_sie() | SIE_STIE);
    set_next_timer();
}

void stop_timer_interrupt(){
    w_sie(r_sie() & ~SIE_STIE);
}

/// Set the next timer interrupt (10 ms)
void set_next_timer() {
    // 100Hz @ QEMU
    const uint64 timebase = TICK_FREQ / TIME_SLICE_PER_SEC; // how many ticks
    uint64 slice_tick = r_time() + timebase;
    uint64 timer_tick = get_min_wakeup_tick();
    set_timer(slice_tick < timer_tick ? slice_tick : timer_tick);
}


uint64 get_time_ms() {
    uint64 time = r_time();
    return time / (TICK_FREQ / MSEC_PER_SEC);
}

uint64 get_time_us() {
    return r_time() / (TICK_FREQ / USEC_PER_SEC);
}

uint64 get_tick() {
    return r_time();
}