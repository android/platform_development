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
#ifndef _ASM_BITOPS_H
#define _ASM_BITOPS_H

#ifndef _LINUX_BITOPS_H
#error only <linux/bitops.h> can be included directly
#endif

#include <linux/compiler.h>
#include <linux/irqflags.h>
#include <linux/types.h>
#include <asm/barrier.h>
#include <asm/bug.h>
#include <asm/byteorder.h>  
#include <asm/cpu-features.h>
#include <asm/sgidefs.h>
#include <asm/war.h>

#if _MIPS_SZLONG == 32
#define SZLONG_LOG 5
#define SZLONG_MASK 31UL
#define __LL "ll	"
#define __SC "sc	"
#define __INS "ins    "
#define __EXT "ext    "
#elif _MIPS_SZLONG == 64
#define SZLONG_LOG 6
#define SZLONG_MASK 63UL
#define __LL "lld	"
#define __SC "scd	"
#define __INS "dins    "
#define __EXT "dext    "
#endif

#define smp_mb__before_clear_bit() smp_llsc_mb()
#define smp_mb__after_clear_bit() smp_llsc_mb()

#include <asm-generic/bitops/non-atomic.h>
#include <asm-generic/bitops/__ffs.h>
#include <asm-generic/bitops/ffs.h>
#include <asm-generic/bitops/fls.h>
#include <asm-generic/bitops/fls64.h>
#include <asm-generic/bitops/ffz.h>
#include <asm-generic/bitops/find.h>
#endif
