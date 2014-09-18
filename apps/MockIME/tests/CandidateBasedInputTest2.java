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
 * Class that tests number of candidate window based input systems like katakana, romaji, etc.
 *
 */
public class CandidateBasedInputTest2 extends BaseTestCase {

  private volatile int counter = 0;

  /**
   * Empty constructor.
   */
  public CandidateBasedInputTest2(SoftKeyboard keyboard, Handler handler) {
    super(keyboard, handler);
    notSupportedFieldTypes.add(InputType.TYPE_CLASS_PHONE);
    notSupportedFieldTypes.add(InputType.TYPE_CLASS_NUMBER);
    notSupportedFieldTypes.add(InputType.TYPE_CLASS_DATETIME);
  }

  @Override
  public void runTheTest(final TestRunCallback callback, final int fieldType) {
    keyboard.setmCompletionOn(true);
    Thread thread = new Thread(new Runnable() {
      @Override
      public void run() {
        clearInputField();

        simulateCharacterClick(KeyEvent.KEYCODE_S);
        simulateCharacterClick(KeyEvent.KEYCODE_E);
        simulateCharacterClick(KeyEvent.KEYCODE_A);
        simulateCharacterClick(KeyEvent.KEYCODE_R);
        simulateCharacterClick(KeyEvent.KEYCODE_C);
        simulateCharacterClick(KeyEvent.KEYCODE_H);
        runSynchronously(new SyncRunnable() {
          @Override
          protected void realRun() {
            keyboard.pickSuggestionManually(0);
          }
        });

        while (!"".equals(getCurrentTextInTheEditor().trim())) {
          simulateCharacterClick(KeyEvent.KEYCODE_DEL);
        }

        simulateCharacterClick(KeyEvent.KEYCODE_T);
        simulateCharacterClick(KeyEvent.KEYCODE_O);
        simulateCharacterClick(KeyEvent.KEYCODE_U);
        simulateCharacterClick(KeyEvent.KEYCODE_K);
        simulateCharacterClick(KeyEvent.KEYCODE_Y);
        simulateCharacterClick(KeyEvent.KEYCODE_O);
        simulateCharacterClick(KeyEvent.KEYCODE_U);

        Collection<String> result = new LinkedList<String>();
        if (!"toukyou".equalsIgnoreCase(keyboard.getmComposing().toString())) {
          result.add("The buffer contains unexpected set of data - '"
              + keyboard.getmComposing().toString() + "' instead of 'toukyou'");
        }
        runSynchronously(new SyncRunnable() {
          @Override
          protected void realRun() {
            keyboard.pickSuggestionManually(1);
            synchronized (lock) {
              lock.notify();
            }
          }
        });

        String internal = getCurrentTextInTheEditor();
        if (!"おうきょう".equals(internal)) {
          result.add("Committed data is not equals to the expected. Received '" + internal
              + "' instead of 'おうきょう'");
        }

        clearInputField();
        if (counter < 10 && result.isEmpty()) {
          counter++;
          CandidateBasedInputTest2.this.runTheTest(callback, fieldType);
        } else {
          callback.onTestComplete(result);
        }

      }
    });
    thread.start();
  }

  @Override
  public String getTestExplanation() {
    return "Test that checks fast input and deletion of the buffer content letter by letter.";
  }
}
