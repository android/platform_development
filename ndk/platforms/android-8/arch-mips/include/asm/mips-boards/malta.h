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
#ifndef __ASM_MIPS_BOARDS_MALTA_H
#define __ASM_MIPS_BOARDS_MALTA_H

#include <asm/addrspace.h>
#include <asm/io.h>
#include <asm/mips-boards/msc01_pci.h>
#include <asm/gt64120.h>

#define MIPS_MSC01_IC_REG_BASE 0x1bc40000
#define MIPS_SOCITSC_IC_REG_BASE 0x1ffa0000

#define MALTA_GT_PORT_BASE get_gt_port_base(GT_PCI0IOLD_OFS)
#define MALTA_BONITO_PORT_BASE ((unsigned long)ioremap (0x1fd00000, 0x10000))
#define MALTA_MSC_PORT_BASE get_msc_port_base(MSC01_PCI_SC2PIOBASL)

#define MALTA_RTC_ADR_REG 0x70
#define MALTA_RTC_DAT_REG 0x71
#define SMSC_CONFIG_REG 0x3f0
#define SMSC_DATA_REG 0x3f1
#define SMSC_CONFIG_DEVNUM 0x7
#define SMSC_CONFIG_ACTIVATE 0x30
#define SMSC_CONFIG_ENTER 0x55
#define SMSC_CONFIG_EXIT 0xaa
#define SMSC_CONFIG_DEVNUM_FLOPPY 0
#define SMSC_CONFIG_ACTIVATE_ENABLE 1
#define SMSC_WRITE(x, a) outb(x, a)
#define MALTA_JMPRS_REG 0x1f000210
#endif
