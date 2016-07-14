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
#ifndef _LINUX_ATMSVC_H
#define _LINUX_ATMSVC_H
#include <linux/atmapi.h>
#include <linux/atm.h>
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#include <linux/atmioc.h>
#define ATMSIGD_CTRL _IO('a', ATMIOC_SPECIAL)
enum atmsvc_msg_type {
  as_catch_null,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  as_bind,
  as_connect,
  as_accept,
  as_reject,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  as_listen,
  as_okay,
  as_error,
  as_indicate,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  as_close,
  as_itf_notify,
  as_modify,
  as_identify,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  as_terminate,
  as_addparty,
  as_dropparty
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct atmsvc_msg {
  enum atmsvc_msg_type type;
  atm_kptr_t vcc;
  atm_kptr_t listen_vcc;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  int reply;
  struct sockaddr_atmpvc pvc;
  struct sockaddr_atmsvc local;
  struct atm_qos qos;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct atm_sap sap;
  unsigned int session;
  struct sockaddr_atmsvc svc;
} __ATM_API_ALIGN;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define SELECT_TOP_PCR(tp) ((tp).pcr ? (tp).pcr : (tp).max_pcr && (tp).max_pcr != ATM_MAX_PCR ? (tp).max_pcr : (tp).min_pcr ? (tp).min_pcr : ATM_MAX_PCR)
#endif
