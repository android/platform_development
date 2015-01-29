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

#ifndef _BIONIC_CONFIG_H_
#define _BIONIC_CONFIG_H_

// valloc(3) and pvalloc(3) were removed from POSIX 2004. We do not include them
// for LP64, but the symbols remain in LP32 for binary compatibility.
#if !defined(__LP64__)
#define HAVE_DEPRECATED_MALLOC_FUNCS 1
#endif

#endif // _BIONIC_CONFIG_H_
