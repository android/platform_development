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
#ifndef _LINUX_USERFAULTFD_H
#define _LINUX_USERFAULTFD_H
#include <linux/types.h>
#define UFFD_API ((__u64) 0xAA)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define UFFD_API_FEATURES (0)
#define UFFD_API_IOCTLS ((__u64) 1 << _UFFDIO_REGISTER | (__u64) 1 << _UFFDIO_UNREGISTER | (__u64) 1 << _UFFDIO_API)
#define UFFD_API_RANGE_IOCTLS ((__u64) 1 << _UFFDIO_WAKE | (__u64) 1 << _UFFDIO_COPY | (__u64) 1 << _UFFDIO_ZEROPAGE)
#define _UFFDIO_REGISTER (0x00)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define _UFFDIO_UNREGISTER (0x01)
#define _UFFDIO_WAKE (0x02)
#define _UFFDIO_COPY (0x03)
#define _UFFDIO_ZEROPAGE (0x04)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define _UFFDIO_API (0x3F)
#define UFFDIO 0xAA
#define UFFDIO_API _IOWR(UFFDIO, _UFFDIO_API, struct uffdio_api)
#define UFFDIO_REGISTER _IOWR(UFFDIO, _UFFDIO_REGISTER, struct uffdio_register)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define UFFDIO_UNREGISTER _IOR(UFFDIO, _UFFDIO_UNREGISTER, struct uffdio_range)
#define UFFDIO_WAKE _IOR(UFFDIO, _UFFDIO_WAKE, struct uffdio_range)
#define UFFDIO_COPY _IOWR(UFFDIO, _UFFDIO_COPY, struct uffdio_copy)
#define UFFDIO_ZEROPAGE _IOWR(UFFDIO, _UFFDIO_ZEROPAGE, struct uffdio_zeropage)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct uffd_msg {
  __u8 event;
  __u8 reserved1;
  __u16 reserved2;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 reserved3;
  union {
    struct {
      __u64 flags;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
      __u64 address;
    } pagefault;
    struct {
      __u64 reserved1;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
      __u64 reserved2;
      __u64 reserved3;
    } reserved;
  } arg;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
} __packed;
#define UFFD_EVENT_PAGEFAULT 0x12
#define UFFD_PAGEFAULT_FLAG_WRITE (1 << 0)
#define UFFD_PAGEFAULT_FLAG_WP (1 << 1)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct uffdio_api {
  __u64 api;
  __u64 features;
  __u64 ioctls;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct uffdio_range {
  __u64 start;
  __u64 len;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct uffdio_register {
  struct uffdio_range range;
#define UFFDIO_REGISTER_MODE_MISSING ((__u64) 1 << 0)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define UFFDIO_REGISTER_MODE_WP ((__u64) 1 << 1)
  __u64 mode;
  __u64 ioctls;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct uffdio_copy {
  __u64 dst;
  __u64 src;
  __u64 len;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define UFFDIO_COPY_MODE_DONTWAKE ((__u64) 1 << 0)
  __u64 mode;
  __s64 copy;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct uffdio_zeropage {
  struct uffdio_range range;
#define UFFDIO_ZEROPAGE_MODE_DONTWAKE ((__u64) 1 << 0)
  __u64 mode;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __s64 zeropage;
};
#endif
