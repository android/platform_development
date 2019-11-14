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

typedef float CppBuiltinTypedef;
typedef CppDummyClass CppClassTypedef;
typedef struct CppStructTypedef {int foo1; int foo2;} CppStructTypedef;

class CppClass {
 public:
  CppClass();
  CppClass(int dummyInt);
  CppClass(int first, char second, bool third);
  ~CppClass();

  void passBuiltin(float foo);
  void passConstBuiltin(const float foo);
  float returnBuiltin();
  const float returnConstBuiltin();  

  void doFooThreeParameters(int first, char second, bool third);

  static int returnStaticBuiltin();
  static void passStaticBuiltinThreeParameters(int first, char second, bool third);

  void passArray(float foo[]);
  void passConstArray(const float foo[]);

  void passFixedSizeArray(float foo[9]);
  void passConstFixedSizeArray(const float foo[9]);

  void passBuiltinPointer(float* foo);
  void passConstBuiltinPointer(const float* foo);
  float* returnBuiltinPointer();
  const float* returnConstBuiltinPointer();

  int doOverloaded();
  int doOverloaded(int x);

  void noParameterName(int);

  static CppEnum returnEnum();
  static void passInEnum(CppEnum foo);

  char returnChar();
  void passInChar(char foo);

  void passPointer(CppDummyClass* dummy);
  void passConstPointer(const CppDummyClass* foo);
  CppDummyClass* returnPointer();
  const CppDummyClass* returnConstPointer();

  void passRef(CppDummyClass& dummy);
  void passConstRef(const CppDummyClass& dummy);
  CppDummyClass& returnRef();
  const CppDummyClass& returnConstRef();

  void passByValue(CppDummyClass dummy);
  CppDummyClass returnByValue();

  void passFunctionPointer(void* (*foo)(int));

  void passBuiltinTypedef(CppBuiltinTypedef foo);
  CppBuiltinTypedef returnBuiltinTypedef();

  void passTypedef(CppClassTypedef foo);
  CppClassTypedef returnTypedef();

  typedef CppDummyClass& CppDummyRef;
  void passMemberTypedef(CppDummyRef foo);
  CppDummyRef returnMemberTypedef();

  void passStructTypedef(CppStructTypedef foo);
  CppStructTypedef returnStructTypedef();

 private:
  int dummyInt_;
  CppDummyClass* dummyClass_;
};

}  // namespace dummynamespace