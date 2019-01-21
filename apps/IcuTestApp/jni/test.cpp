/*
 * Copyright 2018 The Android Open Source Project
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
#include <jni.h>

#include <android/log.h>
#include <dlfcn.h>

#include "unicode/ucnv.h"
#include "unicode/ucal.h"

#define APPNAME "IcuTestApp"

extern "C" {

int main (int argc, char *argv[]) {
  int32_t x = ucnv_countAvailable();
  __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "main DT_NEEEDED libicuuc.so ucnv_countAvailable is %d", x);
  int32_t y = ucal_countAvailable();
  __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "main DT_NEEEDED libicui18n.so ucal_countAvailable is %d", y);
}

JNIEXPORT jboolean JNICALL
Java_com_example_icu4ctestapp_MainActivity_testLibicuuc(JNIEnv *env) {
  int32_t x = ucnv_countAvailable();
  __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "DT_NEEEDED ucnv_countAvailable is %d", x);
  jboolean result = x > 0 ? JNI_TRUE : JNI_FALSE;
  
  return result;
}

JNIEXPORT jboolean JNICALL
Java_com_example_icu4ctestapp_MainActivity_testLibicui18n(JNIEnv *env) {
  int32_t x = ucal_countAvailable();
  __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "ucal_countAvailable is %d", x);
  return x > 0 ? JNI_TRUE : JNI_FALSE;
}

}