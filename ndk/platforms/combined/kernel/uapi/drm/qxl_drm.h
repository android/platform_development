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
#ifndef QXL_DRM_H
#define QXL_DRM_H
#include <stddef.h>
#include "drm/drm.h"
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define QXL_GEM_DOMAIN_CPU 0
#define QXL_GEM_DOMAIN_VRAM 1
#define QXL_GEM_DOMAIN_SURFACE 2
#define DRM_QXL_ALLOC 0x00
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DRM_QXL_MAP 0x01
#define DRM_QXL_EXECBUFFER 0x02
#define DRM_QXL_UPDATE_AREA 0x03
#define DRM_QXL_GETPARAM 0x04
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DRM_QXL_CLIENTCAP 0x05
#define DRM_QXL_ALLOC_SURF 0x06
struct drm_qxl_alloc {
 uint32_t size;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t handle;
};
struct drm_qxl_map {
 uint64_t offset;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t handle;
 uint32_t pad;
};
#define QXL_RELOC_TYPE_BO 1
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define QXL_RELOC_TYPE_SURF 2
struct drm_qxl_reloc {
 uint64_t src_offset;
 uint64_t dst_offset;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t src_handle;
 uint32_t dst_handle;
 uint32_t reloc_type;
 uint32_t pad;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct drm_qxl_command {
 uint64_t __user command;
 uint64_t __user relocs;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t type;
 uint32_t command_size;
 uint32_t relocs_num;
 uint32_t pad;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct drm_qxl_execbuffer {
 uint32_t flags;
 uint32_t commands_num;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint64_t __user commands;
};
struct drm_qxl_update_area {
 uint32_t handle;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t top;
 uint32_t left;
 uint32_t bottom;
 uint32_t right;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t pad;
};
#define QXL_PARAM_NUM_SURFACES 1
#define QXL_PARAM_MAX_RELOCS 2
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct drm_qxl_getparam {
 uint64_t param;
 uint64_t value;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct drm_qxl_clientcap {
 uint32_t index;
 uint32_t pad;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct drm_qxl_alloc_surf {
 uint32_t format;
 uint32_t width;
 uint32_t height;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 int32_t stride;
 uint32_t handle;
 uint32_t pad;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DRM_IOCTL_QXL_ALLOC   DRM_IOWR(DRM_COMMAND_BASE + DRM_QXL_ALLOC, struct drm_qxl_alloc)
#define DRM_IOCTL_QXL_MAP   DRM_IOWR(DRM_COMMAND_BASE + DRM_QXL_MAP, struct drm_qxl_map)
#define DRM_IOCTL_QXL_EXECBUFFER   DRM_IOW(DRM_COMMAND_BASE + DRM_QXL_EXECBUFFER,  struct drm_qxl_execbuffer)
#define DRM_IOCTL_QXL_UPDATE_AREA   DRM_IOW(DRM_COMMAND_BASE + DRM_QXL_UPDATE_AREA,  struct drm_qxl_update_area)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DRM_IOCTL_QXL_GETPARAM   DRM_IOWR(DRM_COMMAND_BASE + DRM_QXL_GETPARAM,  struct drm_qxl_getparam)
#define DRM_IOCTL_QXL_CLIENTCAP   DRM_IOW(DRM_COMMAND_BASE + DRM_QXL_CLIENTCAP,  struct drm_qxl_clientcap)
#define DRM_IOCTL_QXL_ALLOC_SURF   DRM_IOWR(DRM_COMMAND_BASE + DRM_QXL_ALLOC_SURF,  struct drm_qxl_alloc_surf)
#endif
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
