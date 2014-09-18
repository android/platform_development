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
import android.text.TextUtils;
import android.view.KeyEvent;
import android.view.inputmethod.ExtractedText;
import android.view.inputmethod.ExtractedTextRequest;
import android.view.inputmethod.InputConnection;

import java.util.Collection;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Implementation of the TestCase interface that contains set of the common methods that could be
 * used for the IME test implementation.
 *
 */
public abstract class BaseTestCase implements TestCase {

  private final Logger log = Logger.getLogger(BaseTestCase.class.getCanonicalName());

  protected final SoftKeyboard keyboard;
  protected Handler mainHandler;
  protected final Object lock = new Object();
  protected final Collection<Integer> notSupportedFieldTypes = Lists.newLinkedList();

  public BaseTestCase(SoftKeyboard keyboard, Handler handler) {
    assert (keyboard != null);
    this.keyboard = keyboard;
    this.mainHandler = handler;
  }

  @Override
  public Collection<Integer> notSupportedFieldType() {
    return notSupportedFieldTypes;
  }

  /**
   * Method is typing one character to the keyboard specified.
   *
   * @param charId - integer that represents this char id.
   */
  protected void simulateCharacterClick(final int charId) {
    runSynchronously(new SyncRunnable() {
      @Override
      protected void realRun() {
        simulateKeyClick(charId);
      }
    });
  }


  /**
   * Method emulating clicking on the virtual keyboard. Should be invoked in the UI thread,
   * Extracted for testing purposes.
   *
   * @param charId - integer that represents this char id.
   */
  /* default */final void simulateKeyClick(int charId) {
    keyboard.onKeyDown(charId, new KeyEvent(KeyEvent.ACTION_DOWN, charId));
    keyboard.onKeyUp(charId, new KeyEvent(KeyEvent.ACTION_UP, charId));
  }

  /**
   * Method is returning current content of the editor.
   *
   * @return content of the input controller.
   */
  protected String getCurrentTextInTheEditor() {
    InputConnection ic = keyboard.getCurrentInputConnection();
    ExtractedText text = ic.getExtractedText(new ExtractedTextRequest(), 0);
    if (text != null) {
      if (text.text == null) {
        return "";
      } else {
        return text.text.toString();
      }
    } else {
      // 1000 does not matter anything. Every relatively big const would be OK.
      CharSequence bData = ic.getTextBeforeCursor(1000, 0);
      CharSequence aData = ic.getTextAfterCursor(1000, 0);
      if (bData != null || aData != null) {
        String result =
            (bData == null ? "" : bData.toString()) + (aData == null ? "" : aData.toString());
        return result;
      }
    }
    return "";
  }

  /**
   * Clears the input field of the current input connection.
   */
  protected void clearInputField() {
    if (!TextUtils.isEmpty(getCurrentTextInTheEditor().trim())) {
      runSynchronously(new SyncRunnable() {
        @Override
        protected void realRun() {
          internalClearInputField();
        }
      });
    }
  }

  /**
   * Clears the input field by means of InputConnection. Should be invoked in the UI thread,
   * Extracted for testing purposes.
   */
  /* default */void internalClearInputField() {
    InputConnection ic = keyboard.getCurrentInputConnection();
    String text = BaseTestCase.this.getCurrentTextInTheEditor();
    if (text != null) {
      ic.deleteSurroundingText(text.length(), text.length());
    }
    ic.deleteSurroundingText(0, 0);
  }

  /**
   * Runs the specified runnable within the UI thread and freezes current thread until action is
   * executed.
   *
   * @param runnable - instance of SyncRunnable class that contains the logic that should be
   *        executed in the main thread.
   */
  protected void runSynchronously(SyncRunnable runnable) {
    synchronized (lock) {
      try {
        // Some applicatons do not handle immediate IME input properly. Given they work with user
        // input correctly, slow down character input rate to avoid false positives.
        mainHandler.postDelayed(runnable, 50);
        lock.wait(2000);
      } catch (InterruptedException e) {
        log.log(Level.WARNING,
            "InterruptedException happened during a command execution in the UI thread");
      }
    }
  }

  /**
   * Class that extends the Runnable interface with threads notification after the execution is
   * ended.
   */
  protected abstract class SyncRunnable implements Runnable {
    @Override
    public void run() {
      realRun();
      synchronized (lock) {
        lock.notifyAll();
      }
    }

    /**
     * Actual runnable run implementation.
     */
    protected abstract void realRun();
  }
}
