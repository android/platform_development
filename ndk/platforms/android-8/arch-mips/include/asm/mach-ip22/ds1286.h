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
#ifndef __ASM_MACH_IP22_DS1286_H
#define __ASM_MACH_IP22_DS1286_H

#include <asm/sgi/hpc3.h>

#define rtc_read(reg) (hpc3c0->rtcregs[(reg)] & 0xff)
#define rtc_write(data, reg) do { hpc3c0->rtcregs[(reg)] = (data); } while(0)

#endif
