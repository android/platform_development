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
#ifndef _UAPI__LINUX_BPF_H__
#define _UAPI__LINUX_BPF_H__
#include <linux/types.h>
#include <linux/bpf_common.h>
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define BPF_ALU64 0x07
#define BPF_DW 0x18
#define BPF_XADD 0xc0
#define BPF_MOV 0xb0
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define BPF_ARSH 0xc0
#define BPF_END 0xd0
#define BPF_TO_LE 0x00
#define BPF_TO_BE 0x08
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define BPF_FROM_LE BPF_TO_LE
#define BPF_FROM_BE BPF_TO_BE
#define BPF_JNE 0x50
#define BPF_JSGT 0x60
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define BPF_JSGE 0x70
#define BPF_CALL 0x80
#define BPF_EXIT 0x90
enum {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 BPF_REG_0 = 0,
 BPF_REG_1,
 BPF_REG_2,
 BPF_REG_3,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 BPF_REG_4,
 BPF_REG_5,
 BPF_REG_6,
 BPF_REG_7,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 BPF_REG_8,
 BPF_REG_9,
 BPF_REG_10,
 __MAX_BPF_REG,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define MAX_BPF_REG __MAX_BPF_REG
struct bpf_insn {
 __u8 code;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u8 dst_reg:4;
 __u8 src_reg:4;
 __s16 off;
 __s32 imm;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
enum bpf_cmd {
 BPF_MAP_CREATE,
 BPF_MAP_LOOKUP_ELEM,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 BPF_MAP_UPDATE_ELEM,
 BPF_MAP_DELETE_ELEM,
 BPF_MAP_GET_NEXT_KEY,
 BPF_PROG_LOAD,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
enum bpf_map_type {
 BPF_MAP_TYPE_UNSPEC,
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum bpf_prog_type {
 BPF_PROG_TYPE_UNSPEC,
};
union bpf_attr {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct {
 __u32 map_type;
 __u32 key_size;
 __u32 value_size;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 max_entries;
 };
 struct {
 __u32 map_fd;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __aligned_u64 key;
 union {
 __aligned_u64 value;
 __aligned_u64 next_key;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 };
 };
 struct {
 __u32 prog_type;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 insn_cnt;
 __aligned_u64 insns;
 __aligned_u64 license;
 __u32 log_level;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 log_size;
 __aligned_u64 log_buf;
 };
} __attribute__((aligned(8)));
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum bpf_func_id {
 BPF_FUNC_unspec,
 __BPF_FUNC_MAX_ID,
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#endif
