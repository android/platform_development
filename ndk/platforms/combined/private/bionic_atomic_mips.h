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
#ifndef BIONIC_ATOMIC_MIPS_H
#define BIONIC_ATOMIC_MIPS_H

/* Define a full memory barrier, this is only needed if we build the
 * platform for a multi-core device.
 */

__ATOMIC_INLINE__ void __bionic_memory_barrier() {
  __asm__ __volatile__ ( "sync" : : : "memory" );
}

/* Compare-and-swap, without any explicit barriers. Note that this function
 * returns 0 on success, and 1 on failure. The opposite convention is typically
 * used on other platforms.
 */
__ATOMIC_INLINE__ int __bionic_cmpxchg(int32_t old_value, int32_t new_value, volatile int32_t* ptr) {
  int32_t prev, status;
  __asm__ __volatile__ ("1: move %[status], %[new_value]  \n"
                        "   ll %[prev], 0(%[ptr])         \n"
                        "   bne %[old_value], %[prev], 2f \n"
                        "   sc   %[status], 0(%[ptr])     \n"
                        "   beqz %[status], 1b            \n"
                        "2:                               \n"
                        : [prev]"=&r"(prev), [status]"=&r"(status), "+m"(*ptr)
                        : [new_value]"r"(new_value), [old_value]"r"(old_value), [ptr]"r"(ptr)
                        : "memory");
  return prev != old_value;
}

/* Swap, without any explicit barriers. */
__ATOMIC_INLINE__ int32_t __bionic_swap(int32_t new_value, volatile int32_t* ptr) {
  int32_t prev, status;
  __asm__ __volatile__ ("1:  move %[status], %[new_value] \n"
                        "    ll %[prev], 0(%[ptr])        \n"
                        "    sc %[status], 0(%[ptr])      \n"
                        "    beqz %[status], 1b           \n"
                        : [prev]"=&r"(prev), [status]"=&r"(status), "+m"(*ptr)
                        : [ptr]"r"(ptr), [new_value]"r"(new_value)
                        : "memory");
  return prev;
}

/* Atomic decrement, without explicit barriers. */
__ATOMIC_INLINE__ int32_t __bionic_atomic_dec(volatile int32_t* ptr) {
  int32_t prev, status;
  __asm__ __volatile__ ("1:  ll %[prev], 0(%[ptr])        \n"
                        "    addiu %[status], %[prev], -1 \n"
                        "    sc   %[status], 0(%[ptr])    \n"
                        "    beqz %[status], 1b           \n"
                        : [prev]"=&r" (prev), [status]"=&r"(status), "+m" (*ptr)
                        : [ptr]"r"(ptr)
                        : "memory");
  return prev;
}

#endif /* BIONIC_ATOMIC_MIPS_H */
