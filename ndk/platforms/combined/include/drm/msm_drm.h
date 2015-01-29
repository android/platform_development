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
#ifndef __MSM_DRM_H__
#define __MSM_DRM_H__
#include <stddef.h>
#include <drm/drm.h>
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define MSM_PIPE_NONE 0x00
#define MSM_PIPE_2D0 0x01
#define MSM_PIPE_2D1 0x02
#define MSM_PIPE_3D0 0x10
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct drm_msm_timespec {
 int64_t tv_sec;
 int64_t tv_nsec;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define MSM_PARAM_GPU_ID 0x01
#define MSM_PARAM_GMEM_SIZE 0x02
#define MSM_PARAM_CHIP_ID 0x03
struct drm_msm_param {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t pipe;
 uint32_t param;
 uint64_t value;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define MSM_BO_SCANOUT 0x00000001
#define MSM_BO_GPU_READONLY 0x00000002
#define MSM_BO_CACHE_MASK 0x000f0000
#define MSM_BO_CACHED 0x00010000
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define MSM_BO_WC 0x00020000
#define MSM_BO_UNCACHED 0x00040000
#define MSM_BO_FLAGS (MSM_BO_SCANOUT |   MSM_BO_GPU_READONLY |   MSM_BO_CACHED |   MSM_BO_WC |   MSM_BO_UNCACHED)
struct drm_msm_gem_new {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint64_t size;
 uint32_t flags;
 uint32_t handle;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct drm_msm_gem_info {
 uint32_t handle;
 uint32_t pad;
 uint64_t offset;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define MSM_PREP_READ 0x01
#define MSM_PREP_WRITE 0x02
#define MSM_PREP_NOSYNC 0x04
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define MSM_PREP_FLAGS (MSM_PREP_READ | MSM_PREP_WRITE | MSM_PREP_NOSYNC)
struct drm_msm_gem_cpu_prep {
 uint32_t handle;
 uint32_t op;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct drm_msm_timespec timeout;
};
struct drm_msm_gem_cpu_fini {
 uint32_t handle;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct drm_msm_gem_submit_reloc {
 uint32_t submit_offset;
 uint32_t or;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 int32_t shift;
 uint32_t reloc_idx;
 uint64_t reloc_offset;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define MSM_SUBMIT_CMD_BUF 0x0001
#define MSM_SUBMIT_CMD_IB_TARGET_BUF 0x0002
#define MSM_SUBMIT_CMD_CTX_RESTORE_BUF 0x0003
struct drm_msm_gem_submit_cmd {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t type;
 uint32_t submit_idx;
 uint32_t submit_offset;
 uint32_t size;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t pad;
 uint32_t nr_relocs;
 uint64_t __user relocs;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define MSM_SUBMIT_BO_READ 0x0001
#define MSM_SUBMIT_BO_WRITE 0x0002
#define MSM_SUBMIT_BO_FLAGS (MSM_SUBMIT_BO_READ | MSM_SUBMIT_BO_WRITE)
struct drm_msm_gem_submit_bo {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t flags;
 uint32_t handle;
 uint64_t presumed;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct drm_msm_gem_submit {
 uint32_t pipe;
 uint32_t fence;
 uint32_t nr_bos;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t nr_cmds;
 uint64_t __user bos;
 uint64_t __user cmds;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct drm_msm_wait_fence {
 uint32_t fence;
 uint32_t pad;
 struct drm_msm_timespec timeout;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define DRM_MSM_GET_PARAM 0x00
#define DRM_MSM_GEM_NEW 0x02
#define DRM_MSM_GEM_INFO 0x03
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DRM_MSM_GEM_CPU_PREP 0x04
#define DRM_MSM_GEM_CPU_FINI 0x05
#define DRM_MSM_GEM_SUBMIT 0x06
#define DRM_MSM_WAIT_FENCE 0x07
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DRM_MSM_NUM_IOCTLS 0x08
#define DRM_IOCTL_MSM_GET_PARAM DRM_IOWR(DRM_COMMAND_BASE + DRM_MSM_GET_PARAM, struct drm_msm_param)
#define DRM_IOCTL_MSM_GEM_NEW DRM_IOWR(DRM_COMMAND_BASE + DRM_MSM_GEM_NEW, struct drm_msm_gem_new)
#define DRM_IOCTL_MSM_GEM_INFO DRM_IOWR(DRM_COMMAND_BASE + DRM_MSM_GEM_INFO, struct drm_msm_gem_info)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DRM_IOCTL_MSM_GEM_CPU_PREP DRM_IOW (DRM_COMMAND_BASE + DRM_MSM_GEM_CPU_PREP, struct drm_msm_gem_cpu_prep)
#define DRM_IOCTL_MSM_GEM_CPU_FINI DRM_IOW (DRM_COMMAND_BASE + DRM_MSM_GEM_CPU_FINI, struct drm_msm_gem_cpu_fini)
#define DRM_IOCTL_MSM_GEM_SUBMIT DRM_IOWR(DRM_COMMAND_BASE + DRM_MSM_GEM_SUBMIT, struct drm_msm_gem_submit)
#define DRM_IOCTL_MSM_WAIT_FENCE DRM_IOW (DRM_COMMAND_BASE + DRM_MSM_WAIT_FENCE, struct drm_msm_wait_fence)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#endif
