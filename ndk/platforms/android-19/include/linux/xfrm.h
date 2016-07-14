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
#ifndef _LINUX_XFRM_H
#define _LINUX_XFRM_H
#include <linux/in6.h>
#include <linux/types.h>
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
typedef union {
  __be32 a4;
  __be32 a6[4];
  struct in6_addr in6;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
} xfrm_address_t;
struct xfrm_id {
  xfrm_address_t daddr;
  __be32 spi;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u8 proto;
};
struct xfrm_sec_ctx {
  __u8 ctx_doi;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u8 ctx_alg;
  __u16 ctx_len;
  __u32 ctx_sid;
  char ctx_str[0];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define XFRM_SC_DOI_RESERVED 0
#define XFRM_SC_DOI_LSM 1
#define XFRM_SC_ALG_RESERVED 0
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define XFRM_SC_ALG_SELINUX 1
struct xfrm_selector {
  xfrm_address_t daddr;
  xfrm_address_t saddr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __be16 dport;
  __be16 dport_mask;
  __be16 sport;
  __be16 sport_mask;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u16 family;
  __u8 prefixlen_d;
  __u8 prefixlen_s;
  __u8 proto;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  int ifindex;
  __kernel_uid32_t user;
};
#define XFRM_INF (~(__u64) 0)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct xfrm_lifetime_cfg {
  __u64 soft_byte_limit;
  __u64 hard_byte_limit;
  __u64 soft_packet_limit;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u64 hard_packet_limit;
  __u64 soft_add_expires_seconds;
  __u64 hard_add_expires_seconds;
  __u64 soft_use_expires_seconds;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u64 hard_use_expires_seconds;
};
struct xfrm_lifetime_cur {
  __u64 bytes;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u64 packets;
  __u64 add_time;
  __u64 use_time;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct xfrm_replay_state {
  __u32 oseq;
  __u32 seq;
  __u32 bitmap;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define XFRMA_REPLAY_ESN_MAX 4096
struct xfrm_replay_state_esn {
  unsigned int bmp_len;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 oseq;
  __u32 seq;
  __u32 oseq_hi;
  __u32 seq_hi;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 replay_window;
  __u32 bmp[0];
};
struct xfrm_algo {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  char alg_name[64];
  unsigned int alg_key_len;
  char alg_key[0];
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct xfrm_algo_auth {
  char alg_name[64];
  unsigned int alg_key_len;
  unsigned int alg_trunc_len;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  char alg_key[0];
};
struct xfrm_algo_aead {
  char alg_name[64];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  unsigned int alg_key_len;
  unsigned int alg_icv_len;
  char alg_key[0];
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct xfrm_stats {
  __u32 replay_window;
  __u32 replay;
  __u32 integrity_failed;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
enum {
  XFRM_POLICY_TYPE_MAIN = 0,
  XFRM_POLICY_TYPE_SUB = 1,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  XFRM_POLICY_TYPE_MAX = 2,
  XFRM_POLICY_TYPE_ANY = 255
};
enum {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  XFRM_POLICY_IN = 0,
  XFRM_POLICY_OUT = 1,
  XFRM_POLICY_FWD = 2,
  XFRM_POLICY_MASK = 3,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  XFRM_POLICY_MAX = 3
};
enum {
  XFRM_SHARE_ANY,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  XFRM_SHARE_SESSION,
  XFRM_SHARE_USER,
  XFRM_SHARE_UNIQUE
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define XFRM_MODE_TRANSPORT 0
#define XFRM_MODE_TUNNEL 1
#define XFRM_MODE_ROUTEOPTIMIZATION 2
#define XFRM_MODE_IN_TRIGGER 3
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define XFRM_MODE_BEET 4
#define XFRM_MODE_MAX 5
enum {
  XFRM_MSG_BASE = 0x10,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  XFRM_MSG_NEWSA = 0x10,
#define XFRM_MSG_NEWSA XFRM_MSG_NEWSA
  XFRM_MSG_DELSA,
#define XFRM_MSG_DELSA XFRM_MSG_DELSA
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  XFRM_MSG_GETSA,
#define XFRM_MSG_GETSA XFRM_MSG_GETSA
  XFRM_MSG_NEWPOLICY,
#define XFRM_MSG_NEWPOLICY XFRM_MSG_NEWPOLICY
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  XFRM_MSG_DELPOLICY,
#define XFRM_MSG_DELPOLICY XFRM_MSG_DELPOLICY
  XFRM_MSG_GETPOLICY,
#define XFRM_MSG_GETPOLICY XFRM_MSG_GETPOLICY
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  XFRM_MSG_ALLOCSPI,
#define XFRM_MSG_ALLOCSPI XFRM_MSG_ALLOCSPI
  XFRM_MSG_ACQUIRE,
#define XFRM_MSG_ACQUIRE XFRM_MSG_ACQUIRE
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  XFRM_MSG_EXPIRE,
#define XFRM_MSG_EXPIRE XFRM_MSG_EXPIRE
  XFRM_MSG_UPDPOLICY,
#define XFRM_MSG_UPDPOLICY XFRM_MSG_UPDPOLICY
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  XFRM_MSG_UPDSA,
#define XFRM_MSG_UPDSA XFRM_MSG_UPDSA
  XFRM_MSG_POLEXPIRE,
#define XFRM_MSG_POLEXPIRE XFRM_MSG_POLEXPIRE
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  XFRM_MSG_FLUSHSA,
#define XFRM_MSG_FLUSHSA XFRM_MSG_FLUSHSA
  XFRM_MSG_FLUSHPOLICY,
#define XFRM_MSG_FLUSHPOLICY XFRM_MSG_FLUSHPOLICY
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  XFRM_MSG_NEWAE,
#define XFRM_MSG_NEWAE XFRM_MSG_NEWAE
  XFRM_MSG_GETAE,
#define XFRM_MSG_GETAE XFRM_MSG_GETAE
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  XFRM_MSG_REPORT,
#define XFRM_MSG_REPORT XFRM_MSG_REPORT
  XFRM_MSG_MIGRATE,
#define XFRM_MSG_MIGRATE XFRM_MSG_MIGRATE
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  XFRM_MSG_NEWSADINFO,
#define XFRM_MSG_NEWSADINFO XFRM_MSG_NEWSADINFO
  XFRM_MSG_GETSADINFO,
#define XFRM_MSG_GETSADINFO XFRM_MSG_GETSADINFO
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  XFRM_MSG_NEWSPDINFO,
#define XFRM_MSG_NEWSPDINFO XFRM_MSG_NEWSPDINFO
  XFRM_MSG_GETSPDINFO,
#define XFRM_MSG_GETSPDINFO XFRM_MSG_GETSPDINFO
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  XFRM_MSG_MAPPING,
#define XFRM_MSG_MAPPING XFRM_MSG_MAPPING
  __XFRM_MSG_MAX
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define XFRM_MSG_MAX (__XFRM_MSG_MAX - 1)
#define XFRM_NR_MSGTYPES (XFRM_MSG_MAX + 1 - XFRM_MSG_BASE)
struct xfrm_user_sec_ctx {
  __u16 len;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u16 exttype;
  __u8 ctx_alg;
  __u8 ctx_doi;
  __u16 ctx_len;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct xfrm_user_tmpl {
  struct xfrm_id id;
  __u16 family;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  xfrm_address_t saddr;
  __u32 reqid;
  __u8 mode;
  __u8 share;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u8 optional;
  __u32 aalgos;
  __u32 ealgos;
  __u32 calgos;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct xfrm_encap_tmpl {
  __u16 encap_type;
  __be16 encap_sport;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __be16 encap_dport;
  xfrm_address_t encap_oa;
};
enum xfrm_ae_ftype_t {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  XFRM_AE_UNSPEC,
  XFRM_AE_RTHR = 1,
  XFRM_AE_RVAL = 2,
  XFRM_AE_LVAL = 4,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  XFRM_AE_ETHR = 8,
  XFRM_AE_CR = 16,
  XFRM_AE_CE = 32,
  XFRM_AE_CU = 64,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __XFRM_AE_MAX
#define XFRM_AE_MAX (__XFRM_AE_MAX - 1)
};
struct xfrm_userpolicy_type {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u8 type;
  __u16 reserved1;
  __u8 reserved2;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum xfrm_attr_type_t {
  XFRMA_UNSPEC,
  XFRMA_ALG_AUTH,
  XFRMA_ALG_CRYPT,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  XFRMA_ALG_COMP,
  XFRMA_ENCAP,
  XFRMA_TMPL,
  XFRMA_SA,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  XFRMA_POLICY,
  XFRMA_SEC_CTX,
  XFRMA_LTIME_VAL,
  XFRMA_REPLAY_VAL,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  XFRMA_REPLAY_THRESH,
  XFRMA_ETIMER_THRESH,
  XFRMA_SRCADDR,
  XFRMA_COADDR,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  XFRMA_LASTUSED,
  XFRMA_POLICY_TYPE,
  XFRMA_MIGRATE,
  XFRMA_ALG_AEAD,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  XFRMA_KMADDRESS,
  XFRMA_ALG_AUTH_TRUNC,
  XFRMA_MARK,
  XFRMA_TFCPAD,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  XFRMA_REPLAY_ESN_VAL,
  XFRMA_SA_EXTRA_FLAGS,
  XFRMA_PROTO,
  XFRMA_ADDRESS_FILTER,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __XFRMA_MAX
#define XFRMA_MAX (__XFRMA_MAX - 1)
};
struct xfrm_mark {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 v;
  __u32 m;
};
enum xfrm_sadattr_type_t {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  XFRMA_SAD_UNSPEC,
  XFRMA_SAD_CNT,
  XFRMA_SAD_HINFO,
  __XFRMA_SAD_MAX
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define XFRMA_SAD_MAX (__XFRMA_SAD_MAX - 1)
};
struct xfrmu_sadhinfo {
  __u32 sadhcnt;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 sadhmcnt;
};
enum xfrm_spdattr_type_t {
  XFRMA_SPD_UNSPEC,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  XFRMA_SPD_INFO,
  XFRMA_SPD_HINFO,
  XFRMA_SPD_IPV4_HTHRESH,
  XFRMA_SPD_IPV6_HTHRESH,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __XFRMA_SPD_MAX
#define XFRMA_SPD_MAX (__XFRMA_SPD_MAX - 1)
};
struct xfrmu_spdinfo {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 incnt;
  __u32 outcnt;
  __u32 fwdcnt;
  __u32 inscnt;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 outscnt;
  __u32 fwdscnt;
};
struct xfrmu_spdhinfo {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 spdhcnt;
  __u32 spdhmcnt;
};
struct xfrmu_spdhthresh {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u8 lbits;
  __u8 rbits;
};
struct xfrm_usersa_info {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct xfrm_selector sel;
  struct xfrm_id id;
  xfrm_address_t saddr;
  struct xfrm_lifetime_cfg lft;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct xfrm_lifetime_cur curlft;
  struct xfrm_stats stats;
  __u32 seq;
  __u32 reqid;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u16 family;
  __u8 mode;
  __u8 replay_window;
  __u8 flags;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define XFRM_STATE_NOECN 1
#define XFRM_STATE_DECAP_DSCP 2
#define XFRM_STATE_NOPMTUDISC 4
#define XFRM_STATE_WILDRECV 8
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define XFRM_STATE_ICMP 16
#define XFRM_STATE_AF_UNSPEC 32
#define XFRM_STATE_ALIGN4 64
#define XFRM_STATE_ESN 128
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define XFRM_SA_XFLAG_DONT_ENCAP_DSCP 1
struct xfrm_usersa_id {
  xfrm_address_t daddr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __be32 spi;
  __u16 family;
  __u8 proto;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct xfrm_aevent_id {
  struct xfrm_usersa_id sa_id;
  xfrm_address_t saddr;
  __u32 flags;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 reqid;
};
struct xfrm_userspi_info {
  struct xfrm_usersa_info info;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 min;
  __u32 max;
};
struct xfrm_userpolicy_info {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct xfrm_selector sel;
  struct xfrm_lifetime_cfg lft;
  struct xfrm_lifetime_cur curlft;
  __u32 priority;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 index;
  __u8 dir;
  __u8 action;
#define XFRM_POLICY_ALLOW 0
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define XFRM_POLICY_BLOCK 1
  __u8 flags;
#define XFRM_POLICY_LOCALOK 1
#define XFRM_POLICY_ICMP 2
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u8 share;
};
struct xfrm_userpolicy_id {
  struct xfrm_selector sel;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 index;
  __u8 dir;
};
struct xfrm_user_acquire {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct xfrm_id id;
  xfrm_address_t saddr;
  struct xfrm_selector sel;
  struct xfrm_userpolicy_info policy;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 aalgos;
  __u32 ealgos;
  __u32 calgos;
  __u32 seq;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct xfrm_user_expire {
  struct xfrm_usersa_info state;
  __u8 hard;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct xfrm_user_polexpire {
  struct xfrm_userpolicy_info pol;
  __u8 hard;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct xfrm_usersa_flush {
  __u8 proto;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct xfrm_user_report {
  __u8 proto;
  struct xfrm_selector sel;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct xfrm_user_kmaddress {
  xfrm_address_t local;
  xfrm_address_t remote;
  __u32 reserved;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u16 family;
};
struct xfrm_user_migrate {
  xfrm_address_t old_daddr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  xfrm_address_t old_saddr;
  xfrm_address_t new_daddr;
  xfrm_address_t new_saddr;
  __u8 proto;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u8 mode;
  __u16 reserved;
  __u32 reqid;
  __u16 old_family;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u16 new_family;
};
struct xfrm_user_mapping {
  struct xfrm_usersa_id id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 reqid;
  xfrm_address_t old_saddr;
  xfrm_address_t new_saddr;
  __be16 old_sport;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __be16 new_sport;
};
struct xfrm_address_filter {
  xfrm_address_t saddr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  xfrm_address_t daddr;
  __u16 family;
  __u8 splen;
  __u8 dplen;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define XFRMGRP_ACQUIRE 1
#define XFRMGRP_EXPIRE 2
#define XFRMGRP_SA 4
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define XFRMGRP_POLICY 8
#define XFRMGRP_REPORT 0x20
enum xfrm_nlgroups {
  XFRMNLGRP_NONE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define XFRMNLGRP_NONE XFRMNLGRP_NONE
  XFRMNLGRP_ACQUIRE,
#define XFRMNLGRP_ACQUIRE XFRMNLGRP_ACQUIRE
  XFRMNLGRP_EXPIRE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define XFRMNLGRP_EXPIRE XFRMNLGRP_EXPIRE
  XFRMNLGRP_SA,
#define XFRMNLGRP_SA XFRMNLGRP_SA
  XFRMNLGRP_POLICY,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define XFRMNLGRP_POLICY XFRMNLGRP_POLICY
  XFRMNLGRP_AEVENTS,
#define XFRMNLGRP_AEVENTS XFRMNLGRP_AEVENTS
  XFRMNLGRP_REPORT,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define XFRMNLGRP_REPORT XFRMNLGRP_REPORT
  XFRMNLGRP_MIGRATE,
#define XFRMNLGRP_MIGRATE XFRMNLGRP_MIGRATE
  XFRMNLGRP_MAPPING,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define XFRMNLGRP_MAPPING XFRMNLGRP_MAPPING
  __XFRMNLGRP_MAX
};
#define XFRMNLGRP_MAX (__XFRMNLGRP_MAX - 1)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#endif
