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
#ifndef __OMAP_DRM_H__
#define __OMAP_DRM_H__
#include <drm/drm.h>
#define OMAP_PARAM_CHIPSET_ID 1
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct drm_omap_param {
 uint64_t param;
 uint64_t value;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define OMAP_BO_SCANOUT 0x00000001
#define OMAP_BO_CACHE_MASK 0x00000006
#define OMAP_BO_TILED_MASK 0x00000f00
#define OMAP_BO_CACHED 0x00000000
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define OMAP_BO_WC 0x00000002
#define OMAP_BO_UNCACHED 0x00000004
#define OMAP_BO_TILED_8 0x00000100
#define OMAP_BO_TILED_16 0x00000200
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define OMAP_BO_TILED_32 0x00000300
#define OMAP_BO_TILED (OMAP_BO_TILED_8 | OMAP_BO_TILED_16 | OMAP_BO_TILED_32)
union omap_gem_size {
 uint32_t bytes;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct {
 uint16_t width;
 uint16_t height;
 } tiled;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct drm_omap_gem_new {
 union omap_gem_size size;
 uint32_t flags;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t handle;
 uint32_t __pad;
};
enum omap_gem_op {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 OMAP_GEM_READ = 0x01,
 OMAP_GEM_WRITE = 0x02,
};
struct drm_omap_gem_cpu_prep {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t handle;
 uint32_t op;
};
struct drm_omap_gem_cpu_fini {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t handle;
 uint32_t op;
 uint32_t nregions;
 uint32_t __pad;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct drm_omap_gem_info {
 uint32_t handle;
 uint32_t pad;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint64_t offset;
 uint32_t size;
 uint32_t __pad;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DRM_OMAP_GET_PARAM 0x00
#define DRM_OMAP_SET_PARAM 0x01
#define DRM_OMAP_GEM_NEW 0x03
#define DRM_OMAP_GEM_CPU_PREP 0x04
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DRM_OMAP_GEM_CPU_FINI 0x05
#define DRM_OMAP_GEM_INFO 0x06
#define DRM_OMAP_NUM_IOCTLS 0x07
#define DRM_IOCTL_OMAP_GET_PARAM DRM_IOWR(DRM_COMMAND_BASE + DRM_OMAP_GET_PARAM, struct drm_omap_param)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DRM_IOCTL_OMAP_SET_PARAM DRM_IOW (DRM_COMMAND_BASE + DRM_OMAP_SET_PARAM, struct drm_omap_param)
#define DRM_IOCTL_OMAP_GEM_NEW DRM_IOWR(DRM_COMMAND_BASE + DRM_OMAP_GEM_NEW, struct drm_omap_gem_new)
#define DRM_IOCTL_OMAP_GEM_CPU_PREP DRM_IOW (DRM_COMMAND_BASE + DRM_OMAP_GEM_CPU_PREP, struct drm_omap_gem_cpu_prep)
#define DRM_IOCTL_OMAP_GEM_CPU_FINI DRM_IOW (DRM_COMMAND_BASE + DRM_OMAP_GEM_CPU_FINI, struct drm_omap_gem_cpu_fini)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DRM_IOCTL_OMAP_GEM_INFO DRM_IOWR(DRM_COMMAND_BASE + DRM_OMAP_GEM_INFO, struct drm_omap_gem_info)
#endif
