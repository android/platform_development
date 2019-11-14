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

#ifndef CPP2C_CUSTOMTYPEMANAGER_H_
#define CPP2C_CUSTOMTYPEMANAGER_H_

#include <memory>
#include <vector>

#include "src/ICustomTypeBase.h"

namespace cpp2c {

/**
 * Class resonsible for holding custom matchers that implement the
 * ICustomTypeBase interface.
 */
class CustomTypeManager final {
 public:
  template <typename T>
  void addType() {
    customClasses_.push_back(std::make_shared<T>());
  }

  /**
   * `match` is called when a C++ type needs to be converted to an equivalent C
   *
   * type. If it returns nullptr then no custom handling needs to be done for
   * the type and the tool attempts to do the best it can to transpile it.
   *
   * If there is a match then the corresponding ICustomTypeBase::addAsParameter
   * or ICustomTypeBase::addAsReturnType functions are called
   */
  std::shared_ptr<const ICustomTypeBase> match(
      const TypeMetadata& type_metadata) const;

 private:
  std::vector<std::shared_ptr<const ICustomTypeBase>> customClasses_;
};

}  // namespace cpp2c

#endif  // CPP2C_CUSTOMTYPEMANAGER_H_