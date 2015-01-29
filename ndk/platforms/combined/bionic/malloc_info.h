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

#ifndef LIBC_BIONIC_MALLOC_INFO_H_
#define LIBC_BIONIC_MALLOC_INFO_H_

#include <malloc.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

__LIBC_HIDDEN__ size_t __mallinfo_narenas();
__LIBC_HIDDEN__ size_t __mallinfo_nbins();
__LIBC_HIDDEN__ struct mallinfo __mallinfo_arena_info(size_t);
__LIBC_HIDDEN__ struct mallinfo __mallinfo_bin_info(size_t, size_t);

__END_DECLS

#endif // LIBC_BIONIC_MALLOC_INFO_H_
