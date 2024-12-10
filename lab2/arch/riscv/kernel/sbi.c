#include "stdint.h"
#include "sbi.h"

struct sbiret sbi_ecall(uint64_t  eid, uint64_t  fid,
                        uint64_t arg0, uint64_t arg1,
                        uint64_t arg2, uint64_t arg3,
                        uint64_t arg4, uint64_t arg5) {
    long error;
    long value;
    struct sbiret res;
    asm volatile (
        "mv a7, %[eid]\n"
        "mv a6, %[fid]\n"
        "mv a0, %[arg0]\n"
        "mv a1, %[arg1]\n"
        "mv a2, %[arg2]\n"
        "mv a3, %[arg3]\n"
        "mv a4, %[arg4]\n"
        "mv a5, %[arg5]\n"
        "ecall\n"
        "mv %[error], a0\n"
        "mv %[value], a1\n"
        : [error] "=r"(error),[value] "=r"(value)
        : [eid] "r"(eid), [fid] "r"(fid), [arg0] "r"(arg0), [arg1] "r"(arg1), [arg2] "r"(arg2), [arg3] "r"(arg3), [arg4] "r"(arg4), [arg5] "r"(arg5)
        : "memory"
    );
    res.error = error;
    res.value = value;
    return res;
}

struct sbiret sbi_debug_console_write_byte(uint8_t byte) {
    return sbi_ecall(0x4442434E, 0x2, byte, 0, 0, 0, 0, 0);
}

struct sbiret sbi_system_reset(uint32_t reset_type, uint32_t reset_reason) {
    return sbi_ecall(0x53525354, 0x0, 0, 0, 0, 0, 0, 0);
}

struct sbiret sbi_set_timer(uint64_t stime) {
    return sbi_ecall(0x54494D45, 0x0, stime, 0, 0, 0, 0, 0);
}