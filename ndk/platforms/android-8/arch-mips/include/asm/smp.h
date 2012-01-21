/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef __ASM_SMP_H
#define __ASM_SMP_H

#include <linux/bitops.h>
#include <linux/linkage.h>
#include <linux/threads.h>
#include <linux/cpumask.h>

#include <asm/atomic.h>
#include <asm/smp-ops.h>

#define raw_smp_processor_id() (current_thread_info()->cpu)

#define cpu_number_map(cpu) __cpu_number_map[cpu]

#define cpu_logical_map(cpu) __cpu_logical_map[cpu]

#define NO_PROC_ID (-1)

struct call_data_struct {
 void (*func)(void *);
 void *info;
 atomic_t started;
 atomic_t finished;
 int wait;
};

#define SMP_RESCHEDULE_YOURSELF 0x1  
#define SMP_CALL_FUNCTION 0x2

#define cpu_possible_map phys_cpu_present_map

#endif
