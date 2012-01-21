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
#ifndef __ASM_TX4927_TX4927_H
#define __ASM_TX4927_TX4927_H

#include <asm/txx9irq.h>

#define TX4927_IRQ_CP0_BEG MIPS_CPU_IRQ_BASE
#define TX4927_IRQ_CP0_END (MIPS_CPU_IRQ_BASE + 8 - 1)

#define TX4927_IRQ_PIC_BEG TXX9_IRQ_BASE
#define TX4927_IRQ_PIC_END (TXX9_IRQ_BASE + TXx9_MAX_IR - 1)

#define TX4927_IRQ_USER0 (TX4927_IRQ_CP0_BEG+0)
#define TX4927_IRQ_USER1 (TX4927_IRQ_CP0_BEG+1)
#define TX4927_IRQ_NEST_PIC_ON_CP0 (TX4927_IRQ_CP0_BEG+2)
#define TX4927_IRQ_CPU_TIMER (TX4927_IRQ_CP0_BEG+7)

#define TX4927_IRQ_NEST_EXT_ON_PIC (TX4927_IRQ_PIC_BEG+3)

#endif
