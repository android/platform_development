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
#ifndef _ASM_COMPILER_H
#define _ASM_COMPILER_H

#if __GNUC__ > (3 || __GNUC__ == (3 && __GNUC_MINOR__ >= 4))
#define GCC_IMM_ASM "n"
#define GCC_REG_ACCUM "$0"
#else
#define GCC_IMM_ASM "rn"
#define GCC_REG_ACCUM "accum"
#endif

#endif
