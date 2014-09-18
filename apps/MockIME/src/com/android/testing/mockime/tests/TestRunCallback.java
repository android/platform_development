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

package com.android.testing.mockime.tests;

import java.util.Collection;

/**
 * Interface that explains the callback that would be invoked when test is over.
 *
 */
public interface TestRunCallback {

  /**
   * Method is invoking when test is complete. Test implementation should provide this method with
   * correct input parameter.
   *
   * @param testResult - empty collection if test is successful and populated with error details if
   *                   not.
   */
  void onTestComplete(Collection<String> testResult);
}
