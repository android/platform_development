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
#ifndef _UAPI__LINUX_FUNCTIONFS_H__
#define _UAPI__LINUX_FUNCTIONFS_H__
#include <linux/types.h>
#include <linux/ioctl.h>
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#include <linux/usb/ch9.h>
enum {
  FUNCTIONFS_DESCRIPTORS_MAGIC = 1,
  FUNCTIONFS_STRINGS_MAGIC = 2,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  FUNCTIONFS_DESCRIPTORS_MAGIC_V2 = 3,
};
enum functionfs_flags {
  FUNCTIONFS_HAS_FS_DESC = 1,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  FUNCTIONFS_HAS_HS_DESC = 2,
  FUNCTIONFS_HAS_SS_DESC = 4,
  FUNCTIONFS_HAS_MS_OS_DESC = 8,
  FUNCTIONFS_VIRTUAL_ADDR = 16,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  FUNCTIONFS_EVENTFD = 32,
};
struct usb_endpoint_descriptor_no_audio {
  __u8 bLength;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u8 bDescriptorType;
  __u8 bEndpointAddress;
  __u8 bmAttributes;
  __le16 wMaxPacketSize;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u8 bInterval;
} __attribute__((packed));
struct usb_functionfs_descs_head_v2 {
  __le32 magic;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __le32 length;
  __le32 flags;
} __attribute__((packed));
struct usb_functionfs_descs_head {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __le32 magic;
  __le32 length;
  __le32 fs_count;
  __le32 hs_count;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
} __attribute__((packed, deprecated));
struct usb_os_desc_header {
  __u8 interface;
  __le32 dwLength;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __le16 bcdVersion;
  __le16 wIndex;
  union {
    struct {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
      __u8 bCount;
      __u8 Reserved;
    };
    __le16 wCount;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  };
} __attribute__((packed));
struct usb_ext_compat_desc {
  __u8 bFirstInterfaceNumber;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u8 Reserved1;
  __u8 CompatibleID[8];
  __u8 SubCompatibleID[8];
  __u8 Reserved2[6];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct usb_ext_prop_desc {
  __le32 dwSize;
  __le32 dwPropertyDataType;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __le16 wPropertyNameLength;
} __attribute__((packed));
struct usb_functionfs_strings_head {
  __le32 magic;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __le32 length;
  __le32 str_count;
  __le32 lang_count;
} __attribute__((packed));
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum usb_functionfs_event_type {
  FUNCTIONFS_BIND,
  FUNCTIONFS_UNBIND,
  FUNCTIONFS_ENABLE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  FUNCTIONFS_DISABLE,
  FUNCTIONFS_SETUP,
  FUNCTIONFS_SUSPEND,
  FUNCTIONFS_RESUME
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct usb_functionfs_event {
  union {
    struct usb_ctrlrequest setup;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  } __attribute__((packed)) u;
  __u8 type;
  __u8 _pad[3];
} __attribute__((packed));
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define FUNCTIONFS_FIFO_STATUS _IO('g', 1)
#define FUNCTIONFS_FIFO_FLUSH _IO('g', 2)
#define FUNCTIONFS_CLEAR_HALT _IO('g', 3)
#define FUNCTIONFS_INTERFACE_REVMAP _IO('g', 128)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define FUNCTIONFS_ENDPOINT_REVMAP _IO('g', 129)
#define FUNCTIONFS_ENDPOINT_DESC _IOR('g', 130, struct usb_endpoint_descriptor)
#endif
