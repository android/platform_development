/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ***   To edit the content of this header, modify the corresponding
 ***   source file (e.g. under external/kernel-headers/original/) then
 ***   run bionic/libc/kernel/tools/update_all.py
 ***
 ***   Any manual change here will be lost the next time this script will
 ***   be run. You've been warned!
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef _UAPI_LINUX_ION_TEST_H
#define _UAPI_LINUX_ION_TEST_H
#include <linux/ioctl.h>
#include <linux/types.h>
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct ion_test_rw_data {
 __u64 ptr;
 __u64 offset;
 __u64 size;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 int write;
 int __padding;
};
#define ION_IOC_MAGIC 'I'
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define ION_IOC_TEST_SET_FD   _IO(ION_IOC_MAGIC, 0xf0)
#define ION_IOC_TEST_DMA_MAPPING   _IOW(ION_IOC_MAGIC, 0xf1, struct ion_test_rw_data)
#define ION_IOC_TEST_KERNEL_MAPPING   _IOW(ION_IOC_MAGIC, 0xf2, struct ion_test_rw_data)
#endif
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
