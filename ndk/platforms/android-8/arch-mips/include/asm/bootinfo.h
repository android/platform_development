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
#ifndef _ASM_BOOTINFO_H
#define _ASM_BOOTINFO_H

#include <linux/types.h>
#include <asm/setup.h>

#define MACH_UNKNOWN 0  

#define MACH_ACER_PICA_61 0  
#define MACH_MIPS_MAGNUM_4000 1  
#define MACH_OLIVETTI_M700 2  

#define MACH_DSUNKNOWN 0
#define MACH_DS23100 1  
#define MACH_DS5100 2  
#define MACH_DS5000_200 3  
#define MACH_DS5000_1XX 4  
#define MACH_DS5000_XX 5  
#define MACH_DS5000_2X0 6  
#define MACH_DS5400 7  
#define MACH_DS5500 8  
#define MACH_DS5800 9  
#define MACH_DS5900 10  

#define MACH_SNI_RM200_PCI 0  

#define MACH_SGI_IP22 0  
#define MACH_SGI_IP27 1  
#define MACH_SGI_IP28 2  
#define MACH_SGI_IP32 3  
#define MACH_SGI_IP30 4  

#define MACH_PALLAS 0
#define MACH_TOPAS 1
#define MACH_JMR 2
#define MACH_TOSHIBA_JMR3927 3  
#define MACH_TOSHIBA_RBTX4927 4
#define MACH_TOSHIBA_RBTX4937 5
#define MACH_TOSHIBA_RBTX4938 6

#define MACH_LASAT_100 0  
#define MACH_LASAT_200 1  

#define MACH_NEC_MARKEINS 0  

#define MACH_MSP4200_EVAL 0  
#define MACH_MSP4200_GW 1  
#define MACH_MSP4200_FPGA 2  
#define MACH_MSP7120_EVAL 3  
#define MACH_MSP7120_GW 4  
#define MACH_MSP7120_FPGA 5  
#define MACH_MSP_OTHER 255  

#define CL_SIZE COMMAND_LINE_SIZE

#define BOOT_MEM_MAP_MAX 32
#define BOOT_MEM_RAM 1
#define BOOT_MEM_ROM_DATA 2
#define BOOT_MEM_RESERVED 3

struct boot_mem_map {
 int nr_map;
 struct boot_mem_map_entry {
 phys_t addr;
 phys_t size;
 long type;
 } map[BOOT_MEM_MAP_MAX];
};

#endif
