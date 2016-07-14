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
#ifndef _LINUX_RDS_H
#define _LINUX_RDS_H
#include <linux/types.h>
#define RDS_IB_ABI_VERSION 0x301
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define SOL_RDS 276
#define RDS_CANCEL_SENT_TO 1
#define RDS_GET_MR 2
#define RDS_FREE_MR 3
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define RDS_RECVERR 5
#define RDS_CONG_MONITOR 6
#define RDS_GET_MR_FOR_DEST 7
#define SO_RDS_TRANSPORT 8
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define RDS_TRANS_IB 0
#define RDS_TRANS_IWARP 1
#define RDS_TRANS_TCP 2
#define RDS_TRANS_COUNT 3
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define RDS_TRANS_NONE (~0)
#define RDS_CMSG_RDMA_ARGS 1
#define RDS_CMSG_RDMA_DEST 2
#define RDS_CMSG_RDMA_MAP 3
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define RDS_CMSG_RDMA_STATUS 4
#define RDS_CMSG_CONG_UPDATE 5
#define RDS_CMSG_ATOMIC_FADD 6
#define RDS_CMSG_ATOMIC_CSWP 7
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define RDS_CMSG_MASKED_ATOMIC_FADD 8
#define RDS_CMSG_MASKED_ATOMIC_CSWP 9
#define RDS_INFO_FIRST 10000
#define RDS_INFO_COUNTERS 10000
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define RDS_INFO_CONNECTIONS 10001
#define RDS_INFO_SEND_MESSAGES 10003
#define RDS_INFO_RETRANS_MESSAGES 10004
#define RDS_INFO_RECV_MESSAGES 10005
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define RDS_INFO_SOCKETS 10006
#define RDS_INFO_TCP_SOCKETS 10007
#define RDS_INFO_IB_CONNECTIONS 10008
#define RDS_INFO_CONNECTION_STATS 10009
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define RDS_INFO_IWARP_CONNECTIONS 10010
#define RDS_INFO_LAST 10010
struct rds_info_counter {
  uint8_t name[32];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  uint64_t value;
} __attribute__((packed));
#define RDS_INFO_CONNECTION_FLAG_SENDING 0x01
#define RDS_INFO_CONNECTION_FLAG_CONNECTING 0x02
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define RDS_INFO_CONNECTION_FLAG_CONNECTED 0x04
#define TRANSNAMSIZ 16
struct rds_info_connection {
  uint64_t next_tx_seq;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  uint64_t next_rx_seq;
  __be32 laddr;
  __be32 faddr;
  uint8_t transport[TRANSNAMSIZ];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  uint8_t flags;
} __attribute__((packed));
#define RDS_INFO_MESSAGE_FLAG_ACK 0x01
#define RDS_INFO_MESSAGE_FLAG_FAST_ACK 0x02
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct rds_info_message {
  uint64_t seq;
  uint32_t len;
  __be32 laddr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __be32 faddr;
  __be16 lport;
  __be16 fport;
  uint8_t flags;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
} __attribute__((packed));
struct rds_info_socket {
  uint32_t sndbuf;
  __be32 bound_addr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __be32 connected_addr;
  __be16 bound_port;
  __be16 connected_port;
  uint32_t rcvbuf;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  uint64_t inum;
} __attribute__((packed));
struct rds_info_tcp_socket {
  __be32 local_addr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __be16 local_port;
  __be32 peer_addr;
  __be16 peer_port;
  uint64_t hdr_rem;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  uint64_t data_rem;
  uint32_t last_sent_nxt;
  uint32_t last_expected_una;
  uint32_t last_seen_una;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
} __attribute__((packed));
#define RDS_IB_GID_LEN 16
struct rds_info_rdma_connection {
  __be32 src_addr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __be32 dst_addr;
  uint8_t src_gid[RDS_IB_GID_LEN];
  uint8_t dst_gid[RDS_IB_GID_LEN];
  uint32_t max_send_wr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  uint32_t max_recv_wr;
  uint32_t max_send_sge;
  uint32_t rdma_mr_max;
  uint32_t rdma_mr_size;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define RDS_CONG_MONITOR_SIZE 64
#define RDS_CONG_MONITOR_BIT(port) (((unsigned int) port) % RDS_CONG_MONITOR_SIZE)
#define RDS_CONG_MONITOR_MASK(port) (1ULL << RDS_CONG_MONITOR_BIT(port))
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
typedef uint64_t rds_rdma_cookie_t;
struct rds_iovec {
  uint64_t addr;
  uint64_t bytes;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct rds_get_mr_args {
  struct rds_iovec vec;
  uint64_t cookie_addr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  uint64_t flags;
};
struct rds_get_mr_for_dest_args {
  struct sockaddr_storage dest_addr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct rds_iovec vec;
  uint64_t cookie_addr;
  uint64_t flags;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct rds_free_mr_args {
  rds_rdma_cookie_t cookie;
  uint64_t flags;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct rds_rdma_args {
  rds_rdma_cookie_t cookie;
  struct rds_iovec remote_vec;
  uint64_t local_vec_addr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  uint64_t nr_local;
  uint64_t flags;
  uint64_t user_token;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct rds_atomic_args {
  rds_rdma_cookie_t cookie;
  uint64_t local_addr;
  uint64_t remote_addr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  union {
    struct {
      uint64_t compare;
      uint64_t swap;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    } cswp;
    struct {
      uint64_t add;
    } fadd;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    struct {
      uint64_t compare;
      uint64_t swap;
      uint64_t compare_mask;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
      uint64_t swap_mask;
    } m_cswp;
    struct {
      uint64_t add;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
      uint64_t nocarry_mask;
    } m_fadd;
  };
  uint64_t flags;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  uint64_t user_token;
};
struct rds_rdma_notify {
  uint64_t user_token;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  int32_t status;
};
#define RDS_RDMA_SUCCESS 0
#define RDS_RDMA_REMOTE_ERROR 1
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define RDS_RDMA_CANCELED 2
#define RDS_RDMA_DROPPED 3
#define RDS_RDMA_OTHER_ERROR 4
#define RDS_RDMA_READWRITE 0x0001
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define RDS_RDMA_FENCE 0x0002
#define RDS_RDMA_INVALIDATE 0x0004
#define RDS_RDMA_USE_ONCE 0x0008
#define RDS_RDMA_DONTWAIT 0x0010
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define RDS_RDMA_NOTIFY_ME 0x0020
#define RDS_RDMA_SILENT 0x0040
#endif
