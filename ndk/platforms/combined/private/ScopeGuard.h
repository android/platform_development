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

#ifndef _SCOPE_GUARD_H
#define _SCOPE_GUARD_H

#include "private/bionic_macros.h"

// TODO: include explicit std::move when it becomes available
template<typename F>
class ScopeGuard {
 public:
  ScopeGuard(F f) : f_(f), active_(true) {}

  ScopeGuard(ScopeGuard&& that) : f_(that.f_), active_(that.active_) {
    that.active_ = false;
  }

  ~ScopeGuard() {
    if (active_) {
      f_();
    }
  }

  void disable() {
    active_ = false;
  }
 private:
  F f_;
  bool active_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(ScopeGuard);
};

template<typename T>
ScopeGuard<T> make_scope_guard(T f) {
  return ScopeGuard<T>(f);
}

#endif  // _SCOPE_GUARD_H
