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

#ifndef _BIONIC_NETBSD_PORT_BEFORE_H_included
#define _BIONIC_NETBSD_PORT_BEFORE_H_included

#include "namespace.h"
#include <sys/cdefs.h>
#include <time.h>
#include <arpa/nameser.h>

#define ISC_FORMAT_PRINTF(a,b) __printflike(a,b)
#define ISC_SOCKLEN_T socklen_t

#endif
