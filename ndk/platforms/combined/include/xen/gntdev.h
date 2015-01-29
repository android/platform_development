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
#ifndef __LINUX_PUBLIC_GNTDEV_H__
#define __LINUX_PUBLIC_GNTDEV_H__
struct ioctl_gntdev_grant_ref {
 uint32_t domid;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t ref;
};
#define IOCTL_GNTDEV_MAP_GRANT_REF  _IOC(_IOC_NONE, 'G', 0, sizeof(struct ioctl_gntdev_map_grant_ref))
struct ioctl_gntdev_map_grant_ref {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t count;
 uint32_t pad;
 uint64_t index;
 struct ioctl_gntdev_grant_ref refs[1];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define IOCTL_GNTDEV_UNMAP_GRANT_REF  _IOC(_IOC_NONE, 'G', 1, sizeof(struct ioctl_gntdev_unmap_grant_ref))
struct ioctl_gntdev_unmap_grant_ref {
 uint64_t index;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t count;
 uint32_t pad;
};
#define IOCTL_GNTDEV_GET_OFFSET_FOR_VADDR  _IOC(_IOC_NONE, 'G', 2, sizeof(struct ioctl_gntdev_get_offset_for_vaddr))
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct ioctl_gntdev_get_offset_for_vaddr {
 uint64_t vaddr;
 uint64_t offset;
 uint32_t count;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t pad;
};
#define IOCTL_GNTDEV_SET_MAX_GRANTS  _IOC(_IOC_NONE, 'G', 3, sizeof(struct ioctl_gntdev_set_max_grants))
struct ioctl_gntdev_set_max_grants {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t count;
};
#define IOCTL_GNTDEV_SET_UNMAP_NOTIFY  _IOC(_IOC_NONE, 'G', 7, sizeof(struct ioctl_gntdev_unmap_notify))
struct ioctl_gntdev_unmap_notify {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint64_t index;
 uint32_t action;
 uint32_t event_channel_port;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define UNMAP_NOTIFY_CLEAR_BYTE 0x1
#define UNMAP_NOTIFY_SEND_EVENT 0x2
#endif
