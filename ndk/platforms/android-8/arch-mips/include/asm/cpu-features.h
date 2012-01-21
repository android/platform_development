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
#ifndef __ASM_CPU_FEATURES_H
#define __ASM_CPU_FEATURES_H

#include <asm/cpu.h>
#include <asm/cpu-info.h>
#include <cpu-feature-overrides.h>

#ifndef current_cpu_type
#define current_cpu_type() current_cpu_data.cputype
#endif

#ifndef cpu_has_tlb
#define cpu_has_tlb (cpu_data[0].options & MIPS_CPU_TLB)
#endif
#ifndef cpu_has_4kex
#define cpu_has_4kex (cpu_data[0].options & MIPS_CPU_4KEX)
#endif
#ifndef cpu_has_3k_cache
#define cpu_has_3k_cache (cpu_data[0].options & MIPS_CPU_3K_CACHE)
#endif
#define cpu_has_6k_cache 0
#define cpu_has_8k_cache 0
#ifndef cpu_has_4k_cache
#define cpu_has_4k_cache (cpu_data[0].options & MIPS_CPU_4K_CACHE)
#endif
#ifndef cpu_has_tx39_cache
#define cpu_has_tx39_cache (cpu_data[0].options & MIPS_CPU_TX39_CACHE)
#endif
#ifndef cpu_has_fpu
#define cpu_has_fpu (current_cpu_data.options & MIPS_CPU_FPU)
#define raw_cpu_has_fpu (raw_current_cpu_data.options & MIPS_CPU_FPU)
#else
#define raw_cpu_has_fpu cpu_has_fpu
#endif
#ifndef cpu_has_32fpr
#define cpu_has_32fpr (cpu_data[0].options & MIPS_CPU_32FPR)
#endif
#ifndef cpu_has_counter
#define cpu_has_counter (cpu_data[0].options & MIPS_CPU_COUNTER)
#endif
#ifndef cpu_has_watch
#define cpu_has_watch (cpu_data[0].options & MIPS_CPU_WATCH)
#endif
#ifndef cpu_has_divec
#define cpu_has_divec (cpu_data[0].options & MIPS_CPU_DIVEC)
#endif
#ifndef cpu_has_vce
#define cpu_has_vce (cpu_data[0].options & MIPS_CPU_VCE)
#endif
#ifndef cpu_has_cache_cdex_p
#define cpu_has_cache_cdex_p (cpu_data[0].options & MIPS_CPU_CACHE_CDEX_P)
#endif
#ifndef cpu_has_cache_cdex_s
#define cpu_has_cache_cdex_s (cpu_data[0].options & MIPS_CPU_CACHE_CDEX_S)
#endif
#ifndef cpu_has_prefetch
#define cpu_has_prefetch (cpu_data[0].options & MIPS_CPU_PREFETCH)
#endif
#ifndef cpu_has_mcheck
#define cpu_has_mcheck (cpu_data[0].options & MIPS_CPU_MCHECK)
#endif
#ifndef cpu_has_ejtag
#define cpu_has_ejtag (cpu_data[0].options & MIPS_CPU_EJTAG)
#endif
#ifndef cpu_has_llsc
#define cpu_has_llsc (cpu_data[0].options & MIPS_CPU_LLSC)
#endif
#ifndef cpu_has_mips16
#define cpu_has_mips16 (cpu_data[0].ases & MIPS_ASE_MIPS16)
#endif
#ifndef cpu_has_mdmx
#define cpu_has_mdmx (cpu_data[0].ases & MIPS_ASE_MDMX)
#endif
#ifndef cpu_has_mips3d
#define cpu_has_mips3d (cpu_data[0].ases & MIPS_ASE_MIPS3D)
#endif
#ifndef cpu_has_smartmips
#define cpu_has_smartmips (cpu_data[0].ases & MIPS_ASE_SMARTMIPS)
#endif
#ifndef cpu_has_vtag_icache
#define cpu_has_vtag_icache (cpu_data[0].icache.flags & MIPS_CACHE_VTAG)
#endif
#ifndef cpu_has_dc_aliases
#define cpu_has_dc_aliases (cpu_data[0].dcache.flags & MIPS_CACHE_ALIASES)
#endif
#ifndef cpu_has_ic_fills_f_dc
#define cpu_has_ic_fills_f_dc (cpu_data[0].icache.flags & MIPS_CACHE_IC_F_DC)
#endif
#ifndef cpu_has_pindexed_dcache
#define cpu_has_pindexed_dcache (cpu_data[0].dcache.flags & MIPS_CACHE_PINDEX)
#endif

#ifndef cpu_icache_snoops_remote_store
#define cpu_icache_snoops_remote_store 1
#endif

#ifndef cpu_has_mips32r1
#define cpu_has_mips32r1 (cpu_data[0].isa_level & MIPS_CPU_ISA_M32R1)
#endif
#ifndef cpu_has_mips32r2
#define cpu_has_mips32r2 (cpu_data[0].isa_level & MIPS_CPU_ISA_M32R2)
#endif
#ifndef cpu_has_mips64r1
#define cpu_has_mips64r1 (cpu_data[0].isa_level & MIPS_CPU_ISA_M64R1)
#endif
#ifndef cpu_has_mips64r2
#define cpu_has_mips64r2 (cpu_data[0].isa_level & MIPS_CPU_ISA_M64R2)
#endif

#define cpu_has_mips32 (cpu_has_mips32r1 | cpu_has_mips32r2)
#define cpu_has_mips64 (cpu_has_mips64r1 | cpu_has_mips64r2)
#define cpu_has_mips_r1 (cpu_has_mips32r1 | cpu_has_mips64r1)
#define cpu_has_mips_r2 (cpu_has_mips32r2 | cpu_has_mips64r2)

#ifndef cpu_has_dsp
#define cpu_has_dsp (cpu_data[0].ases & MIPS_ASE_DSP)
#endif

#ifndef cpu_has_mipsmt
#define cpu_has_mipsmt (cpu_data[0].ases & MIPS_ASE_MIPSMT)
#endif

#ifndef cpu_has_userlocal
#define cpu_has_userlocal (cpu_data[0].options & MIPS_CPU_ULRI)
#endif

#ifndef cpu_has_vint
#define cpu_has_vint 0
#endif

#ifndef cpu_has_veic
#define cpu_has_veic 0
#endif

#ifndef cpu_has_inclusive_pcaches
#define cpu_has_inclusive_pcaches (cpu_data[0].options & MIPS_CPU_INCLUSIVE_CACHES)
#endif

#ifndef cpu_dcache_line_size
#define cpu_dcache_line_size() cpu_data[0].dcache.linesz
#endif
#ifndef cpu_icache_line_size
#define cpu_icache_line_size() cpu_data[0].icache.linesz
#endif
#ifndef cpu_scache_line_size
#define cpu_scache_line_size() cpu_data[0].scache.linesz
#endif

#endif
