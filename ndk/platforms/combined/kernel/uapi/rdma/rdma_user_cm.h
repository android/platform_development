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
#ifndef RDMA_USER_CM_H
#define RDMA_USER_CM_H
#include <linux/types.h>
#include <linux/socket.h>
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#include <linux/in6.h>
#include <rdma/ib_user_verbs.h>
#include <rdma/ib_user_sa.h>
#define RDMA_USER_CM_ABI_VERSION 4
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define RDMA_MAX_PRIVATE_DATA 256
enum {
 RDMA_USER_CM_CMD_CREATE_ID,
 RDMA_USER_CM_CMD_DESTROY_ID,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 RDMA_USER_CM_CMD_BIND_IP,
 RDMA_USER_CM_CMD_RESOLVE_IP,
 RDMA_USER_CM_CMD_RESOLVE_ROUTE,
 RDMA_USER_CM_CMD_QUERY_ROUTE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 RDMA_USER_CM_CMD_CONNECT,
 RDMA_USER_CM_CMD_LISTEN,
 RDMA_USER_CM_CMD_ACCEPT,
 RDMA_USER_CM_CMD_REJECT,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 RDMA_USER_CM_CMD_DISCONNECT,
 RDMA_USER_CM_CMD_INIT_QP_ATTR,
 RDMA_USER_CM_CMD_GET_EVENT,
 RDMA_USER_CM_CMD_GET_OPTION,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 RDMA_USER_CM_CMD_SET_OPTION,
 RDMA_USER_CM_CMD_NOTIFY,
 RDMA_USER_CM_CMD_JOIN_IP_MCAST,
 RDMA_USER_CM_CMD_LEAVE_MCAST,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 RDMA_USER_CM_CMD_MIGRATE_ID,
 RDMA_USER_CM_CMD_QUERY,
 RDMA_USER_CM_CMD_BIND,
 RDMA_USER_CM_CMD_RESOLVE_ADDR,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 RDMA_USER_CM_CMD_JOIN_MCAST
};
struct rdma_ucm_cmd_hdr {
 __u32 cmd;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u16 in;
 __u16 out;
};
struct rdma_ucm_create_id {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u64 uid;
 __u64 response;
 __u16 ps;
 __u8 qp_type;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u8 reserved[5];
};
struct rdma_ucm_create_id_resp {
 __u32 id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct rdma_ucm_destroy_id {
 __u64 response;
 __u32 id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 reserved;
};
struct rdma_ucm_destroy_id_resp {
 __u32 events_reported;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct rdma_ucm_bind_ip {
 __u64 response;
 struct sockaddr_in6 addr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 id;
};
struct rdma_ucm_bind {
 __u32 id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u16 addr_size;
 __u16 reserved;
 struct sockaddr_storage addr;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct rdma_ucm_resolve_ip {
 struct sockaddr_in6 src_addr;
 struct sockaddr_in6 dst_addr;
 __u32 id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 timeout_ms;
};
struct rdma_ucm_resolve_addr {
 __u32 id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 timeout_ms;
 __u16 src_size;
 __u16 dst_size;
 __u32 reserved;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct sockaddr_storage src_addr;
 struct sockaddr_storage dst_addr;
};
struct rdma_ucm_resolve_route {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 id;
 __u32 timeout_ms;
};
enum {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 RDMA_USER_CM_QUERY_ADDR,
 RDMA_USER_CM_QUERY_PATH,
 RDMA_USER_CM_QUERY_GID
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct rdma_ucm_query {
 __u64 response;
 __u32 id;
 __u32 option;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct rdma_ucm_query_route_resp {
 __u64 node_guid;
 struct ib_user_path_rec ib_route[2];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct sockaddr_in6 src_addr;
 struct sockaddr_in6 dst_addr;
 __u32 num_paths;
 __u8 port_num;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u8 reserved[3];
};
struct rdma_ucm_query_addr_resp {
 __u64 node_guid;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u8 port_num;
 __u8 reserved;
 __u16 pkey;
 __u16 src_size;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u16 dst_size;
 struct sockaddr_storage src_addr;
 struct sockaddr_storage dst_addr;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct rdma_ucm_query_path_resp {
 __u32 num_paths;
 __u32 reserved;
 struct ib_path_rec_data path_data[0];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct rdma_ucm_conn_param {
 __u32 qp_num;
 __u32 qkey;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u8 private_data[RDMA_MAX_PRIVATE_DATA];
 __u8 private_data_len;
 __u8 srq;
 __u8 responder_resources;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u8 initiator_depth;
 __u8 flow_control;
 __u8 retry_count;
 __u8 rnr_retry_count;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u8 valid;
};
struct rdma_ucm_ud_param {
 __u32 qp_num;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 qkey;
 struct ib_uverbs_ah_attr ah_attr;
 __u8 private_data[RDMA_MAX_PRIVATE_DATA];
 __u8 private_data_len;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u8 reserved[7];
};
struct rdma_ucm_connect {
 struct rdma_ucm_conn_param conn_param;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 id;
 __u32 reserved;
};
struct rdma_ucm_listen {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 id;
 __u32 backlog;
};
struct rdma_ucm_accept {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u64 uid;
 struct rdma_ucm_conn_param conn_param;
 __u32 id;
 __u32 reserved;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct rdma_ucm_reject {
 __u32 id;
 __u8 private_data_len;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u8 reserved[3];
 __u8 private_data[RDMA_MAX_PRIVATE_DATA];
};
struct rdma_ucm_disconnect {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 id;
};
struct rdma_ucm_init_qp_attr {
 __u64 response;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 id;
 __u32 qp_state;
};
struct rdma_ucm_notify {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 id;
 __u32 event;
};
struct rdma_ucm_join_ip_mcast {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u64 response;
 __u64 uid;
 struct sockaddr_in6 addr;
 __u32 id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct rdma_ucm_join_mcast {
 __u64 response;
 __u64 uid;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 id;
 __u16 addr_size;
 __u16 reserved;
 struct sockaddr_storage addr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct rdma_ucm_get_event {
 __u64 response;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct rdma_ucm_event_resp {
 __u64 uid;
 __u32 id;
 __u32 event;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 status;
 union {
 struct rdma_ucm_conn_param conn;
 struct rdma_ucm_ud_param ud;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 } param;
};
enum {
 RDMA_OPTION_ID = 0,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 RDMA_OPTION_IB = 1
};
enum {
 RDMA_OPTION_ID_TOS = 0,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 RDMA_OPTION_ID_REUSEADDR = 1,
 RDMA_OPTION_ID_AFONLY = 2,
 RDMA_OPTION_IB_PATH = 1
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct rdma_ucm_set_option {
 __u64 optval;
 __u32 id;
 __u32 level;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 optname;
 __u32 optlen;
};
struct rdma_ucm_migrate_id {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u64 response;
 __u32 id;
 __u32 fd;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct rdma_ucm_migrate_resp {
 __u32 events_reported;
};
#endif
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
