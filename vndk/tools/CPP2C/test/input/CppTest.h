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

namespace dummynamespace {

enum class CppEnum {
  kFirstElement,
  kSecondElement,
  kThirdElement,
};

class CppDummyClass {};

class CppClass {
 public:
  CppClass();
  CppClass(int dummyInt);
  CppClass(int first, char second, bool third);
  ~CppClass();

  int doFoo();
  void doFooThreeParameters(int first, char second, bool third);

  static int doStaticFoo();
  static void doStaticFooThreeParameters(int first, char second, bool third);

  int doOverloaded();
  int doOverloaded(int x);

  void noParameterName(int);

  int returnInt();
  void passInInt(int foo);

  static CppEnum returnEnum();
  static void passInEnum(CppEnum foo);

  char returnChar();
  void passInChar(char foo);

  CppDummyClass& returnRef();
  void passPointer(CppDummyClass* dummy);
  void passRef(const CppDummyClass& dummy);
  void passByValue(CppDummyClass dummy);
  CppDummyClass returnByValue();

  void passFunctionPointer(void* (*foo)(int));

 private:
  int dummyInt_;
  CppDummyClass* dummyClass_;
};

}  // namespace dummynamespace