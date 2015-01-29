/*
 * Copyright (C) 2010 The Android Open Source Project
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

#ifndef BIONIC_ATOMIC_INLINE_H
#define BIONIC_ATOMIC_INLINE_H

/*
 * Inline declarations and macros for some special-purpose atomic
 * operations.  These are intended for rare circumstances where a
 * memory barrier needs to be issued inline rather than as a function
 * call.
 *
 * Macros defined in this header:
 *
 * void ANDROID_MEMBAR_FULL()
 *   Full memory barrier.  Provides a compiler reordering barrier, and
 *   on SMP systems emits an appropriate instruction.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Define __ATOMIC_INLINE__ to control the inlining of all atomics
 * functions declared here. For a slight performance boost, we want
 * all of them to be always_inline
 */
#define  __ATOMIC_INLINE__  static __inline__ __attribute__((always_inline))

#if defined(__arm__)
#  include "bionic_atomic_arm.h"
#elif defined(__aarch64__)
#  include "bionic_atomic_arm64.h"
#elif defined(__i386__)
#  include "bionic_atomic_x86.h"
#elif defined(__mips__)
#  include "bionic_atomic_mips.h"
#else
#  include "bionic_atomic_gcc_builtin.h"
#endif

#define ANDROID_MEMBAR_FULL  __bionic_memory_barrier

#ifdef __cplusplus
} // extern "C"
#endif

#endif // BIONIC_ATOMIC_INLINE_H
