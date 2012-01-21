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
#ifndef _ASM_SN_ARCH_H
#define _ASM_SN_ARCH_H

#include <linux/types.h>
#include <asm/sn/types.h>

typedef u64 hubreg_t;

#define cputonasid(cpu) (sn_cpu_info[(cpu)].p_nasid)
#define cputoslice(cpu) (sn_cpu_info[(cpu)].p_slice)
#define makespnum(_nasid, _slice)   (((_nasid) << CPUS_PER_NODE_SHFT) | (_slice))

#define INVALID_NASID (nasid_t)-1
#define INVALID_CNODEID (cnodeid_t)-1
#define INVALID_PNODEID (pnodeid_t)-1
#define INVALID_MODULE (moduleid_t)-1
#define INVALID_PARTID (partid_t)-1

#define NASID_TO_REGION(nnode)   ((nnode) >>   (is_fine_dirmode() ? NASID_TO_FINEREG_SHFT : NASID_TO_COARSEREG_SHFT))

#define NASID_TO_COMPACT_NODEID(nnode) (nasid_to_compact_node[nnode])
#define COMPACT_TO_NASID_NODEID(cnode) (compact_to_nasid_node[cnode])
#define CPUID_TO_COMPACT_NODEID(cpu) (cpuid_to_compact_node[(cpu)])

#endif
