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
 * This is the interface that should be implemented for each common IME compatibility test.
 *
 */
public interface TestCase {

  /**
   * Runs the test.
   *
   * @param callback - method that would be invoked after test completes
   * @param fieldType is the android input field type could be found in the
   *        {@link android.text.InputType} interface
   */
  void runTheTest(TestRunCallback callback, int fieldType);


  /**
   * Returns the collection of the field types that are <b>not</b> supported by this test.
   *
   * @return Collection of fields that are not supported by this test. If empty or null, then all
   *         fields are supported
   */
  Collection<Integer> notSupportedFieldType();

  /**
   * Returns the text explanation of what this particular test case is doing.
   *
   * @return string explanation of the test. Never return null
   */
  String getTestExplanation();
}
