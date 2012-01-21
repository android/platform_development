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
#ifndef _ASM_FLOPPY_H
#define _ASM_FLOPPY_H

#include <linux/dma-mapping.h>

#define MAX_BUFFER_SECTORS 24
#define FLOPPY0_TYPE fd_drive_type(0)
#define FLOPPY1_TYPE fd_drive_type(1)
#define FDC1 fd_getfdaddr1();
#define N_FDC 1  
#define N_DRIVE 8
#define CROSS_64KB(a, s) ((unsigned long)(a)/K_64 != ((unsigned long)(a) + (s) - 1) / K_64)
#define EXTRA_FLOPPY_PARAMS
#include <floppy.h>
#endif
