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
#ifndef __NDCTL_H__
#define __NDCTL_H__
#include <linux/types.h>
struct nd_cmd_smart {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 status;
  __u8 data[128];
} __packed;
struct nd_cmd_smart_threshold {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 status;
  __u8 data[8];
} __packed;
struct nd_cmd_dimm_flags {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 status;
  __u32 flags;
} __packed;
struct nd_cmd_get_config_size {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 status;
  __u32 config_size;
  __u32 max_xfer;
} __packed;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct nd_cmd_get_config_data_hdr {
  __u32 in_offset;
  __u32 in_length;
  __u32 status;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u8 out_buf[0];
} __packed;
struct nd_cmd_set_config_hdr {
  __u32 in_offset;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 in_length;
  __u8 in_buf[0];
} __packed;
struct nd_cmd_vendor_hdr {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 opcode;
  __u32 in_length;
  __u8 in_buf[0];
} __packed;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct nd_cmd_vendor_tail {
  __u32 status;
  __u32 out_length;
  __u8 out_buf[0];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
} __packed;
struct nd_cmd_ars_cap {
  __u64 address;
  __u64 length;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 status;
  __u32 max_ars_out;
} __packed;
struct nd_cmd_ars_start {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u64 address;
  __u64 length;
  __u16 type;
  __u8 reserved[6];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 status;
} __packed;
struct nd_cmd_ars_status {
  __u32 status;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 out_length;
  __u64 address;
  __u64 length;
  __u16 type;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 num_records;
  struct nd_ars_record {
    __u32 handle;
    __u32 flags;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u64 err_address;
    __u64 length;
  } __packed records[0];
} __packed;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum {
  ND_CMD_IMPLEMENTED = 0,
  ND_CMD_ARS_CAP = 1,
  ND_CMD_ARS_START = 2,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  ND_CMD_ARS_STATUS = 3,
  ND_CMD_SMART = 1,
  ND_CMD_SMART_THRESHOLD = 2,
  ND_CMD_DIMM_FLAGS = 3,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  ND_CMD_GET_CONFIG_SIZE = 4,
  ND_CMD_GET_CONFIG_DATA = 5,
  ND_CMD_SET_CONFIG_DATA = 6,
  ND_CMD_VENDOR_EFFECT_LOG_SIZE = 7,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  ND_CMD_VENDOR_EFFECT_LOG = 8,
  ND_CMD_VENDOR = 9,
};
enum {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  ND_ARS_VOLATILE = 1,
  ND_ARS_PERSISTENT = 2,
};
#define ND_IOCTL 'N'
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define ND_IOCTL_SMART _IOWR(ND_IOCTL, ND_CMD_SMART, struct nd_cmd_smart)
#define ND_IOCTL_SMART_THRESHOLD _IOWR(ND_IOCTL, ND_CMD_SMART_THRESHOLD, struct nd_cmd_smart_threshold)
#define ND_IOCTL_DIMM_FLAGS _IOWR(ND_IOCTL, ND_CMD_DIMM_FLAGS, struct nd_cmd_dimm_flags)
#define ND_IOCTL_GET_CONFIG_SIZE _IOWR(ND_IOCTL, ND_CMD_GET_CONFIG_SIZE, struct nd_cmd_get_config_size)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define ND_IOCTL_GET_CONFIG_DATA _IOWR(ND_IOCTL, ND_CMD_GET_CONFIG_DATA, struct nd_cmd_get_config_data_hdr)
#define ND_IOCTL_SET_CONFIG_DATA _IOWR(ND_IOCTL, ND_CMD_SET_CONFIG_DATA, struct nd_cmd_set_config_hdr)
#define ND_IOCTL_VENDOR _IOWR(ND_IOCTL, ND_CMD_VENDOR, struct nd_cmd_vendor_hdr)
#define ND_IOCTL_ARS_CAP _IOWR(ND_IOCTL, ND_CMD_ARS_CAP, struct nd_cmd_ars_cap)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define ND_IOCTL_ARS_START _IOWR(ND_IOCTL, ND_CMD_ARS_START, struct nd_cmd_ars_start)
#define ND_IOCTL_ARS_STATUS _IOWR(ND_IOCTL, ND_CMD_ARS_STATUS, struct nd_cmd_ars_status)
#define ND_DEVICE_DIMM 1
#define ND_DEVICE_REGION_PMEM 2
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define ND_DEVICE_REGION_BLK 3
#define ND_DEVICE_NAMESPACE_IO 4
#define ND_DEVICE_NAMESPACE_PMEM 5
#define ND_DEVICE_NAMESPACE_BLK 6
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum nd_driver_flags {
  ND_DRIVER_DIMM = 1 << ND_DEVICE_DIMM,
  ND_DRIVER_REGION_PMEM = 1 << ND_DEVICE_REGION_PMEM,
  ND_DRIVER_REGION_BLK = 1 << ND_DEVICE_REGION_BLK,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  ND_DRIVER_NAMESPACE_IO = 1 << ND_DEVICE_NAMESPACE_IO,
  ND_DRIVER_NAMESPACE_PMEM = 1 << ND_DEVICE_NAMESPACE_PMEM,
  ND_DRIVER_NAMESPACE_BLK = 1 << ND_DEVICE_NAMESPACE_BLK,
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum {
  ND_MIN_NAMESPACE_SIZE = 0x00400000,
};
enum ars_masks {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  ARS_STATUS_MASK = 0x0000FFFF,
  ARS_EXT_STATUS_SHIFT = 16,
};
#endif
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
