/*
 * Copyright (C) 2015 The Android Open Source Project
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

#pragma version(1)
#pragma rs java_package_name(com.example.android.rs.reduce)

rs_allocation inputAlloc;

// A reduce kernel that returns the index of the smallest value in inputAlloc.
int32_t __attribute__((kernel("reduce"))) indexOfMin(int32_t idx1, int32_t idx2) {
  float f1 = rsGetElementAt_float(inputAlloc, idx1);
  float f2 = rsGetElementAt_float(inputAlloc, idx2);
  return (f1 <= f2) ? idx1 : idx2;
}

// A reduce kernel that returns the maximum float value.
float __attribute__((kernel("reduce"))) maxf(float lhs, float rhs) {
  return (lhs >= rhs) ? lhs : rhs;
}

// A reduce kernel that adds the inputs together.
float __attribute__((kernel("reduce"))) addFloat(float lhs, float rhs) {
  return lhs + rhs;
}

// A forEach kernel that multiplies the inputs together.
float RS_KERNEL mulFloat(float lhs, float rhs) {
  return lhs * rhs;
}
