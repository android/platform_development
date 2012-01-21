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
#ifndef _MIPS_ATLAS_H
#define _MIPS_ATLAS_H

#include <asm/addrspace.h>

#define ATLAS_RTC_ADR_REG 0x1f000800
#define ATLAS_RTC_DAT_REG 0x1f000808

#define ATLAS_ICTRL_REGS_BASE 0x1f000000

struct atlas_ictrl_regs {
 volatile unsigned int intraw;
 int dummy1;
 volatile unsigned int intseten;
 int dummy2;
 volatile unsigned int intrsten;
 int dummy3;
 volatile unsigned int intenable;
 int dummy4;
 volatile unsigned int intstatus;
 int dummy5;
};

#define ATLAS_UART_REGS_BASE 0x1f000900
#define ATLAS_BASE_BAUD ( 3686400 / 16 )

#define ATLAS_PSUSTBY_REG 0x1f000600
#define ATLAS_GOSTBY 0x4d

#define ATLAS_SAA9730_REG 0x10800000

#define ATLAS_SAA9730_BAUDCLOCK 3692300

#endif
