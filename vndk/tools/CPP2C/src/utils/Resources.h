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
namespace utils {

/** Resource loading utils
 * This class gets passed in the file corresponding to the '-wrap' command line
 * argument It then proceeds to parse the file and store a collection of strings
 * corresponding to the classes to be wrapped
 **/
class Resources final {
 public:
  /**
   * Factory function to create and initialise a Resources object
   */
  static Resources* CreateResource(const std::string file_name);

  /**
   * Returns the collection of classes to be wrapped
   */
  const std::vector<std::string>& getResources() const;

 private:
  Resources(const std::string file_name);
  void init();

  std::vector<std::string> resources_;
  std::string file_name_;
};

}  // namespace utils
}  // namespace cpp2c

#endif  // CPP2C_RESOURCES_H_