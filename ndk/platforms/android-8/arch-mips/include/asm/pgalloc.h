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
#ifndef _ASM_PGALLOC_H
#define _ASM_PGALLOC_H

#include <linux/highmem.h>
#include <linux/mm.h>
#include <linux/sched.h>

#define pmd_pgtable(pmd) pmd_page(pmd)

#define __pte_free_tlb(tlb,pte)  do {   pgtable_page_dtor(pte);   tlb_remove_page((tlb), pte);  } while (0)
#define check_pgt_cache() do { } while (0)

#endif
