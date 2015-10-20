/*
 * Copyright (C) 2015 The Android Open Source Project
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
package com.google.android.apps.puppeteer.testcases;

import android.support.test.uiautomator.UiObjectNotFoundException;

import com.google.android.apps.puppeteer.UiTestCase;

/**
 * Launches Facebook and swipes among its five main function pages.
 */
public class FacebookSwipe extends UiTestCase {

    private static final int WAIT_BEFORE_SWIPE = 3000;
    private static final int REPEAT = 10;
    private static final int SWIPE_DEPTH = 4;
    private static final int SWIPE_PAUSE = 200;
    private static final int SWIPE_LONG_PAUSE = 500;


    public FacebookSwipe() {
        super("com.facebook.katana", "facebook_swipe");
    }

    @Override
    public void testSteps() throws UiObjectNotFoundException, InterruptedException {
        sleep(WAIT_BEFORE_SWIPE);
        annotate("start swipe");
        for (int i = 0; i < REPEAT; ++i) {
            // Switch between Facebook's five main page.
            for (int j = 0; j < SWIPE_DEPTH; ++j) {
                displaySwipeLeft();
                sleep(SWIPE_PAUSE);
            }
            sleep(SWIPE_LONG_PAUSE);
            for (int j = 0; j < SWIPE_DEPTH; ++j) {
                displaySwipeRight();
                sleep(SWIPE_PAUSE);
            }
            sleep(SWIPE_LONG_PAUSE);
        }
    }
}
