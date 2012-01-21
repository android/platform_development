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
#ifndef _ASM_IO_H
#define _ASM_IO_H

#include <linux/compiler.h>
#include <linux/kernel.h>
#include <linux/types.h>

#include <asm/addrspace.h>
#include <asm/byteorder.h>
#include <asm/cpu.h>
#include <asm/cpu-features.h>
#include <asm-generic/iomap.h>
#include <asm/page.h>
#include <asm/pgtable-bits.h>
#include <asm/processor.h>
#include <asm/string.h>

#include <ioremap.h>
#include <mangle-port.h>

#undef CONF_SLOWDOWN_IO

#define __raw_ioswabb(a, x) (x)
#define __raw_ioswabw(a, x) (x)
#define __raw_ioswabl(a, x) (x)
#define __raw_ioswabq(a, x) (x)
#define ____raw_ioswabq(a, x) (x)

#define IO_SPACE_LIMIT 0xffff

#define __SLOW_DOWN_IO   __asm__ __volatile__(   "sb\t$0,0x80(%0)"   : : "r" (mips_io_port_base));
#ifdef CONF_SLOWDOWN_IO
#ifdef REALLY_SLOW_IO
#define SLOW_DOWN_IO { __SLOW_DOWN_IO; __SLOW_DOWN_IO; __SLOW_DOWN_IO; __SLOW_DOWN_IO; }
#else
#define SLOW_DOWN_IO __SLOW_DOWN_IO
#endif
#else
#define SLOW_DOWN_IO
#endif
#define isa_page_to_bus page_to_phys
#define virt_to_bus virt_to_phys
#define bus_to_virt phys_to_virt

#define page_to_phys(page) ((dma_addr_t)page_to_pfn(page) << PAGE_SHIFT)

