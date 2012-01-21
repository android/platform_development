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
#ifndef __ASM_CACHEOPS_H
#define __ASM_CACHEOPS_H

#define Index_Invalidate_I 0x00
#define Index_Writeback_Inv_D 0x01
#define Index_Load_Tag_I 0x04
#define Index_Load_Tag_D 0x05
#define Index_Store_Tag_I 0x08
#define Index_Store_Tag_D 0x09
#define Hit_Invalidate_I 0x10
#define Hit_Invalidate_D 0x11
#define Hit_Writeback_Inv_D 0x15

#define Create_Dirty_Excl_D 0x0d
#define Fill 0x14
#define Hit_Writeback_I 0x18
#define Hit_Writeback_D 0x19

#define Index_Invalidate_SI 0x02
#define Index_Writeback_Inv_SD 0x03
#define Index_Load_Tag_SI 0x06
#define Index_Load_Tag_SD 0x07
#define Index_Store_Tag_SI 0x0A
#define Index_Store_Tag_SD 0x0B
#define Create_Dirty_Excl_SD 0x0f
#define Hit_Invalidate_SI 0x12
#define Hit_Invalidate_SD 0x13
#define Hit_Writeback_Inv_SD 0x17
#define Hit_Writeback_SD 0x1b
#define Hit_Set_Virtual_SI 0x1e
#define Hit_Set_Virtual_SD 0x1f

#define R5K_Page_Invalidate_S 0x17

#define Page_Invalidate_T 0x16

#define Index_Writeback_Inv_S 0x03
#define Index_Load_Tag_S 0x07
#define Index_Store_Tag_S 0x0B
#define Hit_Invalidate_S 0x13
#define Cache_Barrier 0x14
#define Hit_Writeback_Inv_S 0x17
#define Index_Load_Data_I 0x18
#define Index_Load_Data_D 0x19
#define Index_Load_Data_S 0x1b
#define Index_Store_Data_I 0x1c
#define Index_Store_Data_D 0x1d
#define Index_Store_Data_S 0x1f

#endif
