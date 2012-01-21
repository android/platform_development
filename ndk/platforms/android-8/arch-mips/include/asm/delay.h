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
#ifndef _ASM_DELAY_H
#define _ASM_DELAY_H

#include <linux/param.h>
#include <linux/smp.h>

#include <asm/compiler.h>
#include <asm/war.h>

#define __udelay_val cpu_data[raw_smp_processor_id()].udelay_val
#define udelay(usecs) __udelay((usecs), __udelay_val)
#if HZ >= 1000
#define MAX_UDELAY_MS 1
#elif HZ <= 200
#define MAX_UDELAY_MS 5
#else
#define MAX_UDELAY_MS (1000 / HZ)
#endif
#endif
