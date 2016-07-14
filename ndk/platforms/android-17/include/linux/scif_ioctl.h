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
#ifndef SCIF_IOCTL_H
#define SCIF_IOCTL_H
#include <linux/types.h>
struct scif_port_id {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u16 node;
  __u16 port;
};
struct scifioctl_connect {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct scif_port_id self;
  struct scif_port_id peer;
};
struct scifioctl_accept {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __s32 flags;
  struct scif_port_id peer;
  __u64 endpt;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct scifioctl_msg {
  __u64 msg;
  __s32 len;
  __s32 flags;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __s32 out_len;
};
struct scifioctl_reg {
  __u64 addr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u64 len;
  __s64 offset;
  __s32 prot;
  __s32 flags;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __s64 out_offset;
};
struct scifioctl_unreg {
  __s64 offset;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u64 len;
};
struct scifioctl_copy {
  __s64 loffset;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u64 len;
  __s64 roffset;
  __u64 addr;
  __s32 flags;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct scifioctl_fence_mark {
  __s32 flags;
  __u64 mark;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct scifioctl_fence_signal {
  __s64 loff;
  __u64 lval;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __s64 roff;
  __u64 rval;
  __s32 flags;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct scifioctl_node_ids {
  __u64 nodes;
  __u64 self;
  __s32 len;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define SCIF_BIND _IOWR('s', 1, __u64)
#define SCIF_LISTEN _IOW('s', 2, __s32)
#define SCIF_CONNECT _IOWR('s', 3, struct scifioctl_connect)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define SCIF_ACCEPTREQ _IOWR('s', 4, struct scifioctl_accept)
#define SCIF_ACCEPTREG _IOWR('s', 5, __u64)
#define SCIF_SEND _IOWR('s', 6, struct scifioctl_msg)
#define SCIF_RECV _IOWR('s', 7, struct scifioctl_msg)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define SCIF_REG _IOWR('s', 8, struct scifioctl_reg)
#define SCIF_UNREG _IOWR('s', 9, struct scifioctl_unreg)
#define SCIF_READFROM _IOWR('s', 10, struct scifioctl_copy)
#define SCIF_WRITETO _IOWR('s', 11, struct scifioctl_copy)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define SCIF_VREADFROM _IOWR('s', 12, struct scifioctl_copy)
#define SCIF_VWRITETO _IOWR('s', 13, struct scifioctl_copy)
#define SCIF_GET_NODEIDS _IOWR('s', 14, struct scifioctl_node_ids)
#define SCIF_FENCE_MARK _IOWR('s', 15, struct scifioctl_fence_mark)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define SCIF_FENCE_WAIT _IOWR('s', 16, __s32)
#define SCIF_FENCE_SIGNAL _IOWR('s', 17, struct scifioctl_fence_signal)
#endif
