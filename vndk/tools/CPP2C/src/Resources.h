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

#ifndef CPP2C_RESOURCES_H_
#define CPP2C_RESOURCES_H_

#include <string>
#include <vector>

namespace cpp2c {

/** Resource loading utils **/
class Resources final {
 public:
  static Resources* CreateResource(const std::string file_name);

  const std::vector<std::string> getResources() const;

 private:
  Resources(const std::string file_name);
  void init();

  std::vector<std::string> resources;
  std::string file_name;
};

}  // namespace cpp2c

#endif  // CPP2C_RESOURCES_H_