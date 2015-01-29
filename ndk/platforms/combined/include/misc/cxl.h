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
#ifndef _UAPI_MISC_CXL_H
#define _UAPI_MISC_CXL_H
#include <linux/types.h>
#include <linux/ioctl.h>
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct cxl_ioctl_start_work {
 __u64 flags;
 __u64 work_element_descriptor;
 __u64 amr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __s16 num_interrupts;
 __s16 reserved1;
 __s32 reserved2;
 __u64 reserved3;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u64 reserved4;
 __u64 reserved5;
 __u64 reserved6;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define CXL_START_WORK_AMR 0x0000000000000001ULL
#define CXL_START_WORK_NUM_IRQS 0x0000000000000002ULL
#define CXL_START_WORK_ALL (CXL_START_WORK_AMR |  CXL_START_WORK_NUM_IRQS)
#define CXL_MAGIC 0xCA
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define CXL_IOCTL_START_WORK _IOW(CXL_MAGIC, 0x00, struct cxl_ioctl_start_work)
#define CXL_IOCTL_GET_PROCESS_ELEMENT _IOR(CXL_MAGIC, 0x01, __u32)
#define CXL_READ_MIN_SIZE 0x1000
enum cxl_event_type {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 CXL_EVENT_RESERVED = 0,
 CXL_EVENT_AFU_INTERRUPT = 1,
 CXL_EVENT_DATA_STORAGE = 2,
 CXL_EVENT_AFU_ERROR = 3,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct cxl_event_header {
 __u16 type;
 __u16 size;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u16 process_element;
 __u16 reserved1;
};
struct cxl_event_afu_interrupt {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u16 flags;
 __u16 irq;
 __u32 reserved1;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct cxl_event_data_storage {
 __u16 flags;
 __u16 reserved1;
 __u32 reserved2;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u64 addr;
 __u64 dsisr;
 __u64 reserved3;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct cxl_event_afu_error {
 __u16 flags;
 __u16 reserved1;
 __u32 reserved2;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u64 error;
};
struct cxl_event {
 struct cxl_event_header header;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 union {
 struct cxl_event_afu_interrupt irq;
 struct cxl_event_data_storage fault;
 struct cxl_event_afu_error afu_error;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 };
};
#endif
