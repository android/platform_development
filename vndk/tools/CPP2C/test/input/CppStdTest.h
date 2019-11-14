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

#include <memory>
#include <string>

namespace dummynamespace {

class CppClass {
 public:
  static std::unique_ptr<CppClass> returnUniquePointer();
  static void passUniquePointer(std::unique_ptr<CppClass> foo);

  static std::shared_ptr<CppClass> returnSharedPtr();
  static void passSharedPtr(std::shared_ptr<CppClass> foo);

  static std::string returnString();
  static void passString(std::string foo);
};

}  // namespace dummynamespace