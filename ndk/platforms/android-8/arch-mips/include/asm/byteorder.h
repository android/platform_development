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
#ifndef _ASM_BYTEORDER_H
#define _ASM_BYTEORDER_H

#include <linux/compiler.h>
#include <asm/types.h>

#ifdef __GNUC__

#ifndef __STRICT_ANSI__
#define __BYTEORDER_HAS_U64__
#define __SWAB_64_THRU_32__
#endif

#endif

#ifdef __MIPSEB__
#include <linux/byteorder/big_endian.h>
#elif defined(__MIPSEL__)
#include <linux/byteorder/little_endian.h>
#else
#error "MIPS, but neither __MIPSEB__, nor __MIPSEL__???"
#endif

#endif
