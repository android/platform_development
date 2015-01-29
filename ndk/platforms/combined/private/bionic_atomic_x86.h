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
#ifndef BIONIC_ATOMIC_X86_H
#define BIONIC_ATOMIC_X86_H

/* Define a full memory barrier, this is only needed if we build the
 * platform for a multi-core device.
 */
__ATOMIC_INLINE__ void __bionic_memory_barrier() {
  __asm__ __volatile__ ( "mfence" : : : "memory" );
}

/* Compare-and-swap, without any explicit barriers. Note that this function
 * returns 0 on success, and 1 on failure. The opposite convention is typically
 * used on other platforms.
 */
__ATOMIC_INLINE__ int __bionic_cmpxchg(int32_t old_value, int32_t new_value, volatile int32_t* ptr) {
    int32_t prev;
    __asm__ __volatile__ ("lock; cmpxchgl %1, %2"
                          : "=a" (prev)
                          : "q" (new_value), "m" (*ptr), "0" (old_value)
                          : "memory");
    return prev != old_value;
}

/* Swap, without any explicit barriers. */
__ATOMIC_INLINE__ int32_t __bionic_swap(int32_t new_value, volatile int32_t *ptr) {
  __asm__ __volatile__ ("xchgl %1, %0"
                        : "=r" (new_value)
                        : "m" (*ptr), "0" (new_value)
                        : "memory");
  return new_value;
}

/* Atomic decrement, without explicit barriers. */
__ATOMIC_INLINE__ int32_t __bionic_atomic_dec(volatile int32_t* ptr) {
  int increment = -1;
  __asm__ __volatile__ ("lock; xaddl %0, %1"
                        : "+r" (increment), "+m" (*ptr)
                        : : "memory");
  /* increment now holds the old value of *ptr */
  return increment;
}

#endif /* BIONIC_ATOMIC_X86_H */
