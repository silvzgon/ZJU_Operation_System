#ifndef __TRAP_H__
#define __TRAP_H__

struct pt_regs {
    uint64_t sepc;
    uint64_t x31, x30, x29, x28;
    uint64_t x27, x26, x25, x24;
    uint64_t x23, x22, x21, x20;
    uint64_t x19, x18, x17, x16;
    uint64_t x15, x14, x13, x12;
    uint64_t x11, x10, x9, x8;
    uint64_t x7, x6, x5, x4;
    uint64_t x3, x1;
};

#endif