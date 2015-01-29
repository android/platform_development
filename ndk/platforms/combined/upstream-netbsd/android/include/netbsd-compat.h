/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef _BIONIC_NETBSD_COMPAT_H_included
#define _BIONIC_NETBSD_COMPAT_H_included

#define _BSD_SOURCE
#define _GNU_SOURCE

// NetBSD uses _DIAGASSERT to null-check arguments and the like.
#include <assert.h>
#define _DIAGASSERT(e) ((e) ? (void) 0 : __assert2(__FILE__, __LINE__, __func__, #e))

// TODO: update our <sys/cdefs.h> to support this properly.
#define __type_fit(t, a) (0 == 0)

// TODO: we don't yet have thread-safe environment variables.
#define __readlockenv() 0
#define __unlockenv() 0

#endif
