/*
 * Copyright (C) 2013 The Android Open Source Project
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

#ifndef SCOPED_READDIR_H
#define SCOPED_READDIR_H

#include <dirent.h>

#include "private/bionic_macros.h"

class ScopedReaddir {
 public:
  ScopedReaddir(const char* path) {
    dir_ = opendir(path);
  }

  ~ScopedReaddir() {
    if (dir_ != NULL) {
      closedir(dir_);
    }
  }

  bool IsBad() {
    return dir_ == NULL;
  }

  dirent* ReadEntry() {
    return readdir(dir_);
  }

 private:
  DIR* dir_;

  DISALLOW_COPY_AND_ASSIGN(ScopedReaddir);
};

#endif // SCOPED_READDIR_H
