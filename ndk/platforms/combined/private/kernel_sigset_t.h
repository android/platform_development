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

#ifndef LIBC_PRIVATE_KERNEL_SIGSET_T_H_
#define LIBC_PRIVATE_KERNEL_SIGSET_T_H_

#include <signal.h>

// Our sigset_t is wrong for ARM and x86. It's 32-bit but the kernel expects 64 bits.
// This means we can't support real-time signals correctly until we can change the ABI.
// In the meantime, we can use this union to pass an appropriately-sized block of memory
// to the kernel, at the cost of not being able to refer to real-time signals.
union kernel_sigset_t {
  kernel_sigset_t() {
    clear();
  }

  kernel_sigset_t(const sigset_t* value) {
    clear();
    set(value);
  }

  void clear() {
    __builtin_memset(this, 0, sizeof(*this));
  }

  void set(const sigset_t* value) {
    bionic = *value;
  }

  sigset_t* get() {
    return &bionic;
  }

  sigset_t bionic;
#ifndef __mips__
  uint32_t kernel[2];
#endif
};

#endif
