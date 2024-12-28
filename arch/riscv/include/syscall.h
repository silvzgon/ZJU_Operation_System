#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include "stdint.h"
#include "trap.h"

#define SYS_WRITE 64
#define SYS_GETPID 172
#define SYS_CLONE 220

uint64_t sys_getpid();

uint64_t sys_write(unsigned int fd, const char* buf, uint64_t count);

uint64_t *find_phy(uint64_t *pgd, uint64_t *va);

uint64_t do_fork(struct pt_regs *regs);

#endif