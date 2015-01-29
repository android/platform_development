/*
 * Copyright (C) 2011 The Android Open Source Project
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
#ifndef BIONIC_ATOMIC_GCC_BUILTIN_H
#define BIONIC_ATOMIC_GCC_BUILTIN_H

/*
 * This header file is used by default if we don't have optimized atomic
 * routines for a given platform. See bionic_atomic_arm.h and
 * bionic_atomic_x86.h for examples.
 *
 * Note that the GCC builtins include barriers that aren't present in
 * the architecture-specific assembler versions.
 */

__ATOMIC_INLINE__ void __bionic_memory_barrier() {
  __sync_synchronize();
}

__ATOMIC_INLINE__ int __bionic_cmpxchg(int32_t old_value, int32_t new_value, volatile int32_t* ptr) {
  /* We must return 0 on success. */
  return __sync_val_compare_and_swap(ptr, old_value, new_value) != old_value;
}

__ATOMIC_INLINE__ int32_t __bionic_swap(int32_t new_value, volatile int32_t* ptr) {
  int32_t old_value;
  do {
    old_value = *ptr;
  } while (__sync_val_compare_and_swap(ptr, old_value, new_value) != old_value);
  return old_value;
}

__ATOMIC_INLINE__ int32_t __bionic_atomic_dec(volatile int32_t* ptr) {
  /* We must return the old value. */
  return __sync_fetch_and_add(ptr, -1);
}

#endif /* BIONIC_ATOMIC_GCC_BUILTIN_H */