#define __IS_LOW512(addr) (!((phys_t)(addr) & (phys_t) ~0x1fffffffULL))
#undef __IS_LOW512
#define ioremap(offset, size)   __ioremap_mode((offset), (size), _CACHE_UNCACHED)
#define ioremap_nocache(offset, size)   __ioremap_mode((offset), (size), _CACHE_UNCACHED)
#define ioremap_cachable(offset, size)   __ioremap_mode((offset), (size), PAGE_CACHABLE_DEFAULT)
#define ioremap_cacheable_cow(offset, size)   __ioremap_mode((offset), (size), _CACHE_CACHABLE_COW)
#define ioremap_uncached_accelerated(offset, size)   __ioremap_mode((offset), (size), _CACHE_UNCACHED_ACCELERATED)
#define __IS_KSEG1(addr) (((unsigned long)(addr) & ~0x1fffffffUL) == CKSEG1)
#undef __IS_KSEG1
#define __BUILD_MEMORY_SINGLE(pfx, bwlq, type, irq)    static inline void pfx##write##bwlq(type val,   volatile void __iomem *mem)  {   volatile type *__mem;   type __val;     __mem = (void *)__swizzle_addr_##bwlq((unsigned long)(mem));     __val = pfx##ioswab##bwlq(__mem, val);     if (sizeof(type) != sizeof(u64) || sizeof(u64) == sizeof(long))   *__mem = __val;   else if (cpu_has_64bits) {   unsigned long __flags;   type __tmp;     if (irq)   local_irq_save(__flags);   __asm__ __volatile__(   ".set	mips3" "\t\t# __writeq""\n\t"   "dsll32	%L0, %L0, 0" "\n\t"   "dsrl32	%L0, %L0, 0" "\n\t"   "dsll32	%M0, %M0, 0" "\n\t"   "or	%L0, %L0, %M0" "\n\t"   "sd	%L0, %2" "\n\t"   ".set	mips0" "\n"   : "=r" (__tmp)   : "0" (__val), "m" (*__mem));   if (irq)   local_irq_restore(__flags);   } else   BUG();  }    static inline type pfx##read##bwlq(const volatile void __iomem *mem)  {   volatile type *__mem;   type __val;     __mem = (void *)__swizzle_addr_##bwlq((unsigned long)(mem));     if (sizeof(type) != sizeof(u64) || sizeof(u64) == sizeof(long))   __val = *__mem;   else if (cpu_has_64bits) {   unsigned long __flags;     if (irq)   local_irq_save(__flags);   __asm__ __volatile__(   ".set	mips3" "\t\t# __readq" "\n\t"   "ld	%L0, %1" "\n\t"   "dsra32	%M0, %L0, 0" "\n\t"   "sll	%L0, %L0, 0" "\n\t"   ".set	mips0" "\n"   : "=r" (__val)   : "m" (*__mem));   if (irq)   local_irq_restore(__flags);   } else {   __val = 0;   BUG();   }     return pfx##ioswab##bwlq(__mem, __val);  }
#define __BUILD_IOPORT_SINGLE(pfx, bwlq, type, p, slow)    static inline void pfx##out##bwlq##p(type val, unsigned long port)  {   volatile type *__addr;   type __val;     __addr = (void *)__swizzle_addr_##bwlq(mips_io_port_base + port);     __val = pfx##ioswab##bwlq(__addr, val);         BUILD_BUG_ON(sizeof(type) > sizeof(unsigned long));     *__addr = __val;   slow;  }    static inline type pfx##in##bwlq##p(unsigned long port)  {   volatile type *__addr;   type __val;     __addr = (void *)__swizzle_addr_##bwlq(mips_io_port_base + port);     BUILD_BUG_ON(sizeof(type) > sizeof(unsigned long));     __val = *__addr;   slow;     return pfx##ioswab##bwlq(__addr, __val);  }
#define __BUILD_MEMORY_PFX(bus, bwlq, type)    __BUILD_MEMORY_SINGLE(bus, bwlq, type, 1)
#define BUILDIO_MEM(bwlq, type)    __BUILD_MEMORY_PFX(__raw_, bwlq, type)  __BUILD_MEMORY_PFX(, bwlq, type)  __BUILD_MEMORY_PFX(__mem_, bwlq, type)  
#define __BUILD_IOPORT_PFX(bus, bwlq, type)   __BUILD_IOPORT_SINGLE(bus, bwlq, type, ,)   __BUILD_IOPORT_SINGLE(bus, bwlq, type, _p, SLOW_DOWN_IO)
#define BUILDIO_IOPORT(bwlq, type)   __BUILD_IOPORT_PFX(, bwlq, type)   __BUILD_IOPORT_PFX(__mem_, bwlq, type)
#define __BUILDIO(bwlq, type)    __BUILD_MEMORY_SINGLE(____raw_, bwlq, type, 0)
#define readb_relaxed readb
#define readw_relaxed readw
#define readl_relaxed readl
#define readq_relaxed readq
#define readq readq
#define writeq writeq
#define __BUILD_MEMORY_STRING(bwlq, type)    static inline void writes##bwlq(volatile void __iomem *mem,   const void *addr, unsigned int count)  {   const volatile type *__addr = addr;     while (count--) {   __mem_write##bwlq(*__addr, mem);   __addr++;   }  }    static inline void reads##bwlq(volatile void __iomem *mem, void *addr,   unsigned int count)  {   volatile type *__addr = addr;     while (count--) {   *__addr = __mem_read##bwlq(mem);   __addr++;   }  }
#define __BUILD_IOPORT_STRING(bwlq, type)    static inline void outs##bwlq(unsigned long port, const void *addr,   unsigned int count)  {   const volatile type *__addr = addr;     while (count--) {   __mem_out##bwlq(*__addr, port);   __addr++;   }  }    static inline void ins##bwlq(unsigned long port, void *addr,   unsigned int count)  {   volatile type *__addr = addr;     while (count--) {   *__addr = __mem_in##bwlq(port);   __addr++;   }  }
#define BUILDSTRING(bwlq, type)    __BUILD_MEMORY_STRING(bwlq, type)  __BUILD_IOPORT_STRING(bwlq, type)
#define mmiowb() asm volatile ("sync" ::: "memory")
#define __ISA_IO_base ((char *)(isa_slot_offset))
#define dma_cache_wback_inv(start,size)   do { (void) (start); (void) (size); } while (0)
#define dma_cache_wback(start,size)   do { (void) (start); (void) (size); } while (0)
#define dma_cache_inv(start,size)   do { (void) (start); (void) (size); } while (0)
#ifdef __MIPSEB__
#define __CSR_32_ADJUST 4
#else
#define __CSR_32_ADJUST 0
#endif
#define csr_out32(v, a) (*(volatile u32 *)((unsigned long)(a) + __CSR_32_ADJUST) = (v))
#define csr_in32(a) (*(volatile u32 *)((unsigned long)(a) + __CSR_32_ADJUST))
#define xlate_dev_mem_ptr(p) __va(p)
#define xlate_dev_kmem_ptr(p) p
#endif
