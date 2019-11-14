/*
 * Copyright 2019 The Android Open Source Project
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

#include <utils/RefBase.h>
#include <utils/StrongPointer.h>

namespace dummynamespace {
using namespace android;

class RefCountedClass : public VirtualLightRefBase {};

class HwuiTest final {
 public:
  static sp<RefCountedClass> returnSp();
  static void passSp(sp<RefCountedClass> foo);

  static void passSpRef(const sp<RefCountedClass>& foo);
};
}  // namespace dummynamespace