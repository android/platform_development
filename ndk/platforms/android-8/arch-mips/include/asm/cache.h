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
#ifndef _ASM_CACHE_H
#define _ASM_CACHE_H

#include <kmalloc.h>

#define L1_CACHE_SHIFT CONFIG_MIPS_L1_CACHE_SHIFT
#define L1_CACHE_BYTES (1 << L1_CACHE_SHIFT)

#define SMP_CACHE_SHIFT L1_CACHE_SHIFT
#define SMP_CACHE_BYTES L1_CACHE_BYTES

#endif
