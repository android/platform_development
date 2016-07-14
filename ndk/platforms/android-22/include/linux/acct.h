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
#ifndef _UAPI_LINUX_ACCT_H
#define _UAPI_LINUX_ACCT_H
#include <linux/types.h>
#include <asm/param.h>
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#include <asm/byteorder.h>
typedef __u16 comp_t;
typedef __u32 comp2_t;
#define ACCT_COMM 16
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct acct {
  char ac_flag;
  char ac_version;
  __u16 ac_uid16;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u16 ac_gid16;
  __u16 ac_tty;
  __u32 ac_btime;
  comp_t ac_utime;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  comp_t ac_stime;
  comp_t ac_etime;
  comp_t ac_mem;
  comp_t ac_io;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  comp_t ac_rw;
  comp_t ac_minflt;
  comp_t ac_majflt;
  comp_t ac_swaps;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u16 ac_ahz;
  __u32 ac_exitcode;
  char ac_comm[ACCT_COMM + 1];
  __u8 ac_etime_hi;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u16 ac_etime_lo;
  __u32 ac_uid;
  __u32 ac_gid;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct acct_v3 {
  char ac_flag;
  char ac_version;
  __u16 ac_tty;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 ac_exitcode;
  __u32 ac_uid;
  __u32 ac_gid;
  __u32 ac_pid;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 ac_ppid;
  __u32 ac_btime;
  float ac_etime;
  comp_t ac_utime;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  comp_t ac_stime;
  comp_t ac_mem;
  comp_t ac_io;
  comp_t ac_rw;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  comp_t ac_minflt;
  comp_t ac_majflt;
  comp_t ac_swaps;
  char ac_comm[ACCT_COMM];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define AFORK 0x01
#define ASU 0x02
#define ACOMPAT 0x04
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define ACORE 0x08
#define AXSIG 0x10
#if defined(__BYTE_ORDER) ? __BYTE_ORDER == __BIG_ENDIAN : defined(__BIG_ENDIAN)
#define ACCT_BYTEORDER 0x80
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#elif defined(__BYTE_ORDER)?__BYTE_ORDER==__LITTLE_ENDIAN:defined(__LITTLE_ENDIAN)
#define ACCT_BYTEORDER 0x00
#else
#error unspecified endianness
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#endif
#define ACCT_VERSION 2
#define AHZ (HZ)
#endif
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
