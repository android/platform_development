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

#ifndef CPP2C_HEADERCONVERTERFRONTENDACTIONFACTORY_H_
#define CPP2C_HEADERCONVERTERFRONTENDACTIONFACTORY_H_

#include <clang/Tooling/Tooling.h>

#include <string>

#include "src/HeaderConverterOptions.h"

namespace cpp2c {

/**
 * Class responsible for creating a FrontendAction.
 * Needed in order to pass options down the call chain
 */
class HeaderConverterFrontendActionFactory
    : public clang::tooling::FrontendActionFactory {
 public:
  HeaderConverterFrontendActionFactory(const HeaderConverterOptions& options);

  std::unique_ptr<clang::FrontendAction> create() override;

 private:
  const HeaderConverterOptions& options_;
};

}  // namespace cpp2c

#endif  // CPP2C_HEADERCONVERTERFRONTENDACTIONFACTORY_H_