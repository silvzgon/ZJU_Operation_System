#include "stdint.h"
#include "sbi.h"
#include "printk.h"

struct sbiret sbi_ecall(uint64_t eid, uint64_t fid,
                        uint64_t arg0, uint64_t arg1, uint64_t arg2,
                        uint64_t arg3, uint64_t arg4, uint64_t arg5) {
    uint64_t error, value;
    __asm__ volatile(
        "add a7, %[eid], x0\n" // move eid to a7
        "add a6, %[fid], x0\n" // move fid to a6
        "add a0, %[arg0], x0\n" // move arg0-5 to a0-5
        "add a1, %[arg1], x0\n"
        "add a2, %[arg2], x0\n"
        "add a3, %[arg3], x0\n"
        "add a4, %[arg4], x0\n"
        "add a5, %[arg5], x0\n"
        "ecall\n" // call SBI ecall
        "add %[error], a0, x0\n" // move error to output
        "add %[value], a1, x0\n" // move value to output
        : [error] "=r" (error), [value] "=r" (value)
        : [eid] "r" (eid), [fid] "r" (fid),
          [arg0] "r" (arg0), [arg1] "r" (arg1), [arg2] "r" (arg2),
          [arg3] "r" (arg3), [arg4] "r" (arg4), [arg5] "r" (arg5)
        : "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "memory"
    );
    return (struct sbiret){error, value};
}

struct sbiret sbi_debug_console_write_byte(uint8_t byte) {
    struct sbiret ret = sbi_ecall(SBI_DEBUG_CONSOLE_WRITE, 2, (uint64_t)byte, 0, 0, 0, 0, 0);
    return ret;
}

struct sbiret sbi_system_reset(uint32_t reset_type, uint32_t reset_reason) {
    struct sbiret ret = sbi_ecall(SBI_SYSTEM_RESET, 0, reset_type, reset_reason, 0, 0, 0, 0);
    return ret;
}

struct sbiret sbi_set_timer(uint64_t stime_value) {
    struct sbiret ret = sbi_ecall(SBI_SET_TIMER, 0, stime_value, 0, 0, 0, 0, 0);
    return ret;
}