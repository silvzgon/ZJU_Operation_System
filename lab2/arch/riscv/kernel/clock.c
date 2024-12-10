#include "sbi.h"
#include "clock.h"
#include "printk.h"
#include "stdint.h"

uint64_t TIME = 10000000;

uint64_t get_cycles() {
    uint64_t m_time = 0;
    asm volatile ("rdtime %[m_time]\n": [m_time] "=r"(m_time): : "memory");
    return m_time;
}

void clock_set_next_event() {
    uint64_t next = get_cycles() + TIME;
    struct sbiret res;
    res = sbi_set_timer(next);
    if (res.error)
        printk("Error.\n");
    return;
}