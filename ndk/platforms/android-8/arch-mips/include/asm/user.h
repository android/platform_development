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
#ifndef _ASM_USER_H
#define _ASM_USER_H

#include <asm/page.h>
#include <asm/reg.h>

#define EF_SIZE 180

struct user {
 unsigned long regs[EF_SIZE /
 sizeof(unsigned long) + 64];
 size_t u_tsize;
 size_t u_dsize;
 size_t u_ssize;
 unsigned long start_code;
 unsigned long start_data;
 unsigned long start_stack;
 long int signal;
 unsigned long u_ar0;
 unsigned long magic;
 char u_comm[32];
};

#define NBPG PAGE_SIZE
#define UPAGES 1
#define HOST_TEXT_START_ADDR (u.start_code)
#define HOST_DATA_START_ADDR (u.start_data)
#define HOST_STACK_END_ADDR (u.start_stack + u.u_ssize * NBPG)

#endif
