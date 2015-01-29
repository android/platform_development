/*
 * Copyright (C) 2014 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef _RESOLV_NETID_H
#define _RESOLV_NETID_H

/* This header contains declarations related to per-network DNS
 * server selection. They are used by system/netd/ and should not be
 * exposed by the C library's public NDK headers.
 */
#include <sys/cdefs.h>
#include <netinet/in.h>
#include <stdio.h>

/*
 * Passing NETID_UNSET as the netId causes system/netd/server/DnsProxyListener.cpp to
 * fill in the appropriate default netId for the query.
 */
#define NETID_UNSET 0u

/*
 * MARK_UNSET represents the default (i.e. unset) value for a socket mark.
 */
#define MARK_UNSET 0u

__BEGIN_DECLS

struct addrinfo;

#define __used_in_netd __attribute__((visibility ("default")))

struct hostent *android_gethostbyaddrfornet(const void *, socklen_t, int, unsigned, unsigned) __used_in_netd;
struct hostent *android_gethostbynamefornet(const char *, int, unsigned, unsigned) __used_in_netd;
int android_getaddrinfofornet(const char *, const char *, const struct addrinfo *, unsigned,
		unsigned, struct addrinfo **) __used_in_netd;

/* set name servers for a network */
extern void _resolv_set_nameservers_for_net(unsigned netid,
    const char** servers, int numservers, const char *domains) __used_in_netd;

/* flush the cache associated with a certain network */
extern void _resolv_flush_cache_for_net(unsigned netid) __used_in_netd;

/* delete the cache associated with a certain network */
extern void _resolv_delete_cache_for_net(unsigned netid) __used_in_netd;

/* Internal use only. */
struct hostent *android_gethostbyaddrfornet_proxy(const void *, socklen_t, int , unsigned, unsigned) __LIBC_HIDDEN__;
int android_getnameinfofornet(const struct sockaddr *, socklen_t, char *, size_t, char *, size_t, int, unsigned, unsigned) __LIBC_HIDDEN__;
FILE* android_open_proxy(void) __LIBC_HIDDEN__;

/* delete the cache associated with a certain network */
extern void _resolv_delete_cache_for_net(unsigned netid);

__END_DECLS

#endif /* _RESOLV_NETID_H */
