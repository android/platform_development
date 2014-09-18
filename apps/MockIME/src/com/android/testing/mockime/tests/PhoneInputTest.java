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

import com.android.testing.mockime.SoftKeyboard;

import android.os.Handler;
import android.text.InputType;
import android.view.KeyEvent;

import java.util.Collection;
import java.util.LinkedList;

/**
 * Class that tests if numbers input is OK.
 *
 */
public class PhoneInputTest extends BaseTestCase {

  public PhoneInputTest(SoftKeyboard keyboard, Handler handler) {
    super(keyboard, handler);
    notSupportedFieldTypes.add(InputType.TYPE_CLASS_NUMBER);
    notSupportedFieldTypes.add(InputType.TYPE_CLASS_DATETIME);
  }

  @Override
  public void runTheTest(final TestRunCallback callback, int fieldType) {
    keyboard.setmCompletionOn(false);
    Thread thread = new Thread(new Runnable() {
      @Override
      public void run() {
        clearInputField();

        simulateCharacterClick(KeyEvent.KEYCODE_PLUS);
        simulateCharacterClick(KeyEvent.KEYCODE_1);
        simulateCharacterClick(KeyEvent.KEYCODE_MINUS);
        simulateCharacterClick(KeyEvent.KEYCODE_0);
        simulateCharacterClick(KeyEvent.KEYCODE_1);
        simulateCharacterClick(KeyEvent.KEYCODE_2);
        simulateCharacterClick(KeyEvent.KEYCODE_MINUS);
        simulateCharacterClick(KeyEvent.KEYCODE_3);
        simulateCharacterClick(KeyEvent.KEYCODE_4);
        simulateCharacterClick(KeyEvent.KEYCODE_5);
        simulateCharacterClick(KeyEvent.KEYCODE_6);

        Collection<String> result = new LinkedList<String>();

        String internal = getCurrentTextInTheEditor().trim();
        if (!"+1-012-3456".equals(internal)) {
          result.add("Simple buffer based phone number input test failed. " + "Received '"
              + internal + "' instead of '+1-012-3456'. "
              + "That my happen if InputConnection was closed during the input "
              + "and then reopened again.");
        }
        clearInputField();
        callback.onTestComplete(result);
      }
    });
    thread.start();
  }

  @Override
  public String getTestExplanation() {
    return "Test that checks the simple buffered direct input of a phone number.";
  }

}
