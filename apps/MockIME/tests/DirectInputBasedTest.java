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

import android.os.Handler;
import android.text.InputType;
import android.view.KeyEvent;

import java.util.Collection;
import java.util.LinkedList;

/**
 * Class that tests IMEs with direct characters input. Chinese IME, voice ime, handwriting IME.
 *
 */
public class DirectInputBasedTest extends BaseTestCase {

  public DirectInputBasedTest(SoftKeyboard keyboard, Handler handler) {
    super(keyboard, handler);
    notSupportedFieldTypes.add(InputType.TYPE_CLASS_PHONE);
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

        simulateCharacterClick(KeyEvent.KEYCODE_T);
        simulateCharacterClick(KeyEvent.KEYCODE_O);
        simulateCharacterClick(KeyEvent.KEYCODE_U);
        simulateCharacterClick(KeyEvent.KEYCODE_K);
        simulateCharacterClick(KeyEvent.KEYCODE_Y);
        simulateCharacterClick(KeyEvent.KEYCODE_O);
        simulateCharacterClick(KeyEvent.KEYCODE_U);

        Collection<String> result = new LinkedList<String>();

        String internal = getCurrentTextInTheEditor().trim();
        if (!"toukyou".equalsIgnoreCase(internal)) {
          result.add("Simple buffer based input failed. Received '" + internal
              + "' instead of 'toukyou'. That my happen if InputConnection was "
              + "closed during the input and then reopened again.");
        }
        clearInputField();
        callback.onTestComplete(result);
      }
    });
    thread.start();
  }

  @Override
  public String getTestExplanation() {
    return "Test that checks a buffer content after an English word is entered.";
  }
}
