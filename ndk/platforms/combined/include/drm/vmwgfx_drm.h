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
#ifndef __VMWGFX_DRM_H__
#define __VMWGFX_DRM_H__
#include <drm/drm.h>
#define DRM_VMW_MAX_SURFACE_FACES 6
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DRM_VMW_MAX_MIP_LEVELS 24
#define DRM_VMW_GET_PARAM 0
#define DRM_VMW_ALLOC_DMABUF 1
#define DRM_VMW_UNREF_DMABUF 2
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DRM_VMW_CURSOR_BYPASS 3
#define DRM_VMW_CONTROL_STREAM 4
#define DRM_VMW_CLAIM_STREAM 5
#define DRM_VMW_UNREF_STREAM 6
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DRM_VMW_CREATE_CONTEXT 7
#define DRM_VMW_UNREF_CONTEXT 8
#define DRM_VMW_CREATE_SURFACE 9
#define DRM_VMW_UNREF_SURFACE 10
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DRM_VMW_REF_SURFACE 11
#define DRM_VMW_EXECBUF 12
#define DRM_VMW_GET_3D_CAP 13
#define DRM_VMW_FENCE_WAIT 14
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DRM_VMW_FENCE_SIGNALED 15
#define DRM_VMW_FENCE_UNREF 16
#define DRM_VMW_FENCE_EVENT 17
#define DRM_VMW_PRESENT 18
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DRM_VMW_PRESENT_READBACK 19
#define DRM_VMW_UPDATE_LAYOUT 20
#define DRM_VMW_CREATE_SHADER 21
#define DRM_VMW_UNREF_SHADER 22
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DRM_VMW_GB_SURFACE_CREATE 23
#define DRM_VMW_GB_SURFACE_REF 24
#define DRM_VMW_SYNCCPU 25
#define DRM_VMW_PARAM_NUM_STREAMS 0
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DRM_VMW_PARAM_NUM_FREE_STREAMS 1
#define DRM_VMW_PARAM_3D 2
#define DRM_VMW_PARAM_HW_CAPS 3
#define DRM_VMW_PARAM_FIFO_CAPS 4
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DRM_VMW_PARAM_MAX_FB_SIZE 5
#define DRM_VMW_PARAM_FIFO_HW_VERSION 6
#define DRM_VMW_PARAM_MAX_SURF_MEMORY 7
#define DRM_VMW_PARAM_3D_CAPS_SIZE 8
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DRM_VMW_PARAM_MAX_MOB_MEMORY 9
#define DRM_VMW_PARAM_MAX_MOB_SIZE 10
enum drm_vmw_handle_type {
 DRM_VMW_HANDLE_LEGACY = 0,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 DRM_VMW_HANDLE_PRIME = 1
};
struct drm_vmw_getparam_arg {
 uint64_t value;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t param;
 uint32_t pad64;
};
struct drm_vmw_context_arg {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 int32_t cid;
 uint32_t pad64;
};
struct drm_vmw_surface_create_req {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t flags;
 uint32_t format;
 uint32_t mip_levels[DRM_VMW_MAX_SURFACE_FACES];
 uint64_t size_addr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 int32_t shareable;
 int32_t scanout;
};
struct drm_vmw_surface_arg {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 int32_t sid;
 enum drm_vmw_handle_type handle_type;
};
struct drm_vmw_size {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t width;
 uint32_t height;
 uint32_t depth;
 uint32_t pad64;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
union drm_vmw_surface_create_arg {
 struct drm_vmw_surface_arg rep;
 struct drm_vmw_surface_create_req req;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
union drm_vmw_surface_reference_arg {
 struct drm_vmw_surface_create_req rep;
 struct drm_vmw_surface_arg req;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define DRM_VMW_EXECBUF_VERSION 1
struct drm_vmw_execbuf_arg {
 uint64_t commands;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t command_size;
 uint32_t throttle_us;
 uint64_t fence_rep;
 uint32_t version;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t flags;
};
struct drm_vmw_fence_rep {
 uint32_t handle;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t mask;
 uint32_t seqno;
 uint32_t passed_seqno;
 uint32_t pad64;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 int32_t error;
};
struct drm_vmw_alloc_dmabuf_req {
 uint32_t size;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t pad64;
};
struct drm_vmw_dmabuf_rep {
 uint64_t map_handle;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t handle;
 uint32_t cur_gmr_id;
 uint32_t cur_gmr_offset;
 uint32_t pad64;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
union drm_vmw_alloc_dmabuf_arg {
 struct drm_vmw_alloc_dmabuf_req req;
 struct drm_vmw_dmabuf_rep rep;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct drm_vmw_unref_dmabuf_arg {
 uint32_t handle;
 uint32_t pad64;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct drm_vmw_rect {
 int32_t x;
 int32_t y;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t w;
 uint32_t h;
};
struct drm_vmw_control_stream_arg {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t stream_id;
 uint32_t enabled;
 uint32_t flags;
 uint32_t color_key;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t handle;
 uint32_t offset;
 int32_t format;
 uint32_t size;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t width;
 uint32_t height;
 uint32_t pitch[3];
 uint32_t pad64;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct drm_vmw_rect src;
 struct drm_vmw_rect dst;
};
#define DRM_VMW_CURSOR_BYPASS_ALL (1 << 0)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DRM_VMW_CURSOR_BYPASS_FLAGS (1)
struct drm_vmw_cursor_bypass_arg {
 uint32_t flags;
 uint32_t crtc_id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 int32_t xpos;
 int32_t ypos;
 int32_t xhot;
 int32_t yhot;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct drm_vmw_stream_arg {
 uint32_t stream_id;
 uint32_t pad64;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct drm_vmw_get_3d_cap_arg {
 uint64_t buffer;
 uint32_t max_size;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t pad64;
};
#define DRM_VMW_FENCE_FLAG_EXEC (1 << 0)
#define DRM_VMW_FENCE_FLAG_QUERY (1 << 1)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DRM_VMW_WAIT_OPTION_UNREF (1 << 0)
struct drm_vmw_fence_wait_arg {
 uint32_t handle;
 int32_t cookie_valid;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint64_t kernel_cookie;
 uint64_t timeout_us;
 int32_t lazy;
 int32_t flags;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 int32_t wait_options;
 int32_t pad64;
};
struct drm_vmw_fence_signaled_arg {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t handle;
 uint32_t flags;
 int32_t signaled;
 uint32_t passed_seqno;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t signaled_flags;
 uint32_t pad64;
};
struct drm_vmw_fence_arg {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t handle;
 uint32_t pad64;
};
#define DRM_VMW_EVENT_FENCE_SIGNALED 0x80000000
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct drm_vmw_event_fence {
 struct drm_event base;
 uint64_t user_data;
 uint32_t tv_sec;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t tv_usec;
};
#define DRM_VMW_FE_FLAG_REQ_TIME (1 << 0)
struct drm_vmw_fence_event_arg {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint64_t fence_rep;
 uint64_t user_data;
 uint32_t handle;
 uint32_t flags;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct drm_vmw_present_arg {
 uint32_t fb_id;
 uint32_t sid;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 int32_t dest_x;
 int32_t dest_y;
 uint64_t clips_ptr;
 uint32_t num_clips;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t pad64;
};
struct drm_vmw_present_readback_arg {
 uint32_t fb_id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t num_clips;
 uint64_t clips_ptr;
 uint64_t fence_rep;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct drm_vmw_update_layout_arg {
 uint32_t num_outputs;
 uint32_t pad64;
 uint64_t rects;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
enum drm_vmw_shader_type {
 drm_vmw_shader_type_vs = 0,
 drm_vmw_shader_type_ps,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 drm_vmw_shader_type_gs
};
struct drm_vmw_shader_create_arg {
 enum drm_vmw_shader_type shader_type;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t size;
 uint32_t buffer_handle;
 uint32_t shader_handle;
 uint64_t offset;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct drm_vmw_shader_arg {
 uint32_t handle;
 uint32_t pad64;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
enum drm_vmw_surface_flags {
 drm_vmw_surface_flag_shareable = (1 << 0),
 drm_vmw_surface_flag_scanout = (1 << 1),
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 drm_vmw_surface_flag_create_buffer = (1 << 2)
};
struct drm_vmw_gb_surface_create_req {
 uint32_t svga3d_flags;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t format;
 uint32_t mip_levels;
 enum drm_vmw_surface_flags drm_surface_flags;
 uint32_t multisample_count;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t autogen_filter;
 uint32_t buffer_handle;
 uint32_t pad64;
 struct drm_vmw_size base_size;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct drm_vmw_gb_surface_create_rep {
 uint32_t handle;
 uint32_t backup_size;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t buffer_handle;
 uint32_t buffer_size;
 uint64_t buffer_map_handle;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
union drm_vmw_gb_surface_create_arg {
 struct drm_vmw_gb_surface_create_rep rep;
 struct drm_vmw_gb_surface_create_req req;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct drm_vmw_gb_surface_ref_rep {
 struct drm_vmw_gb_surface_create_req creq;
 struct drm_vmw_gb_surface_create_rep crep;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
union drm_vmw_gb_surface_reference_arg {
 struct drm_vmw_gb_surface_ref_rep rep;
 struct drm_vmw_surface_arg req;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum drm_vmw_synccpu_flags {
 drm_vmw_synccpu_read = (1 << 0),
 drm_vmw_synccpu_write = (1 << 1),
 drm_vmw_synccpu_dontblock = (1 << 2),
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 drm_vmw_synccpu_allow_cs = (1 << 3)
};
enum drm_vmw_synccpu_op {
 drm_vmw_synccpu_grab,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 drm_vmw_synccpu_release
};
struct drm_vmw_synccpu_arg {
 enum drm_vmw_synccpu_op op;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 enum drm_vmw_synccpu_flags flags;
 uint32_t handle;
 uint32_t pad64;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#endif
