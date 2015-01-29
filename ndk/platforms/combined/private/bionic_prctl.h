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

#ifndef BIONIC_PRCTL_H
#define BIONIC_PRCTL_H

#include <sys/prctl.h>

// This is only supported by Android kernels, so it's not in the uapi headers.
#define PR_SET_VMA   0x53564d41
#define PR_SET_VMA_ANON_NAME    0

#endif // BIONIC_PRCTL_H
