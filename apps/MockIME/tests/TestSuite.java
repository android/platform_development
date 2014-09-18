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

package com.android.MockIME.tests;

import com.android.MockIME.SoftKeyboard;
import com.google.common.collect.Lists;

import android.os.Handler;

import java.util.Collection;
import java.util.Iterator;
import java.util.LinkedList;

/**
 * An object that runs all predefined tests at once.
 */
public class TestSuite {

  private final Collection<TestCase> tests = Lists.newLinkedList();

  public TestSuite(SoftKeyboard keyboard, Handler handler) {
    tests.add(new CandidateBasedInputTest(keyboard, handler));
    // TODO(antkrumin): Uncomment when the reason of this bug would be figured out.
    // tests.add(new CandidateBasedInputTest2(keyboard, handler));
    tests.add(new DirectInputBasedTest(keyboard, handler));
    tests.add(new DirectCJKInput(keyboard, handler));
    tests.add(new PhoneInputTest(keyboard, handler));
    tests.add(new UpdateSelectionTest(keyboard, handler));
  }

  /**
   * Returns the string explanations for the tests that were executed.
   *
   * @return collection of tests explanations
   */
  public Collection<String> getTestsExplanation() {
    Collection<String> result = Lists.newArrayListWithCapacity(tests.size());
    for (TestCase test : tests) {
      result.add(test.getTestExplanation());
    }
    return result;
  }

  public void runTheTests(final TestRunCallback callback, final int inputType) {
    final Collection<String> result = new LinkedList<String>();
    runAllTests(callback, tests.iterator(), result, inputType);
  }

  /**
   * Runs all the tests from the given iterator. Because of the multithreading nature of the app,
   * uses callback to inform caller about test results. Recursive method.
   *
   * @param callback instance of the {@link TestRunCallback} class that would be called after tests
   *        complete
   * @param iterator of the collection of the {@link TestCase} instances
   * @param result mutable final collection of the results
   * @param inputType is the integer describing the android input field type
   */
  private void runAllTests(final TestRunCallback callback, final Iterator<TestCase> iterator,
      final Collection<String> result, final int inputType) {
    if (iterator.hasNext()) {
      TestCase testCase = iterator.next();
      Collection<Integer> notSupportedTypes = testCase.notSupportedFieldType();
      if (notSupportedTypes == null || notSupportedTypes.isEmpty()
          || !notSupportedTypes.contains(inputType)) {
        testCase.runTheTest(new TestRunCallback() {
          @Override
          public void onTestComplete(Collection<String> testResult) {
            result.addAll(testResult);
            runAllTests(callback, iterator, result, inputType);
          }
        }, inputType);
      } else {
        runAllTests(callback, iterator, result, inputType);
      }
    } else {
      callback.onTestComplete(result);
    }

  }
}
