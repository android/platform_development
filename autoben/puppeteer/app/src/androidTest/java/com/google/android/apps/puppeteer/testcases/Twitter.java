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
 * Launches Twitter and scrolls it.
 */
public class Twitter extends UiTestCase {
    private static final int MAX_SWIPES = 5;
    private static final int SCROLL_STEPS = 15;

    public Twitter() {
        super("com.twitter.android", "twitter");
    }

    @Override
    public void testSteps() throws UiObjectNotFoundException, InterruptedException {
        scrollDownUp("android:id/list", MAX_SWIPES, SCROLL_STEPS);
    }
}
