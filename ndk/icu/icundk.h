/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *            http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

/**
 * @addtogroup ICU ICU4C
 * @{
 */

/**
 * @file icundk.h
 * @brief
 */

#include <stdbool.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

/**
 * @brief Returns true if an ICU function is available at runtime.
 *
 * Since older devices will have older versions of ICU, it may be necessary to
 * check that a function is available before calling it.
 *
 * @param name The name of the function to check the availability of.
 * @return True if the function is available, false otherwise.
 */
bool ndk_is_icu_function_available(const char* name);

__END_DECLS

/**
 * @}
 */
