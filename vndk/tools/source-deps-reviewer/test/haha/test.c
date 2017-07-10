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

// To work around b/25643775, we disable clang optimization so that
//   VTT for std::__1::basic_stringstream<char, std::__1::char_traits<char>,
//   std::__1::allocator<char> >
// will be correctly kept for other module's references.
#if defined(__clang__) && (defined(__arm__) || defined(__aarch64__))
#pragma clang optimize off
#endif
#include <gtest/gtest.h>
#if defined(__clang__) && (defined(__arm__) || defined(__aarch64__))
#pragma clang optimize on
#endif

#include <dlfcn.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>

#include <string>

TEST(atexit, sofile) {
  std::string atexit_call_sequence;
  bool valid_this_in_static_dtor = false;
  bool attr_dtor_called = false;

  void* handle = dlopen("libtest_atexit.so", RTLD_NOW);
  ASSERT_TRUE(handle != nullptr);

  typedef int (*int_fn)(void);
  int_fn get_cxx_ctor_called, get_attr_ctor_called;
  get_cxx_ctor_called = reinterpret_cast<int_fn>(dlsym(handle, "get_cxx_ctor_called"));
  get_attr_ctor_called = reinterpret_cast<int_fn>(dlsym(handle, "get_attr_ctor_called"));
  ASSERT_TRUE(get_cxx_ctor_called != nullptr);
  ASSERT_TRUE(get_attr_ctor_called != nullptr);

  ASSERT_EQ(1, get_cxx_ctor_called());
  ASSERT_EQ(1, get_attr_ctor_called());

  void* sym = dlsym(handle, "register_atexit");
  ASSERT_TRUE(sym != nullptr);
  reinterpret_cast<void (*)(std::string*, bool*, bool*)>(sym)(&atexit_call_sequence, &valid_this_in_static_dtor, &attr_dtor_called);

  ASSERT_EQ(0, dlclose(handle));
  // this test verifies atexit call from atexit handler. as well as the order of calls
  ASSERT_EQ("Humpty Dumpty sat on a wall", atexit_call_sequence);
  ASSERT_TRUE(valid_this_in_static_dtor);
  ASSERT_TRUE(attr_dtor_called);
}

class TestMainStaticDtorClass {
 public:
  TestMainStaticDtorClass() {
    expected_this = this;
  }

  ~TestMainStaticDtorClass() {
    if (this != expected_this) {
      fprintf(stderr, "\nerror: static d-tor called with incorrect this pointer: %p, expected: %p\n", this, expected_this);
    } else {
      fprintf(stderr, "6");
    }
  }
 private:
  static const TestMainStaticDtorClass* expected_this;
};

const TestMainStaticDtorClass* TestMainStaticDtorClass::expected_this = nullptr;

static void atexit_func5() {
  fprintf(stderr, "5");
}

static void atexit_func4() {
  fprintf(stderr, "4");
}

static void atexit_func3() {
  fprintf(stderr, "3");
  atexit(atexit_func4);
}

static void atexit_func2() {
  fprintf(stderr, "2");
}

static void atexit_func1() {
  fprintf(stderr, "1");
}

static void atexit_main() {
  // This should result in "123456" output to stderr
  static TestMainStaticDtorClass static_obj;
  atexit(atexit_func5);
  atexit(atexit_func3);
  atexit(atexit_func2);
  atexit(atexit_func1);
  exit(0);
}

TEST(atexit, exit) {
  ASSERT_EXIT(atexit_main(), testing::ExitedWithCode(0), "123456");
}

