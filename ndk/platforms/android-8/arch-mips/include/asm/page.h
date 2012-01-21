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
#ifndef _ASM_PAGE_H
#define _ASM_PAGE_H

#ifndef PAGE_SHIFT
#define PAGE_SHIFT 12
#endif
#define PAGE_SIZE (1UL << PAGE_SHIFT)
#define PAGE_MASK (~((1 << PAGE_SHIFT) - 1))

#endif
