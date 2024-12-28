#include "syscall.h"
#include "stdint.h"
#include "sbi.h"
#include "proc.h"
#include "mm.h"
#include "defs.h"
#include "stddef.h"
#include "vm.h"
#include "printk.h"
#include "trap.h"

extern struct task_struct *current;
extern uint64_t nr_tasks;
extern struct task_struct *task[];
extern uint64_t swapper_pg_dir[];
extern void __ret_from_fork();

uint64_t sys_getpid() {
    return current -> pid;
}

uint64_t sys_write(unsigned int fd, const char* buf, uint64_t count) {
    if (fd == 1U) {
        for (int i = 0; i < count; i++)
            sbi_debug_console_write_byte((uint8_t)buf[i]);
    }
    return count;
}

uint64_t do_fork(struct pt_regs *regs) {
    uint64_t parent_pid = sys_getpid();
    uint64_t child_pid  = ++nr_tasks;
    // 创建一个新进程
    task[child_pid] = (struct task_struct*)kalloc();
    for (uint64_t i = 0; i < PGSIZE; i++)
        ((char*)(task[child_pid]))[i] = ((char*)(task[parent_pid]))[i];
    // _task->pid 根据 nr_tasks 来赋值
    task[child_pid] -> pid = child_pid;
    // _task->mm.mmap 为 NULL，因为新进程还没有任何映射
    (task[child_pid] -> mm).mmap = NULL;
    // 创建子进程页表
    // _task->pgd 为新分配的页表地址
    task[child_pid] -> pgd = (uint64_t *)((uint64_t)kalloc() - PA2VA_OFFSET);
    uint64_t *pgd_va = (uint64_t *)((uint64_t)(task[child_pid] -> pgd) + PA2VA_OFFSET);
    // 拷贝内核页表 swapper_pg_dir
    for (int j = 256; j < 512; j++)
        (pgd_va)[j] = swapper_pg_dir[j];
    // 遍历父进程 vma，并遍历父进程页表
    for (struct vm_area_struct *p_vma = (task[parent_pid] -> mm).mmap; p_vma; p_vma = p_vma -> vm_next) {
        // 将这个 vma 也添加到新进程的 vma 链表中
        do_mmap(&(task[child_pid] -> mm), p_vma -> vm_start, (uint64_t)(p_vma->vm_end) - (uint64_t)(p_vma->vm_start), p_vma -> vm_pgoff, p_vma -> vm_filesz, p_vma -> vm_flags);
        for (uint64_t tar_va = PGROUNDDOWN(p_vma -> vm_start); tar_va < PGROUNDUP(p_vma -> vm_end); tar_va += PGSIZE) {
            uint64_t *tar_pa;
            uint64_t *base_address, offset1, offset2, offset3, temp;
            base_address = ((uint64_t)task[parent_pid] -> pgd & 0x8000000000000000U) ? task[parent_pid] -> pgd : (uint64_t *)((uint64_t)task[parent_pid] -> pgd + PA2VA_OFFSET);
            offset1 = ((uint64_t)tar_va >> 30) & (((uint64_t)1 << 9) - 1);
            offset2 = ((uint64_t)tar_va >> 21) & (((uint64_t)1 << 9) - 1);
            offset3 = ((uint64_t)tar_va >> 12) & (((uint64_t)1 << 9) - 1);
            temp = base_address[offset1];
            if (!(temp & PGTBL_VALID))
                tar_pa = NULL;
            base_address = (uint64_t *)((temp >> 10 << 12) + PA2VA_OFFSET);
            temp = base_address[offset2];
            if (!(temp & PGTBL_VALID))
                tar_pa = NULL;
            base_address = (uint64_t *)((temp >> 10 << 12) + PA2VA_OFFSET);
            temp = base_address[offset3];
            if (!(temp & PGTBL_VALID))
                tar_pa = NULL;
            base_address = (uint64_t *)(temp >> 10 << 12);
            tar_pa = base_address;
            if (tar_pa == NULL) continue;
            // 如果该 vma 项有对应的页表项存在（说明已经创建了映射），则需要深拷贝一整页的内容并映射到新页表中
            uint64_t *new_pa = (uint64_t *)kalloc();
            for (uint64_t i = 0; i < PGSIZE; i++)
                ((char*)new_pa)[i] = ((char*)tar_pa + PA2VA_OFFSET)[i];
            uint64_t perm = PGTBL_VALID | PGTBL_U |
                ((p_vma -> vm_flags & VM_EXEC ) ? PGTBL_X : 0) |
                ((p_vma -> vm_flags & VM_WRITE) ? PGTBL_W : 0) |
                ((p_vma -> vm_flags & VM_READ ) ? PGTBL_R : 0);
            create_mapping(task[child_pid] -> pgd, (uint64_t)tar_va, (uint64_t)new_pa - PA2VA_OFFSET, PGSIZE, perm);
        }
    }
    (task[child_pid] -> thread).ra = (uint64_t)__ret_from_fork;
    (task[child_pid] -> thread).sp = ((uint64_t)regs & (PGSIZE - 1)) + (uint64_t)task[child_pid];
    ((struct pt_regs*)((task[child_pid] -> thread).sp)) -> x10 = 0;
    (task[child_pid] -> thread).sscratch = csr_read(sscratch);
    return child_pid;
}