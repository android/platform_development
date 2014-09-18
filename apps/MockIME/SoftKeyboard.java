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

package com.android.MockIME;

import com.android.MockIME.tests.TestRunCallback;
import com.android.MockIME.tests.TestSuite;

import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.inputmethodservice.InputMethodService;
import android.inputmethodservice.Keyboard;
import android.inputmethodservice.KeyboardView;
import android.net.Uri;
import android.os.Handler;
import android.provider.Browser;
import android.text.Html;
import android.text.InputType;
import android.text.Layout;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.method.LinkMovementMethod;
import android.text.method.MetaKeyKeyListener;
import android.text.style.URLSpan;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.view.inputmethod.CompletionInfo;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;

/**
 * An object that provides the mock IME keyboard functionality.
 */
public class SoftKeyboard extends InputMethodService implements
    KeyboardView.OnKeyboardActionListener {

  public static final String BROADCAST_CH_TO_RUN_DEFAULT_TESTS =
      "com.android.MockIME.TEST_DEFAULT";
  public static final String BROADCAST_CH_TO_SEND_TEST_RESULTS =
      "com.google.android.testing.i18n.imetester.controller.TEST_RESULT";
  public static final String EXTRA_NAME_WITH_TEST_RESULTS = "testResult";
  /**
   * Flag that switches ime from mock program to the real ime that is expecting key input.
   */
  private boolean mIsTestingInProgress = false;

  private MockImeKeyboardView mInputView;
  private CandidateView mCandidateView;
  private CompletionInfo[] mCompletions;

  private final StringBuilder mComposing = new StringBuilder();
  private volatile boolean mCompletionOn = false;
  private boolean mCapsLock;
  private long mLastShiftTime;
  private long mMetaState;

  private Keyboard mKeyboard;

  private String mWordSeparators;

  private Handler mMainHandler;
  private int mInputType = 0;

  private BroadcastReceiver receiver = null;

  /** Main initialization of the input method component. Be sure to call to super class. */
  @Override
  public void onCreate() {
    super.onCreate();
    mWordSeparators = getResources().getString(R.string.word_separators);
    mMainHandler = new Handler();
    IntentFilter filter = new IntentFilter(BROADCAST_CH_TO_RUN_DEFAULT_TESTS);
    receiver = new BroadcastReceiver() {
      @Override
      public void onReceive(Context context, final Intent intent) {
        mIsTestingInProgress = true;
        TestSuite test = new TestSuite(SoftKeyboard.this, mMainHandler);
        test.runTheTests(new TestRunCallback() {
          @Override
          public void onTestComplete(final Collection<String> testResult) {
            Intent intent = new Intent();
            intent.setAction(BROADCAST_CH_TO_SEND_TEST_RESULTS);
            if (testResult != null && !testResult.isEmpty()) {
              intent.putExtra(EXTRA_NAME_WITH_TEST_RESULTS,
                  testResult.toArray(new String[testResult.size()]));
            }
            SoftKeyboard.this.sendBroadcast(intent);
            mIsTestingInProgress = false;
          }
        }, mInputType);
      }
    };
    registerReceiver(receiver, filter);
  }

  @Override
  public void onDestroy() {
    super.onDestroy();
    unregisterReceiver(receiver);
  }

  /**
   * This is the point where you can do all of your UI initialization. It is called after creation
   * and any configuration change.
   */
  @Override
  public void onInitializeInterface() {
    mKeyboard = new Keyboard(this, R.xml.mock_ime);
  }

  /**
   * Called by the framework when your view for creating input needs to be generated. This will be
   * called the first time your input method is displayed, and every time it needs to be re-created
   * such as due to a configuration change.
   */
  @Override
  public View onCreateInputView() {
    mInputView = (MockImeKeyboardView) getLayoutInflater().inflate(R.layout.input, null);
    mInputView.setOnKeyboardActionListener(this);
    mInputView.setKeyboard(mKeyboard);
    return mInputView;
  }

  /**
   * Called by the framework when your view for showing candidates needs to be generated, like
   * {@link #onCreateInputView}.
   */
  @Override
  public View onCreateCandidatesView() {
    mCandidateView = new CandidateView(this);
    mCandidateView.setService(this);
    return mCandidateView;
  }

  /**
   * This is the main point where we do our initialization of the input method to begin operating on
   * an application. At this point we have been bound to the client, and are now receiving all of
   * the detailed information about the target of our edits.
   */
  @Override
  public void onStartInput(EditorInfo attribute, boolean restarting) {
    super.onStartInput(attribute, restarting);

    // Reset our state. We want to do this even if restarting, because
    // the underlying state of the text editor could have changed in any way.
    mComposing.setLength(0);
    updateCandidates();

    if (!restarting) {
      // Clear shift states.
      mMetaState = 0;
    }
    mCompletions = null;

    mInputType = attribute.inputType & InputType.TYPE_MASK_CLASS;
  }

  /**
   * This is called when the user is done editing a field. We can use this to reset our state.
   */
  @Override
  public void onFinishInput() {
    super.onFinishInput();

    // Clear current composing text and candidates.
    mComposing.setLength(0);
    updateCandidates();
    setCandidatesViewShown(false);

    if (mInputView != null) {
      mInputView.closing();
    }
  }

  @Override
  public void onStartInputView(EditorInfo attribute, boolean restarting) {
    super.onStartInputView(attribute, restarting);
    mInputView.setKeyboard(mKeyboard);
    mInputView.closing();
  }

  /** Deal with the editor reporting movement of its cursor. */
  @Override
  public void onUpdateSelection(int oldSelStart,
      int oldSelEnd,
      int newSelStart,
      int newSelEnd,
      int candidatesStart,
      int candidatesEnd) {
    super.onUpdateSelection(oldSelStart,
        oldSelEnd,
        newSelStart,
        newSelEnd,
        candidatesStart,
        candidatesEnd);

    // If the current selection in the text view changes, we should
    // clear whatever candidate text we have.
    if (mComposing.length() > 0 && (newSelStart != candidatesEnd || newSelEnd != candidatesEnd)) {
      mComposing.setLength(0);
      updateCandidates();
      InputConnection ic = getCurrentInputConnection();
      if (ic != null) {
        ic.finishComposingText();
      }
    }
  }

  /**
   * This translates incoming hard key events in to edit operations on an InputConnection. It is
   * only needed when using the PROCESS_HARD_KEYS option.
   */
  private boolean translateKeyDown(int keyCode, KeyEvent event) {
    mMetaState = MetaKeyKeyListener.handleKeyDown(mMetaState, keyCode, event);
    int c = event.getUnicodeChar(MetaKeyKeyListener.getMetaState(mMetaState));
    mMetaState = MetaKeyKeyListener.adjustMetaAfterKeypress(mMetaState);
    InputConnection ic = getCurrentInputConnection();
    if (c == 0 || ic == null) {
      return false;
    }

    if ((c & KeyCharacterMap.COMBINING_ACCENT) != 0) {
      c = c & KeyCharacterMap.COMBINING_ACCENT_MASK;
    }

    if (mComposing.length() > 0) {
      char accent = mComposing.charAt(mComposing.length() - 1);
      int composed = KeyEvent.getDeadChar(accent, c);

      if (composed != 0) {
        c = composed;
        mComposing.setLength(mComposing.length() - 1);
      }
    }

    onKey(c, null);

    return true;
  }

  /**
   * Use this to monitor key events being delivered to the application. We get first crack at them,
   * and can either resume them or let them continue to the app.
   */
  @Override
  public boolean onKeyDown(int keyCode, KeyEvent event) {
    switch (keyCode) {
      case KeyEvent.KEYCODE_BACK:
        if (event.getRepeatCount() == 0 && mInputView != null) {
          if (mInputView.handleBack()) {
            return true;
          }
        }
        break;

      case KeyEvent.KEYCODE_DEL:
        onKey(Keyboard.KEYCODE_DELETE, null);
        return true;

      case KeyEvent.KEYCODE_ENTER:
        return false;

      default:
        if (translateKeyDown(keyCode, event)) {
          return true;
        }
    }

    return super.onKeyDown(keyCode, event);
  }

  /**
   * Use this to monitor key events being delivered to the application. We get first crack at them,
   * and can either resume them or let them continue to the app.
   */
  @Override
  public boolean onKeyUp(int keyCode, KeyEvent event) {
    return super.onKeyUp(keyCode, event);
  }

  /**
   * Helper function to commit any text being composed in to the editor.
   */
  private void commitTyped(InputConnection inputConnection) {
    if (mComposing.length() > 0) {
      inputConnection.commitText(mComposing, mComposing.length());
      mComposing.setLength(0);
      updateCandidates();
    }
  }

  /**
   * Helper to determine if a given character code is alphabetic.
   */
  private boolean isAlphabet(int code) {
    if (Character.isLetter(code)) {
      return true;
    } else {
      return false;
    }
  }

  /**
   * Helper to send a key down / key up pair to the current editor.
   */
  private void keyDownUp(int keyEventCode) {
    getCurrentInputConnection().sendKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN, keyEventCode));
    getCurrentInputConnection().sendKeyEvent(new KeyEvent(KeyEvent.ACTION_UP, keyEventCode));
  }

  /**
   * Helper to send a character to the editor as raw key events.
   */
  private void sendKey(int keyCode) {
    if (keyCode >= '0' && keyCode <= '9') {
      keyDownUp(keyCode - '0' + KeyEvent.KEYCODE_0);
    } else {
      getCurrentInputConnection().commitText(String.valueOf((char) keyCode), 1);
    }
  }

  @Override
  public void onKey(int primaryCode, int[] keyCodes) {
    if (!mIsTestingInProgress) {
      if (String.valueOf((char) primaryCode).equals(" ")) {
        mIsTestingInProgress = true;
        // Running the default ImeTests on space press.
        final TestSuite test = new TestSuite(this, mMainHandler);
        test.runTheTests(new TestRunCallback() {
          @Override
          public void onTestComplete(final Collection<String> testResult) {
            SoftKeyboard.this.mIsTestingInProgress = false;
            if (testResult != null && !testResult.isEmpty()) {
              mMainHandler.post(new Runnable() {
                @Override
                public void run() {
                  StringBuilder errorMessage = new StringBuilder();
                  errorMessage.append("The following errors occurred during the testing:");
                  errorMessage.append("<br/>");
                  for (String err : testResult) {
                    errorMessage.append("- ");
                    errorMessage.append(err);
                    errorMessage.append("<br/>");
                  }
                  errorMessage.append("<br/>");
                  errorMessage.append("For more details please visit ");
                  errorMessage.append("<a href=\"http://go/androidimetest\">");
                  errorMessage.append("go/androidimetest.</a>");
                  errorMessage.append("<br/>");
                  errorMessage.append("<br/>");
                  errorMessage.append("<i> Mock IME Version - ");
                  errorMessage.append(getString(R.string.version_number));
                  errorMessage.append("</i>");
                  showMessageDialog("IME error.", errorMessage.toString());
                }
              });
            } else {
              mMainHandler.post(new Runnable() {
                @Override
                public void run() {
                  Collection<String> explanations = test.getTestsExplanation();
                  StringBuilder message = new StringBuilder();
                  message.append("IME tests passed successfully. ");
                  message.append("The following tests were executed:");
                  message.append("<br/>");
                  for (String explanation : explanations) {
                    message.append("- ");
                    message.append(explanation);
                    message.append("<br/>");
                  }
                  message.append("<br/>");
                  message.append("For more details please visit ");
                  message.append("<a href=\"http://go/androidimetest\">");
                  message.append("go/androidimetest.</a>");
                  message.append("<br/>");
                  message.append("<br/>");
                  message.append("<i> Mock IME Version - ");
                  message.append(getString(R.string.version_number));
                  message.append("</i>");
                  showMessageDialog("Success.", message.toString());
                }
              });
            }
          }
        }, mInputType);
      }
    } else {
      if (isWordSeparator(primaryCode)) {
        // Handle separator
        if (mComposing.length() > 0) {
          commitTyped(getCurrentInputConnection());
        }
        sendKey(primaryCode);
      } else if (primaryCode == Keyboard.KEYCODE_DELETE) {
        handleBackspace();
      } else if (primaryCode == Keyboard.KEYCODE_SHIFT) {
        handleShift();
      } else if (primaryCode == Keyboard.KEYCODE_CANCEL) {
        handleClose();
        return;
      } else {
        handleCharacter(primaryCode);
        if (mCompletionOn) {
          setSuggestions(Arrays.asList(mComposing.toString(), "とうきょう"), true, true);
        }
      }
    }
  }

  @Override
  public void onText(CharSequence text) {
    InputConnection ic = getCurrentInputConnection();
    if (ic == null) {
      return;
    }
    ic.beginBatchEdit();
    if (mComposing.length() > 0) {
      commitTyped(ic);
    }
    ic.commitText(text, 0);
    ic.endBatchEdit();
  }

  /**
   * Updates the list of available candidates from the current composing text. This will need to be
   * filled in by however you are determining candidates.
   */
  private void updateCandidates() {
    if (!mCompletionOn) {
      if (mComposing.length() > 0) {
        ArrayList<String> list = new ArrayList<String>();
        list.add(mComposing.toString());
        setSuggestions(list, true, true);
      } else {
        setSuggestions(null, false, false);
      }
    }
  }

  @Override
  public void onComputeInsets(Insets outInsets) {
    super.onComputeInsets(outInsets);
  }

  public void setSuggestions(List<String> suggestions, boolean completions,
      boolean typedWordValid) {
    if (suggestions != null && suggestions.size() > 0) {
      setCandidatesViewShown(true);
      mCompletions = new CompletionInfo[suggestions.size()];
      int position = 0;
      for (String suggestion : suggestions) {
        mCompletions[position] = new CompletionInfo(position, position, suggestion);
        position++;
      }
    } else if (isExtractViewShown()) {
      setCandidatesViewShown(true);
    }
    if (mCandidateView != null) {
      mCandidateView.setSuggestions(suggestions, completions, typedWordValid);
      mCandidateView.setVisibility(View.VISIBLE);
    }
  }

  private void handleBackspace() {
    final int length = mComposing.length();
    if (length > 1) {
      mComposing.delete(length - 1, length);
      getCurrentInputConnection().setComposingText(mComposing, 1);
      updateCandidates();
    } else if (length > 0) {
      mComposing.setLength(0);
      getCurrentInputConnection().commitText("", 0);
      updateCandidates();
    } else {
      keyDownUp(KeyEvent.KEYCODE_DEL);
    }
  }

  private void handleShift() {
    if (mInputView == null) {
      return;
    }

    Keyboard currentKeyboard = mInputView.getKeyboard();
    if (mKeyboard == currentKeyboard) {
      checkToggleCapsLock();
      mInputView.setShifted(mCapsLock || !mInputView.isShifted());
    }
  }

  private void handleCharacter(int primaryCode) {
    if (isInputViewShown()) {
      if (mInputView.isShifted()) {
        primaryCode = Character.toUpperCase(primaryCode);
      }
    }
    if (isAlphabet(primaryCode) && mCompletionOn) {
      mComposing.append((char) primaryCode);
      getCurrentInputConnection().setComposingText(mComposing, 1);
      updateCandidates();
    } else {
      getCurrentInputConnection().commitText(String.valueOf((char) primaryCode), 1);
    }
  }

  private void handleClose() {
    commitTyped(getCurrentInputConnection());
    requestHideSelf(0);
    mInputView.closing();
  }

  private void checkToggleCapsLock() {
    long now = System.currentTimeMillis();
    if (mLastShiftTime + 800 > now) {
      mCapsLock = !mCapsLock;
      mLastShiftTime = 0;
    } else {
      mLastShiftTime = now;
    }
  }

  /**
   * Shows message dialog with given content.
   *
   * @param title of the message dialog
   * @param message content of the message dialog
   */
  private void showMessageDialog(String title, final String message) {
    AlertDialog dialog = new AlertDialog.Builder(SoftKeyboard.this.getBaseContext()).setTitle(title)
        .setMessage(Html.fromHtml(message)).setPositiveButton("ok", null).create();
    Window window = dialog.getWindow();
    WindowManager.LayoutParams lp = window.getAttributes();
    lp.token = mInputView.getWindowToken();
    lp.type = WindowManager.LayoutParams.TYPE_APPLICATION_ATTACHED_DIALOG;
    window.setAttributes(lp);
    window.addFlags(WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM);
    dialog.show();

    // Makes the link in the text clickable.
    try {
      TextView textView = (TextView) dialog.getWindow().findViewById(android.R.id.message);
      if (textView != null) {
        // I should override the onTouchEvent on the LinkMovementMethod because it is required to
        // replace default ClickableSpan with slightly changed version.
        textView.setMovementMethod(new LinkMovementMethod() {

          @Override
          public boolean onTouchEvent(TextView widget, Spannable buffer, MotionEvent event) {
            SpannableStringBuilder builder = new SpannableStringBuilder(message);
            int x = (int) event.getX();
            int y = (int) event.getY();

            x -= widget.getTotalPaddingLeft();
            y -= widget.getTotalPaddingTop();

            x += widget.getScrollX();
            y += widget.getScrollY();

            Layout layout = widget.getLayout();
            int line = layout.getLineForVertical(y);
            int off = layout.getOffsetForHorizontal(line, x);

            URLSpan[] link = buffer.getSpans(off, off, URLSpan.class);
            if (link != null & link.length != 0) {
              final URLSpan span = link[0];
              // This wrapper adds the flag FLAG_ACTIVITY_NEW_TASK to the intent, because it is
              // required if the intent is calling from non activity context.
              URLSpan spanWrapper = new URLSpan(span.getURL()) {
                @Override
                public void onClick(View widget) {
                  Uri uri = Uri.parse(getURL());
                  Context context = widget.getContext();
                  Intent intent = new Intent(Intent.ACTION_VIEW, uri);
                  intent.putExtra(Browser.EXTRA_APPLICATION_ID, context.getPackageName());
                  intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                  context.startActivity(intent);
                }
              };
              builder.setSpan(spanWrapper, off, off, Spanned.SPAN_USER);

              return super.onTouchEvent(widget, builder, event);
            } else {
              return super.onTouchEvent(widget, buffer, event);
            }
          }

        });
      }
    } catch (Exception e) {
      // do nothing.
    }
  }

  private String getWordSeparators() {
    return mWordSeparators;
  }

  public boolean isWordSeparator(int code) {
    String separators = getWordSeparators();
    return separators.contains(String.valueOf((char) code));
  }

  public void pickDefaultCandidate() {
    pickSuggestionManually(0);
  }

  /**
   * Picks the suggestion specified by means of the given {@code index}.
   *
   * @param index of the suggetsion
   */
  public void pickSuggestionManually(int index) {
    if (mCompletionOn && mCompletions != null && index >= 0 && index < mCompletions.length) {
      CompletionInfo ci = mCompletions[index];
      getCurrentInputConnection().commitText(ci.getText(), 1);
      this.mComposing.delete(0, this.mComposing.length());
      if (mCandidateView != null) {
        mCandidateView.clear();
        mCandidateView.setVisibility(View.INVISIBLE);
      }
    } else if (mComposing.length() > 0) {
      commitTyped(getCurrentInputConnection());
    }
  }

  @Override
  public void swipeRight() {
    if (mCompletionOn) {
      pickDefaultCandidate();
    }
  }

  @Override
  public void swipeLeft() {
    handleBackspace();
  }

  @Override
  public void swipeDown() {
    handleClose();
  }

  @Override
  public void swipeUp() {}

  @Override
  public void onPress(int primaryCode) {}

  @Override
  public void onRelease(int primaryCode) {}

  public StringBuilder getmComposing() {
    return mComposing;
  }

  public MockImeKeyboardView getmInputView() {
    return mInputView;
  }

  public boolean ismCompletionOn() {
    return mCompletionOn;
  }

  public void setmCompletionOn(boolean mCompletionOn) {
    this.mCompletionOn = mCompletionOn;
  }


}
