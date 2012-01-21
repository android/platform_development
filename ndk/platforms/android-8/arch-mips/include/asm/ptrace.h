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
#ifndef _ASM_PTRACE_H
#define _ASM_PTRACE_H

#define FPR_BASE 32
#define PC 64
#define CAUSE 65
#define BADVADDR 66
#define MMHI 67
#define MMLO 68
#define FPC_CSR 69
#define FPC_EIR 70
#define DSP_BASE 71  
#define DSP_CONTROL 77
#define ACX 78

struct pt_regs {
 unsigned long long regs[32];
 unsigned long long lo;
 unsigned long long hi;
 unsigned long long cp0_epc;
 unsigned long long cp0_badvaddr;
 unsigned long long cp0_status;
 unsigned long long cp0_cause;
} __attribute__ ((aligned (8)));

#define PTRACE_GETREGS 12
#define PTRACE_SETREGS 13
#define PTRACE_GETFPREGS 14
#define PTRACE_SETFPREGS 15

#define PTRACE_OLDSETOPTIONS 21

#define PTRACE_GET_THREAD_AREA 25
#define PTRACE_SET_THREAD_AREA 26

#define PTRACE_PEEKTEXT_3264 0xc0
#define PTRACE_PEEKDATA_3264 0xc1
#define PTRACE_POKETEXT_3264 0xc2
#define PTRACE_POKEDATA_3264 0xc3
#define PTRACE_GET_THREAD_AREA_3264 0xc4

#endif
