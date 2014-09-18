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

package com.android.testing.mockime;

import android.content.Context;
import android.inputmethodservice.Keyboard.Key;
import android.inputmethodservice.KeyboardView;
import android.util.AttributeSet;
import android.view.inputmethod.InputMethodManager;

/**
 * An object that describes the behavior of the Mock IME keyboard view.
 */
public class MockImeKeyboardView extends KeyboardView {

  public MockImeKeyboardView(Context context, AttributeSet attrs) {
    super(context, attrs);
  }

  public MockImeKeyboardView(Context context, AttributeSet attrs, int defStyle) {
    super(context, attrs, defStyle);
  }

  @Override
  protected boolean onLongPress(Key key) {
    // On long press, switching the input type.
    if (key.codes[0] == ' ') {
      InputMethodManager im =
          (InputMethodManager) getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
      im.showInputMethodPicker();
      return true;
    } else {
      return super.onLongPress(key);
    }
  }

}
