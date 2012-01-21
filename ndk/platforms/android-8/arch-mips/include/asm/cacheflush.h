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
#ifndef _ASM_CACHEFLUSH_H
#define _ASM_CACHEFLUSH_H

#include <linux/mm.h>
#include <asm/cpu-features.h>

#define flush_cache_dup_mm(mm) do { (void) (mm); } while (0)

#define flush_dcache_mmap_lock(mapping) do { } while (0)
#define flush_dcache_mmap_unlock(mapping) do { } while (0)
#define ARCH_HAS_FLUSH_ANON_PAGE

#define PG_dcache_dirty PG_arch_1

#define Page_dcache_dirty(page)   test_bit(PG_dcache_dirty, &(page)->flags)
#define SetPageDcacheDirty(page)   set_bit(PG_dcache_dirty, &(page)->flags)
#define ClearPageDcacheDirty(page)   clear_bit(PG_dcache_dirty, &(page)->flags)

#endif
