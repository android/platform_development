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
#ifndef _ASM_MODULE_H
#define _ASM_MODULE_H

#include <linux/list.h>
#include <asm/uaccess.h>

struct mod_arch_specific {

 struct list_head dbe_list;
 const struct exception_table_entry *dbe_start;
 const struct exception_table_entry *dbe_end;
};

typedef uint8_t Elf64_Byte;

typedef struct {
 Elf64_Addr r_offset;
 Elf64_Word r_sym;
 Elf64_Byte r_ssym;
 Elf64_Byte r_type3;
 Elf64_Byte r_type2;
 Elf64_Byte r_type;
} Elf64_Mips_Rel;

typedef struct {
 Elf64_Addr r_offset;
 Elf64_Word r_sym;
 Elf64_Byte r_ssym;
 Elf64_Byte r_type3;
 Elf64_Byte r_type2;
 Elf64_Byte r_type;
 Elf64_Sxword r_addend;
} Elf64_Mips_Rela;

#error MODULE_PROC_FAMILY undefined for your processor configuration
#define MODULE_KERNEL_SMTC ""
#define MODULE_ARCH_VERMAGIC   MODULE_PROC_FAMILY MODULE_KERNEL_TYPE MODULE_KERNEL_SMTC
#endif
