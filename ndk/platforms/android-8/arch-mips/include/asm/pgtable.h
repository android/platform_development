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
#ifndef _ASM_PGTABLE_H
#define _ASM_PGTABLE_H

#include <asm/io.h>
#include <asm/pgtable-bits.h>

struct mm_struct;
struct vm_area_struct;

#define PAGE_NONE __pgprot(_PAGE_PRESENT | _CACHE_CACHABLE_NONCOHERENT)
#define PAGE_SHARED __pgprot(_PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE |   PAGE_CACHABLE_DEFAULT)
#define PAGE_COPY __pgprot(_PAGE_PRESENT | _PAGE_READ |   PAGE_CACHABLE_DEFAULT)
#define PAGE_READONLY __pgprot(_PAGE_PRESENT | _PAGE_READ |   PAGE_CACHABLE_DEFAULT)
#define PAGE_KERNEL __pgprot(_PAGE_PRESENT | __READABLE | __WRITEABLE |   _PAGE_GLOBAL | PAGE_CACHABLE_DEFAULT)
#define PAGE_USERIO __pgprot(_PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE |   PAGE_CACHABLE_DEFAULT)
#define PAGE_KERNEL_UNCACHED __pgprot(_PAGE_PRESENT | __READABLE |   __WRITEABLE | _PAGE_GLOBAL | _CACHE_UNCACHED)

#define __P000 PAGE_NONE
#define __P001 PAGE_READONLY
#define __P010 PAGE_COPY
#define __P011 PAGE_COPY
#define __P100 PAGE_READONLY
#define __P101 PAGE_READONLY
#define __P110 PAGE_COPY
#define __P111 PAGE_COPY

#define __S000 PAGE_NONE
#define __S001 PAGE_READONLY
#define __S010 PAGE_SHARED
#define __S011 PAGE_SHARED
#define __S100 PAGE_READONLY
#define __S101 PAGE_READONLY
#define __S110 PAGE_SHARED
#define __S111 PAGE_SHARED

#define ZERO_PAGE(vaddr)   (virt_to_page((void *)(empty_zero_page + (((unsigned long)(vaddr)) & zero_page_mask))))

#define pmd_phys(pmd) virt_to_phys((void *)pmd_val(pmd))
#define pmd_page(pmd) (pfn_to_page(pmd_phys(pmd) >> PAGE_SHIFT))
#define pmd_page_vaddr(pmd) pmd_val(pmd)

#define pte_none(pte) (!(pte_val(pte) & ~_PAGE_GLOBAL))
#define pte_present(pte) (pte_val(pte) & _PAGE_PRESENT)

#define set_pte_at(mm, addr, ptep, pteval) set_pte(ptep, pteval)
#define set_pmd(pmdptr, pmdval) do { *(pmdptr) = (pmdval); } while(0)
#define PGD_T_LOG2 (__builtin_ffs(sizeof(pgd_t)) - 1)
#define PMD_T_LOG2 (__builtin_ffs(sizeof(pmd_t)) - 1)
#define PTE_T_LOG2 (__builtin_ffs(sizeof(pte_t)) - 1)

#define pgprot_noncached pgprot_noncached
#define mk_pte(page, pgprot) pfn_pte(page_to_pfn(page), (pgprot))

#define kern_addr_valid(addr) (1)
#define io_remap_pfn_range(vma, vaddr, pfn, size, prot)   remap_pfn_range(vma, vaddr, pfn, size, prot)
#include <asm-generic/pgtable.h>
#define HAVE_ARCH_UNMAPPED_AREA
#define pgtable_cache_init() do { } while (0)
#endif
