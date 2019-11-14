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

#include "Resources.h"

#include <fstream>
#include <iostream>

namespace cpp2c {

Resources* Resources::CreateResource(const std::string file_name) {
  Resources* resource = new Resources(file_name);
  resource->init();

  return resource;
}

Resources::Resources(const std::string file) : file_name(file) {}

void Resources::init() {
  std::ifstream classes_file(file_name);
  std::string class_name;

  if (classes_file.is_open()) {
    while (getline(classes_file, class_name)) {
      resources.push_back(class_name);
    }
    classes_file.close();
  } else {
    std::cout << "Error reading file" << std::endl;
  }
}

const std::vector<std::string> Resources::getResources() const {
  return resources;
}

}  // namespace cpp2c