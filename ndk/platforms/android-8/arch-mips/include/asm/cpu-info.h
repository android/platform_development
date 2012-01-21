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
#ifndef __ASM_CPU_INFO_H
#define __ASM_CPU_INFO_H

#include <asm/cache.h>

struct cache_desc {
 unsigned int waysize;
 unsigned short sets;
 unsigned char ways;
 unsigned char linesz;
 unsigned char waybit;
 unsigned char flags;
};

#define MIPS_CACHE_NOT_PRESENT 0x00000001
#define MIPS_CACHE_VTAG 0x00000002  
#define MIPS_CACHE_ALIASES 0x00000004  
#define MIPS_CACHE_IC_F_DC 0x00000008  
#define MIPS_IC_SNOOPS_REMOTE 0x00000010  
#define MIPS_CACHE_PINDEX 0x00000020  

struct cpuinfo_mips {
 unsigned long udelay_val;
 unsigned long asid_cache;

 unsigned long options;
 unsigned long ases;
 unsigned int processor_id;
 unsigned int fpu_id;
 unsigned int cputype;
 int isa_level;
 int tlbsize;
 struct cache_desc icache;
 struct cache_desc dcache;
 struct cache_desc scache;
 struct cache_desc tcache;
 int srsets;
 int core;
 void *data;
} __attribute__((aligned(SMP_CACHE_BYTES)));

#define current_cpu_data cpu_data[smp_processor_id()]
#define raw_current_cpu_data cpu_data[raw_smp_processor_id()]

#define cpu_name_string() __cpu_name[smp_processor_id()]

#endif
