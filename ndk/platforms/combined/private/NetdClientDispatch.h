/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PRIVATE_NETD_CLIENT_DISPATCH_H
#define PRIVATE_NETD_CLIENT_DISPATCH_H

#include <sys/cdefs.h>
#include <sys/socket.h>

__BEGIN_DECLS

struct NetdClientDispatch {
    int (*accept4)(int, struct sockaddr*, socklen_t*, int);
    int (*connect)(int, const struct sockaddr*, socklen_t);
    int (*socket)(int, int, int);
    unsigned (*netIdForResolv)(unsigned);
};

extern __LIBC_HIDDEN__ struct NetdClientDispatch __netdClientDispatch;

__END_DECLS

#endif  // PRIVATE_NETD_CLIENT_DISPATCH_H
