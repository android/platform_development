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
#ifndef _ASM_SIBYTE_SB1250_H
#define _ASM_SIBYTE_SB1250_H

#define SIBYTE_RELEASE 0x02111403

#define SB1250_NR_IRQS 64

#define BCM1480_NR_IRQS 128
#define BCM1480_NR_IRQS_HALF 64

#define SB1250_DUART_MINOR_BASE 64

#ifndef __ASSEMBLY__

#include <asm/addrspace.h>

#include <asm/sibyte/sb1250_scd.h>
#include <asm/sibyte/bcm1480_scd.h>

#define AT_spin   __asm__ __volatile__ (   ".set noat\n"   "li $at, 0\n"   "1: beqz $at, 1b\n"   ".set at\n"   )

#endif

#define IOADDR(a) ((void __iomem *)(IO_BASE + (a)))

#endif
