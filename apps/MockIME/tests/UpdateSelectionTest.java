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

/** An object that tests if onUpdateSelection for custom input fields work properly. */
public class UpdateSelectionTest extends BaseTestCase {

  /** Default constructor. */
  public UpdateSelectionTest(SoftKeyboard keyboard, Handler handler) {
    super(keyboard, handler);
    notSupportedFieldTypes.add(InputType.TYPE_CLASS_PHONE);
    notSupportedFieldTypes.add(InputType.TYPE_CLASS_NUMBER);
    notSupportedFieldTypes.add(InputType.TYPE_CLASS_DATETIME);
  }

  @Override
  public void runTheTest(final TestRunCallback callback, int fieldType) {
    keyboard.setmCompletionOn(true);
    Thread thread = new Thread(new Runnable() {
      @Override
      public void run() {
        clearInputField();
        simulateCharacterClick(KeyEvent.KEYCODE_F);
        simulateCharacterClick(KeyEvent.KEYCODE_I);
        simulateCharacterClick(KeyEvent.KEYCODE_S);
        simulateCharacterClick(KeyEvent.KEYCODE_T);

        runSynchronously(new SyncRunnable() {
          @Override
          protected void realRun() {
            keyboard.pickSuggestionManually(0);
          }
        });
        
        runSynchronously(new SyncRunnable() {
          @Override
          protected void realRun() {
            keyboard.getCurrentInputConnection().setSelection(2, 2);
          }
        });

        simulateCharacterClick(KeyEvent.KEYCODE_R);

        runSynchronously(new SyncRunnable() {
          @Override
          protected void realRun() {
            keyboard.pickSuggestionManually(0);
          }
        });

        Collection<String> result = new LinkedList<String>();

        String internal = getCurrentTextInTheEditor().trim();
        if (!"first".equals(internal)) {
          result.add(String.format(
              "Test of selection change functionality failed. \"%s\" receved instead of \"first\"",
              internal));
        }
        clearInputField();
        callback.onTestComplete(result);
      }
    });
    thread.start();
  }

  @Override
  public String getTestExplanation() {
    return "Checks if the update selection behaves correctly.";
  }
}
