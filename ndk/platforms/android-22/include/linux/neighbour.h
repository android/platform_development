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
#ifndef __LINUX_NEIGHBOUR_H
#define __LINUX_NEIGHBOUR_H
#include <linux/types.h>
#include <linux/netlink.h>
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct ndmsg {
  __u8 ndm_family;
  __u8 ndm_pad1;
  __u16 ndm_pad2;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __s32 ndm_ifindex;
  __u16 ndm_state;
  __u8 ndm_flags;
  __u8 ndm_type;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
enum {
  NDA_UNSPEC,
  NDA_DST,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  NDA_LLADDR,
  NDA_CACHEINFO,
  NDA_PROBES,
  NDA_VLAN,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  NDA_PORT,
  NDA_VNI,
  NDA_IFINDEX,
  NDA_MASTER,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  NDA_LINK_NETNSID,
  __NDA_MAX
};
#define NDA_MAX (__NDA_MAX - 1)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define NTF_USE 0x01
#define NTF_SELF 0x02
#define NTF_MASTER 0x04
#define NTF_PROXY 0x08
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define NTF_EXT_LEARNED 0x10
#define NTF_ROUTER 0x80
#define NUD_INCOMPLETE 0x01
#define NUD_REACHABLE 0x02
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define NUD_STALE 0x04
#define NUD_DELAY 0x08
#define NUD_PROBE 0x10
#define NUD_FAILED 0x20
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define NUD_NOARP 0x40
#define NUD_PERMANENT 0x80
#define NUD_NONE 0x00
struct nda_cacheinfo {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 ndm_confirmed;
  __u32 ndm_used;
  __u32 ndm_updated;
  __u32 ndm_refcnt;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct ndt_stats {
  __u64 ndts_allocs;
  __u64 ndts_destroys;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u64 ndts_hash_grows;
  __u64 ndts_res_failed;
  __u64 ndts_lookups;
  __u64 ndts_hits;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u64 ndts_rcv_probes_mcast;
  __u64 ndts_rcv_probes_ucast;
  __u64 ndts_periodic_gc_runs;
  __u64 ndts_forced_gc_runs;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u64 ndts_table_fulls;
};
enum {
  NDTPA_UNSPEC,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  NDTPA_IFINDEX,
  NDTPA_REFCNT,
  NDTPA_REACHABLE_TIME,
  NDTPA_BASE_REACHABLE_TIME,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  NDTPA_RETRANS_TIME,
  NDTPA_GC_STALETIME,
  NDTPA_DELAY_PROBE_TIME,
  NDTPA_QUEUE_LEN,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  NDTPA_APP_PROBES,
  NDTPA_UCAST_PROBES,
  NDTPA_MCAST_PROBES,
  NDTPA_ANYCAST_DELAY,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  NDTPA_PROXY_DELAY,
  NDTPA_PROXY_QLEN,
  NDTPA_LOCKTIME,
  NDTPA_QUEUE_LENBYTES,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  NDTPA_MCAST_REPROBES,
  __NDTPA_MAX
};
#define NDTPA_MAX (__NDTPA_MAX - 1)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct ndtmsg {
  __u8 ndtm_family;
  __u8 ndtm_pad1;
  __u16 ndtm_pad2;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct ndt_config {
  __u16 ndtc_key_len;
  __u16 ndtc_entry_size;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 ndtc_entries;
  __u32 ndtc_last_flush;
  __u32 ndtc_last_rand;
  __u32 ndtc_hash_rnd;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 ndtc_hash_mask;
  __u32 ndtc_hash_chain_gc;
  __u32 ndtc_proxy_qlen;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum {
  NDTA_UNSPEC,
  NDTA_NAME,
  NDTA_THRESH1,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  NDTA_THRESH2,
  NDTA_THRESH3,
  NDTA_CONFIG,
  NDTA_PARMS,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  NDTA_STATS,
  NDTA_GC_INTERVAL,
  __NDTA_MAX
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define NDTA_MAX (__NDTA_MAX - 1)
#endif
