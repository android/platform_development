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
#ifndef __ASM_TLBFLUSH_H
#define __ASM_TLBFLUSH_H

#include <linux/mm.h>

#define flush_tlb_all() local_flush_tlb_all()
#define flush_tlb_mm(mm) local_flush_tlb_mm(mm)
#define flush_tlb_range(vma, vmaddr, end) local_flush_tlb_range(vma, vmaddr, end)
#define flush_tlb_kernel_range(vmaddr,end)   local_flush_tlb_kernel_range(vmaddr, end)
#define flush_tlb_page(vma, page) local_flush_tlb_page(vma, page)
#define flush_tlb_one(vaddr) local_flush_tlb_one(vaddr)

#endif
