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

import java.util.Collection;
import java.util.LinkedList;

/**
 * Class that tests IMEs with direct characters input. Chinese IME, voice ime, handwriting IME.
 *
 */
public class DirectCJKInput extends BaseTestCase {

  public DirectCJKInput(SoftKeyboard keyboard, Handler handler) {
    super(keyboard, handler);
    notSupportedFieldTypes.add(InputType.TYPE_CLASS_PHONE);
    notSupportedFieldTypes.add(InputType.TYPE_CLASS_NUMBER);
  }

  @Override
  public void runTheTest(final TestRunCallback callback, int fieldType) {
    keyboard.setmCompletionOn(false);
    Thread thread = new Thread(new Runnable() {
      @Override
      public void run() {
        clearInputField();

        simulateCharacterClick('東');
        simulateCharacterClick('京');

        Collection<String> result = new LinkedList<String>();

        String internal = getCurrentTextInTheEditor().trim();
        if (!"東京".equals(internal)) {
          result.add("Simple buffer based CJK input failed. Received '" + internal
              + "' instead of '東京'. That my happen if InputConnection was "
              + "closed during the input and then reopened again.");
        }
        clearInputField();
        callback.onTestComplete(result);
      }
    });
    thread.start();
  }

  @Override
  protected void simulateCharacterClick(final int charId) {
    runSynchronously(new SyncRunnable() {
      @Override
      protected void realRun() {
        keyboard.onKey(charId, null);
      }
    });
  }

  @Override
  public String getTestExplanation() {
    return "Test that checks the direct insertion of some CJK characters.";
  }
}
