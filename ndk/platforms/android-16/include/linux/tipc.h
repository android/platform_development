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
#ifndef _LINUX_TIPC_H_
#define _LINUX_TIPC_H_
#include <linux/types.h>
#include <linux/sockios.h>
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct tipc_portid {
  __u32 ref;
  __u32 node;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct tipc_name {
  __u32 type;
  __u32 instance;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct tipc_name_seq {
  __u32 type;
  __u32 lower;
  __u32 upper;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define TIPC_CFG_SRV 0
#define TIPC_TOP_SRV 1
#define TIPC_LINK_STATE 2
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TIPC_RESERVED_TYPES 64
#define TIPC_ZONE_SCOPE 1
#define TIPC_CLUSTER_SCOPE 2
#define TIPC_NODE_SCOPE 3
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TIPC_MAX_USER_MSG_SIZE 66000U
#define TIPC_LOW_IMPORTANCE 0
#define TIPC_MEDIUM_IMPORTANCE 1
#define TIPC_HIGH_IMPORTANCE 2
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TIPC_CRITICAL_IMPORTANCE 3
#define TIPC_OK 0
#define TIPC_ERR_NO_NAME 1
#define TIPC_ERR_NO_PORT 2
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TIPC_ERR_NO_NODE 3
#define TIPC_ERR_OVERLOAD 4
#define TIPC_CONN_SHUTDOWN 5
#define TIPC_SUB_PORTS 0x01
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TIPC_SUB_SERVICE 0x02
#define TIPC_SUB_CANCEL 0x04
#define TIPC_WAIT_FOREVER (~0)
struct tipc_subscr {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct tipc_name_seq seq;
  __u32 timeout;
  __u32 filter;
  char usr_handle[8];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define TIPC_PUBLISHED 1
#define TIPC_WITHDRAWN 2
#define TIPC_SUBSCR_TIMEOUT 3
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct tipc_event {
  __u32 event;
  __u32 found_lower;
  __u32 found_upper;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct tipc_portid port;
  struct tipc_subscr s;
};
#ifndef AF_TIPC
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define AF_TIPC 30
#endif
#ifndef PF_TIPC
#define PF_TIPC AF_TIPC
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#endif
#ifndef SOL_TIPC
#define SOL_TIPC 271
#endif
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TIPC_ADDR_NAMESEQ 1
#define TIPC_ADDR_MCAST 1
#define TIPC_ADDR_NAME 2
#define TIPC_ADDR_ID 3
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct sockaddr_tipc {
  unsigned short family;
  unsigned char addrtype;
  signed char scope;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  union {
    struct tipc_portid id;
    struct tipc_name_seq nameseq;
    struct {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
      struct tipc_name name;
      __u32 domain;
    } name;
  } addr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define TIPC_ERRINFO 1
#define TIPC_RETDATA 2
#define TIPC_DESTNAME 3
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TIPC_IMPORTANCE 127
#define TIPC_SRC_DROPPABLE 128
#define TIPC_DEST_DROPPABLE 129
#define TIPC_CONN_TIMEOUT 130
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TIPC_NODE_RECVQ_DEPTH 131
#define TIPC_SOCK_RECVQ_DEPTH 132
#define TIPC_MAX_MEDIA_NAME 16
#define TIPC_MAX_IF_NAME 16
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TIPC_MAX_BEARER_NAME 32
#define TIPC_MAX_LINK_NAME 60
#define SIOCGETLINKNAME SIOCPROTOPRIVATE
struct tipc_sioc_ln_req {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 peer;
  __u32 bearer_id;
  char linkname[TIPC_MAX_LINK_NAME];
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#endif
