/*
 * Copyright (C) 2008 The Android Open Source Project
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
#ifndef _RESOLV_CACHE_H_
#define _RESOLV_CACHE_H_

#include <stddef.h>
#include <sys/cdefs.h>

struct __res_state;

/* sets the name server addresses to the provided res_state structure. The
 * name servers are retrieved from the cache which is associated
 * with the network to which the res_state structure is associated */
__LIBC_HIDDEN__
extern void _resolv_populate_res_for_net(struct __res_state* statp);

typedef enum {
    RESOLV_CACHE_UNSUPPORTED,  /* the cache can't handle that kind of queries */
                               /* or the answer buffer is too small */
    RESOLV_CACHE_NOTFOUND,     /* the cache doesn't know about this query */
    RESOLV_CACHE_FOUND         /* the cache found the answer */
} ResolvCacheStatus;

__LIBC_HIDDEN__
extern ResolvCacheStatus
_resolv_cache_lookup( unsigned              netid,
                      const void*           query,
                      int                   querylen,
                      void*                 answer,
                      int                   answersize,
                      int                  *answerlen );

/* add a (query,answer) to the cache, only call if _resolv_cache_lookup
 * did return RESOLV_CACHE_NOTFOUND
 */
__LIBC_HIDDEN__
extern void
_resolv_cache_add( unsigned              netid,
                   const void*           query,
                   int                   querylen,
                   const void*           answer,
                   int                   answerlen );

/* Notify the cache a request failed */
__LIBC_HIDDEN__
extern void
_resolv_cache_query_failed( unsigned     netid,
                   const void* query,
                   int         querylen);

#endif /* _RESOLV_CACHE_H_ */
