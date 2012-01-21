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
#ifndef _ASM_VGA_H
#define _ASM_VGA_H

#include <asm/byteorder.h>

#define VGA_MAP_MEM(x, s) (0xb0000000L + (unsigned long)(x))

#define vga_readb(x) (*(x))
#define vga_writeb(x, y) (*(y) = (x))

#define VT_BUF_HAVE_RW

#undef scr_writew
#undef scr_readw

#define scr_memcpyw(d, s, c) memcpy(d, s, c)
#define scr_memmovew(d, s, c) memmove(d, s, c)
#define VT_BUF_HAVE_MEMCPYW
#define VT_BUF_HAVE_MEMMOVEW
#endif
