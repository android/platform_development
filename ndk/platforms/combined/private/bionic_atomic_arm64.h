/*
 * Copyright (C) 2013 The Android Open Source Project
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
#ifndef BIONIC_ATOMIC_AARCH64_H
#define BIONIC_ATOMIC_AARCH64_H

/* For ARMv8, we can use the 'dmb' instruction directly */
__ATOMIC_INLINE__ void __bionic_memory_barrier() {
  __asm__ __volatile__ ( "dmb ish" : : : "memory" );
}

/* Compare-and-swap, without any explicit barriers. Note that this function
 * returns 0 on success, and 1 on failure. The opposite convention is typically
 * used on other platforms.
 */
__ATOMIC_INLINE__ int __bionic_cmpxchg(int32_t old_value, int32_t new_value, volatile int32_t* ptr) {
  int32_t tmp, oldval;
  __asm__ __volatile__ (
      "// atomic_cmpxchg\n"
      "1:  ldxr %w1, [%3]\n"
      "    cmp %w1, %w4\n"
      "    b.ne 2f\n"
      "    stxr %w0, %w5, [%3]\n"
      "    cbnz  %w0, 1b\n"
      "2:"
      : "=&r" (tmp), "=&r" (oldval), "+o"(*ptr)
      : "r" (ptr), "Ir" (old_value), "r" (new_value)
      : "cc", "memory");
  return oldval != old_value;
}

/* Swap, without any explicit barriers.  */
__ATOMIC_INLINE__ int32_t __bionic_swap(int32_t new_value, volatile int32_t* ptr) {
  int32_t prev, status;
  __asm__ __volatile__ (
      "// atomic_swap\n"
      "1:  ldxr %w0, [%3]\n"
      "    stxr %w1, %w4, [%3]\n"
      "    cbnz %w1, 1b\n"
      : "=&r" (prev), "=&r" (status), "+o" (*ptr)
      : "r" (ptr), "r" (new_value)
      : "cc", "memory");
  return prev;
}

/* Atomic decrement, without explicit barriers.  */
__ATOMIC_INLINE__ int32_t __bionic_atomic_dec(volatile int32_t* ptr) {
  int32_t prev, tmp, status;
  __asm__ __volatile__ (
      "1:  ldxr %w0, [%4]\n"
      "    sub %w1, %w0, #1\n"
      "    stxr %w2, %w1, [%4]\n"
      "    cbnz %w2, 1b"
      : "=&r" (prev), "=&r" (tmp), "=&r" (status), "+m"(*ptr)
      : "r" (ptr)
      : "cc", "memory");
  return prev;
}

#endif /* BIONIC_ATOMICS_AARCH64_H */
