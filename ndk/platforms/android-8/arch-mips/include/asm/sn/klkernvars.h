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
#ifndef __ASM_SN_KLKERNVARS_H
#define __ASM_SN_KLKERNVARS_H

#define KV_MAGIC_OFFSET 0x0
#define KV_RO_NASID_OFFSET 0x4
#define KV_RW_NASID_OFFSET 0x6

#define KV_MAGIC 0x5f4b565f

#ifndef __ASSEMBLY__

#include <asm/sn/types.h>

typedef struct kern_vars_s {
 int kv_magic;
 nasid_t kv_ro_nasid;
 nasid_t kv_rw_nasid;
 unsigned long kv_ro_baseaddr;
 unsigned long kv_rw_baseaddr;
} kern_vars_t;

#endif

#endif

