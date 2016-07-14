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
#ifndef _IPCONNTRACK_NETLINK_H
#define _IPCONNTRACK_NETLINK_H
#include <linux/netfilter/nfnetlink.h>
enum cntl_msg_types {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  IPCTNL_MSG_CT_NEW,
  IPCTNL_MSG_CT_GET,
  IPCTNL_MSG_CT_DELETE,
  IPCTNL_MSG_CT_GET_CTRZERO,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  IPCTNL_MSG_CT_GET_STATS_CPU,
  IPCTNL_MSG_CT_GET_STATS,
  IPCTNL_MSG_CT_GET_DYING,
  IPCTNL_MSG_CT_GET_UNCONFIRMED,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  IPCTNL_MSG_MAX
};
enum ctnl_exp_msg_types {
  IPCTNL_MSG_EXP_NEW,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  IPCTNL_MSG_EXP_GET,
  IPCTNL_MSG_EXP_DELETE,
  IPCTNL_MSG_EXP_GET_STATS_CPU,
  IPCTNL_MSG_EXP_MAX
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
enum ctattr_type {
  CTA_UNSPEC,
  CTA_TUPLE_ORIG,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_TUPLE_REPLY,
  CTA_STATUS,
  CTA_PROTOINFO,
  CTA_HELP,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_NAT_SRC,
#define CTA_NAT CTA_NAT_SRC
  CTA_TIMEOUT,
  CTA_MARK,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_COUNTERS_ORIG,
  CTA_COUNTERS_REPLY,
  CTA_USE,
  CTA_ID,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_NAT_DST,
  CTA_TUPLE_MASTER,
  CTA_SEQ_ADJ_ORIG,
  CTA_NAT_SEQ_ADJ_ORIG = CTA_SEQ_ADJ_ORIG,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_SEQ_ADJ_REPLY,
  CTA_NAT_SEQ_ADJ_REPLY = CTA_SEQ_ADJ_REPLY,
  CTA_SECMARK,
  CTA_ZONE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_SECCTX,
  CTA_TIMESTAMP,
  CTA_MARK_MASK,
  CTA_LABELS,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_LABELS_MASK,
  __CTA_MAX
};
#define CTA_MAX (__CTA_MAX - 1)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum ctattr_tuple {
  CTA_TUPLE_UNSPEC,
  CTA_TUPLE_IP,
  CTA_TUPLE_PROTO,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_TUPLE_ZONE,
  __CTA_TUPLE_MAX
};
#define CTA_TUPLE_MAX (__CTA_TUPLE_MAX - 1)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum ctattr_ip {
  CTA_IP_UNSPEC,
  CTA_IP_V4_SRC,
  CTA_IP_V4_DST,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_IP_V6_SRC,
  CTA_IP_V6_DST,
  __CTA_IP_MAX
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define CTA_IP_MAX (__CTA_IP_MAX - 1)
enum ctattr_l4proto {
  CTA_PROTO_UNSPEC,
  CTA_PROTO_NUM,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_PROTO_SRC_PORT,
  CTA_PROTO_DST_PORT,
  CTA_PROTO_ICMP_ID,
  CTA_PROTO_ICMP_TYPE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_PROTO_ICMP_CODE,
  CTA_PROTO_ICMPV6_ID,
  CTA_PROTO_ICMPV6_TYPE,
  CTA_PROTO_ICMPV6_CODE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __CTA_PROTO_MAX
};
#define CTA_PROTO_MAX (__CTA_PROTO_MAX - 1)
enum ctattr_protoinfo {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_PROTOINFO_UNSPEC,
  CTA_PROTOINFO_TCP,
  CTA_PROTOINFO_DCCP,
  CTA_PROTOINFO_SCTP,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __CTA_PROTOINFO_MAX
};
#define CTA_PROTOINFO_MAX (__CTA_PROTOINFO_MAX - 1)
enum ctattr_protoinfo_tcp {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_PROTOINFO_TCP_UNSPEC,
  CTA_PROTOINFO_TCP_STATE,
  CTA_PROTOINFO_TCP_WSCALE_ORIGINAL,
  CTA_PROTOINFO_TCP_WSCALE_REPLY,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_PROTOINFO_TCP_FLAGS_ORIGINAL,
  CTA_PROTOINFO_TCP_FLAGS_REPLY,
  __CTA_PROTOINFO_TCP_MAX
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define CTA_PROTOINFO_TCP_MAX (__CTA_PROTOINFO_TCP_MAX - 1)
enum ctattr_protoinfo_dccp {
  CTA_PROTOINFO_DCCP_UNSPEC,
  CTA_PROTOINFO_DCCP_STATE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_PROTOINFO_DCCP_ROLE,
  CTA_PROTOINFO_DCCP_HANDSHAKE_SEQ,
  __CTA_PROTOINFO_DCCP_MAX,
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define CTA_PROTOINFO_DCCP_MAX (__CTA_PROTOINFO_DCCP_MAX - 1)
enum ctattr_protoinfo_sctp {
  CTA_PROTOINFO_SCTP_UNSPEC,
  CTA_PROTOINFO_SCTP_STATE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_PROTOINFO_SCTP_VTAG_ORIGINAL,
  CTA_PROTOINFO_SCTP_VTAG_REPLY,
  __CTA_PROTOINFO_SCTP_MAX
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define CTA_PROTOINFO_SCTP_MAX (__CTA_PROTOINFO_SCTP_MAX - 1)
enum ctattr_counters {
  CTA_COUNTERS_UNSPEC,
  CTA_COUNTERS_PACKETS,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_COUNTERS_BYTES,
  CTA_COUNTERS32_PACKETS,
  CTA_COUNTERS32_BYTES,
  __CTA_COUNTERS_MAX
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define CTA_COUNTERS_MAX (__CTA_COUNTERS_MAX - 1)
enum ctattr_tstamp {
  CTA_TIMESTAMP_UNSPEC,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_TIMESTAMP_START,
  CTA_TIMESTAMP_STOP,
  __CTA_TIMESTAMP_MAX
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define CTA_TIMESTAMP_MAX (__CTA_TIMESTAMP_MAX - 1)
enum ctattr_nat {
  CTA_NAT_UNSPEC,
  CTA_NAT_V4_MINIP,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define CTA_NAT_MINIP CTA_NAT_V4_MINIP
  CTA_NAT_V4_MAXIP,
#define CTA_NAT_MAXIP CTA_NAT_V4_MAXIP
  CTA_NAT_PROTO,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_NAT_V6_MINIP,
  CTA_NAT_V6_MAXIP,
  __CTA_NAT_MAX
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define CTA_NAT_MAX (__CTA_NAT_MAX - 1)
enum ctattr_protonat {
  CTA_PROTONAT_UNSPEC,
  CTA_PROTONAT_PORT_MIN,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_PROTONAT_PORT_MAX,
  __CTA_PROTONAT_MAX
};
#define CTA_PROTONAT_MAX (__CTA_PROTONAT_MAX - 1)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum ctattr_seqadj {
  CTA_SEQADJ_UNSPEC,
  CTA_SEQADJ_CORRECTION_POS,
  CTA_SEQADJ_OFFSET_BEFORE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_SEQADJ_OFFSET_AFTER,
  __CTA_SEQADJ_MAX
};
#define CTA_SEQADJ_MAX (__CTA_SEQADJ_MAX - 1)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum ctattr_natseq {
  CTA_NAT_SEQ_UNSPEC,
  CTA_NAT_SEQ_CORRECTION_POS,
  CTA_NAT_SEQ_OFFSET_BEFORE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_NAT_SEQ_OFFSET_AFTER,
  __CTA_NAT_SEQ_MAX
};
#define CTA_NAT_SEQ_MAX (__CTA_NAT_SEQ_MAX - 1)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum ctattr_expect {
  CTA_EXPECT_UNSPEC,
  CTA_EXPECT_MASTER,
  CTA_EXPECT_TUPLE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_EXPECT_MASK,
  CTA_EXPECT_TIMEOUT,
  CTA_EXPECT_ID,
  CTA_EXPECT_HELP_NAME,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_EXPECT_ZONE,
  CTA_EXPECT_FLAGS,
  CTA_EXPECT_CLASS,
  CTA_EXPECT_NAT,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_EXPECT_FN,
  __CTA_EXPECT_MAX
};
#define CTA_EXPECT_MAX (__CTA_EXPECT_MAX - 1)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum ctattr_expect_nat {
  CTA_EXPECT_NAT_UNSPEC,
  CTA_EXPECT_NAT_DIR,
  CTA_EXPECT_NAT_TUPLE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __CTA_EXPECT_NAT_MAX
};
#define CTA_EXPECT_NAT_MAX (__CTA_EXPECT_NAT_MAX - 1)
enum ctattr_help {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_HELP_UNSPEC,
  CTA_HELP_NAME,
  CTA_HELP_INFO,
  __CTA_HELP_MAX
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define CTA_HELP_MAX (__CTA_HELP_MAX - 1)
enum ctattr_secctx {
  CTA_SECCTX_UNSPEC,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_SECCTX_NAME,
  __CTA_SECCTX_MAX
};
#define CTA_SECCTX_MAX (__CTA_SECCTX_MAX - 1)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum ctattr_stats_cpu {
  CTA_STATS_UNSPEC,
  CTA_STATS_SEARCHED,
  CTA_STATS_FOUND,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_STATS_NEW,
  CTA_STATS_INVALID,
  CTA_STATS_IGNORE,
  CTA_STATS_DELETE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_STATS_DELETE_LIST,
  CTA_STATS_INSERT,
  CTA_STATS_INSERT_FAILED,
  CTA_STATS_DROP,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_STATS_EARLY_DROP,
  CTA_STATS_ERROR,
  CTA_STATS_SEARCH_RESTART,
  __CTA_STATS_MAX,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define CTA_STATS_MAX (__CTA_STATS_MAX - 1)
enum ctattr_stats_global {
  CTA_STATS_GLOBAL_UNSPEC,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_STATS_GLOBAL_ENTRIES,
  __CTA_STATS_GLOBAL_MAX,
};
#define CTA_STATS_GLOBAL_MAX (__CTA_STATS_GLOBAL_MAX - 1)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum ctattr_expect_stats {
  CTA_STATS_EXP_UNSPEC,
  CTA_STATS_EXP_NEW,
  CTA_STATS_EXP_CREATE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CTA_STATS_EXP_DELETE,
  __CTA_STATS_EXP_MAX,
};
#define CTA_STATS_EXP_MAX (__CTA_STATS_EXP_MAX - 1)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#endif
