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
#ifndef _UAPI_VIDEO_ADF_H_
#define _UAPI_VIDEO_ADF_H_
#include <linux/ioctl.h>
#include <linux/types.h>
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#include <drm/drm_fourcc.h>
#include <drm/drm_mode.h>
#define ADF_NAME_LEN 32
#define ADF_MAX_CUSTOM_DATA_SIZE 4096
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum adf_interface_type {
 ADF_INTF_DSI = 0,
 ADF_INTF_eDP = 1,
 ADF_INTF_DPI = 2,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 ADF_INTF_VGA = 3,
 ADF_INTF_DVI = 4,
 ADF_INTF_HDMI = 5,
 ADF_INTF_MEMORY = 6,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 ADF_INTF_TYPE_DEVICE_CUSTOM = 128,
 ADF_INTF_TYPE_MAX = (~(__u32)0),
};
#define ADF_INTF_FLAG_PRIMARY (1 << 0)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define ADF_INTF_FLAG_EXTERNAL (1 << 1)
enum adf_event_type {
 ADF_EVENT_VSYNC = 0,
 ADF_EVENT_HOTPLUG = 1,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 ADF_EVENT_DEVICE_CUSTOM = 128,
 ADF_EVENT_TYPE_MAX = 255,
};
struct adf_set_event {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u8 type;
 __u8 enabled;
};
struct adf_event {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u8 type;
 __u32 length;
};
struct adf_vsync_event {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct adf_event base;
 __aligned_u64 timestamp;
};
struct adf_hotplug_event {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct adf_event base;
 __u8 connected;
};
#define ADF_MAX_PLANES 4
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct adf_buffer_config {
 __u32 overlay_engine;
 __u32 w;
 __u32 h;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 format;
 __s32 fd[ADF_MAX_PLANES];
 __u32 offset[ADF_MAX_PLANES];
 __u32 pitch[ADF_MAX_PLANES];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u8 n_planes;
 __s32 acquire_fence;
};
#define ADF_MAX_BUFFERS (4096 / sizeof(struct adf_buffer_config))
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct adf_post_config {
 size_t n_interfaces;
 __u32 __user *interfaces;
 size_t n_bufs;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct adf_buffer_config __user *bufs;
 size_t custom_data_size;
 void __user *custom_data;
 __s32 complete_fence;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define ADF_MAX_INTERFACES (4096 / sizeof(__u32))
struct adf_simple_buffer_alloc {
 __u16 w;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u16 h;
 __u32 format;
 __s32 fd;
 __u32 offset;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 pitch;
};
struct adf_simple_post_config {
 struct adf_buffer_config buf;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __s32 complete_fence;
};
struct adf_attachment_config {
 __u32 overlay_engine;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 interface;
};
struct adf_device_data {
 char name[ADF_NAME_LEN];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 size_t n_attachments;
 struct adf_attachment_config __user *attachments;
 size_t n_allowed_attachments;
 struct adf_attachment_config __user *allowed_attachments;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 size_t custom_data_size;
 void __user *custom_data;
};
#define ADF_MAX_ATTACHMENTS (4096 / sizeof(struct adf_attachment_config))
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct adf_interface_data {
 char name[ADF_NAME_LEN];
 __u32 type;
 __u32 id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 flags;
 __u8 dpms_state;
 __u8 hotplug_detect;
 __u16 width_mm;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u16 height_mm;
 struct drm_mode_modeinfo current_mode;
 size_t n_available_modes;
 struct drm_mode_modeinfo __user *available_modes;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 size_t custom_data_size;
 void __user *custom_data;
};
#define ADF_MAX_MODES (4096 / sizeof(struct drm_mode_modeinfo))
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct adf_overlay_engine_data {
 char name[ADF_NAME_LEN];
 size_t n_supported_formats;
 __u32 __user *supported_formats;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 size_t custom_data_size;
 void __user *custom_data;
};
#define ADF_MAX_SUPPORTED_FORMATS (4096 / sizeof(__u32))
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define ADF_IOCTL_TYPE 'D'
#define ADF_IOCTL_NR_CUSTOM 128
#define ADF_SET_EVENT _IOW(ADF_IOCTL_TYPE, 0, struct adf_set_event)
#define ADF_BLANK _IOW(ADF_IOCTL_TYPE, 1, __u8)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define ADF_POST_CONFIG _IOW(ADF_IOCTL_TYPE, 2, struct adf_post_config)
#define ADF_SET_MODE _IOW(ADF_IOCTL_TYPE, 3,   struct drm_mode_modeinfo)
#define ADF_GET_DEVICE_DATA _IOR(ADF_IOCTL_TYPE, 4, struct adf_device_data)
#define ADF_GET_INTERFACE_DATA _IOR(ADF_IOCTL_TYPE, 5,   struct adf_interface_data)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define ADF_GET_OVERLAY_ENGINE_DATA   _IOR(ADF_IOCTL_TYPE, 6,   struct adf_overlay_engine_data)
#define ADF_SIMPLE_POST_CONFIG _IOW(ADF_IOCTL_TYPE, 7,   struct adf_simple_post_config)
#define ADF_SIMPLE_BUFFER_ALLOC _IOW(ADF_IOCTL_TYPE, 8,   struct adf_simple_buffer_alloc)
#define ADF_ATTACH _IOW(ADF_IOCTL_TYPE, 9,   struct adf_attachment_config)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define ADF_DETACH _IOW(ADF_IOCTL_TYPE, 10,   struct adf_attachment_config)
#endif
