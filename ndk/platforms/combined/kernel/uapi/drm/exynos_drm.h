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
#ifndef _UAPI_EXYNOS_DRM_H_
#define _UAPI_EXYNOS_DRM_H_
#include <drm/drm.h>
struct drm_exynos_gem_create {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint64_t size;
 unsigned int flags;
 unsigned int handle;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct drm_exynos_gem_info {
 unsigned int handle;
 unsigned int flags;
 uint64_t size;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct drm_exynos_vidi_connection {
 unsigned int connection;
 unsigned int extensions;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint64_t edid;
};
enum e_drm_exynos_gem_mem_type {
 EXYNOS_BO_CONTIG = 0 << 0,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 EXYNOS_BO_NONCONTIG = 1 << 0,
 EXYNOS_BO_NONCACHABLE = 0 << 1,
 EXYNOS_BO_CACHABLE = 1 << 1,
 EXYNOS_BO_WC = 1 << 2,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 EXYNOS_BO_MASK = EXYNOS_BO_NONCONTIG | EXYNOS_BO_CACHABLE |
 EXYNOS_BO_WC
};
struct drm_exynos_g2d_get_ver {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 major;
 __u32 minor;
};
struct drm_exynos_g2d_cmd {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 offset;
 __u32 data;
};
enum drm_exynos_g2d_buf_type {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 G2D_BUF_USERPTR = 1 << 31,
};
enum drm_exynos_g2d_event_type {
 G2D_EVENT_NOT,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 G2D_EVENT_NONSTOP,
 G2D_EVENT_STOP,
};
struct drm_exynos_g2d_userptr {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned long userptr;
 unsigned long size;
};
struct drm_exynos_g2d_set_cmdlist {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u64 cmd;
 __u64 cmd_buf;
 __u32 cmd_nr;
 __u32 cmd_buf_nr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u64 event_type;
 __u64 user_data;
};
struct drm_exynos_g2d_exec {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u64 async;
};
enum drm_exynos_ops_id {
 EXYNOS_DRM_OPS_SRC,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 EXYNOS_DRM_OPS_DST,
 EXYNOS_DRM_OPS_MAX,
};
struct drm_exynos_sz {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 hsize;
 __u32 vsize;
};
struct drm_exynos_pos {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 x;
 __u32 y;
 __u32 w;
 __u32 h;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
enum drm_exynos_flip {
 EXYNOS_DRM_FLIP_NONE = (0 << 0),
 EXYNOS_DRM_FLIP_VERTICAL = (1 << 0),
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 EXYNOS_DRM_FLIP_HORIZONTAL = (1 << 1),
 EXYNOS_DRM_FLIP_BOTH = EXYNOS_DRM_FLIP_VERTICAL |
 EXYNOS_DRM_FLIP_HORIZONTAL,
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum drm_exynos_degree {
 EXYNOS_DRM_DEGREE_0,
 EXYNOS_DRM_DEGREE_90,
 EXYNOS_DRM_DEGREE_180,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 EXYNOS_DRM_DEGREE_270,
};
enum drm_exynos_planer {
 EXYNOS_DRM_PLANAR_Y,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 EXYNOS_DRM_PLANAR_CB,
 EXYNOS_DRM_PLANAR_CR,
 EXYNOS_DRM_PLANAR_MAX,
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct drm_exynos_ipp_prop_list {
 __u32 version;
 __u32 ipp_id;
 __u32 count;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 writeback;
 __u32 flip;
 __u32 degree;
 __u32 csc;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 crop;
 __u32 scale;
 __u32 refresh_min;
 __u32 refresh_max;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 reserved;
 struct drm_exynos_sz crop_min;
 struct drm_exynos_sz crop_max;
 struct drm_exynos_sz scale_min;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct drm_exynos_sz scale_max;
};
struct drm_exynos_ipp_config {
 enum drm_exynos_ops_id ops_id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 enum drm_exynos_flip flip;
 enum drm_exynos_degree degree;
 __u32 fmt;
 struct drm_exynos_sz sz;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct drm_exynos_pos pos;
};
enum drm_exynos_ipp_cmd {
 IPP_CMD_NONE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 IPP_CMD_M2M,
 IPP_CMD_WB,
 IPP_CMD_OUTPUT,
 IPP_CMD_MAX,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct drm_exynos_ipp_property {
 struct drm_exynos_ipp_config config[EXYNOS_DRM_OPS_MAX];
 enum drm_exynos_ipp_cmd cmd;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 ipp_id;
 __u32 prop_id;
 __u32 refresh_rate;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum drm_exynos_ipp_buf_type {
 IPP_BUF_ENQUEUE,
 IPP_BUF_DEQUEUE,
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct drm_exynos_ipp_queue_buf {
 enum drm_exynos_ops_id ops_id;
 enum drm_exynos_ipp_buf_type buf_type;
 __u32 prop_id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 buf_id;
 __u32 handle[EXYNOS_DRM_PLANAR_MAX];
 __u32 reserved;
 __u64 user_data;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
enum drm_exynos_ipp_ctrl {
 IPP_CTRL_PLAY,
 IPP_CTRL_STOP,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 IPP_CTRL_PAUSE,
 IPP_CTRL_RESUME,
 IPP_CTRL_MAX,
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct drm_exynos_ipp_cmd_ctrl {
 __u32 prop_id;
 enum drm_exynos_ipp_ctrl ctrl;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DRM_EXYNOS_GEM_CREATE 0x00
#define DRM_EXYNOS_GEM_GET 0x04
#define DRM_EXYNOS_VIDI_CONNECTION 0x07
#define DRM_EXYNOS_G2D_GET_VER 0x20
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DRM_EXYNOS_G2D_SET_CMDLIST 0x21
#define DRM_EXYNOS_G2D_EXEC 0x22
#define DRM_EXYNOS_IPP_GET_PROPERTY 0x30
#define DRM_EXYNOS_IPP_SET_PROPERTY 0x31
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DRM_EXYNOS_IPP_QUEUE_BUF 0x32
#define DRM_EXYNOS_IPP_CMD_CTRL 0x33
#define DRM_IOCTL_EXYNOS_GEM_CREATE DRM_IOWR(DRM_COMMAND_BASE +   DRM_EXYNOS_GEM_CREATE, struct drm_exynos_gem_create)
#define DRM_IOCTL_EXYNOS_GEM_GET DRM_IOWR(DRM_COMMAND_BASE +   DRM_EXYNOS_GEM_GET, struct drm_exynos_gem_info)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DRM_IOCTL_EXYNOS_VIDI_CONNECTION DRM_IOWR(DRM_COMMAND_BASE +   DRM_EXYNOS_VIDI_CONNECTION, struct drm_exynos_vidi_connection)
#define DRM_IOCTL_EXYNOS_G2D_GET_VER DRM_IOWR(DRM_COMMAND_BASE +   DRM_EXYNOS_G2D_GET_VER, struct drm_exynos_g2d_get_ver)
#define DRM_IOCTL_EXYNOS_G2D_SET_CMDLIST DRM_IOWR(DRM_COMMAND_BASE +   DRM_EXYNOS_G2D_SET_CMDLIST, struct drm_exynos_g2d_set_cmdlist)
#define DRM_IOCTL_EXYNOS_G2D_EXEC DRM_IOWR(DRM_COMMAND_BASE +   DRM_EXYNOS_G2D_EXEC, struct drm_exynos_g2d_exec)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DRM_IOCTL_EXYNOS_IPP_GET_PROPERTY DRM_IOWR(DRM_COMMAND_BASE +   DRM_EXYNOS_IPP_GET_PROPERTY, struct drm_exynos_ipp_prop_list)
#define DRM_IOCTL_EXYNOS_IPP_SET_PROPERTY DRM_IOWR(DRM_COMMAND_BASE +   DRM_EXYNOS_IPP_SET_PROPERTY, struct drm_exynos_ipp_property)
#define DRM_IOCTL_EXYNOS_IPP_QUEUE_BUF DRM_IOWR(DRM_COMMAND_BASE +   DRM_EXYNOS_IPP_QUEUE_BUF, struct drm_exynos_ipp_queue_buf)
#define DRM_IOCTL_EXYNOS_IPP_CMD_CTRL DRM_IOWR(DRM_COMMAND_BASE +   DRM_EXYNOS_IPP_CMD_CTRL, struct drm_exynos_ipp_cmd_ctrl)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DRM_EXYNOS_G2D_EVENT 0x80000000
#define DRM_EXYNOS_IPP_EVENT 0x80000001
struct drm_exynos_g2d_event {
 struct drm_event base;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u64 user_data;
 __u32 tv_sec;
 __u32 tv_usec;
 __u32 cmdlist_no;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 reserved;
};
struct drm_exynos_ipp_event {
 struct drm_event base;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u64 user_data;
 __u32 tv_sec;
 __u32 tv_usec;
 __u32 prop_id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 reserved;
 __u32 buf_id[EXYNOS_DRM_OPS_MAX];
};
#endif
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
