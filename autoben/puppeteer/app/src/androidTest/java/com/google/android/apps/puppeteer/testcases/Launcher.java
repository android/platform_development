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

import android.support.test.uiautomator.UiObject;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.support.test.uiautomator.UiScrollable;
import android.widget.TextView;

import com.google.android.apps.puppeteer.UiTestCase;


/**
 * Scrolls launcher and browse apps.
 */
public class Launcher extends UiTestCase {

    private static final int SWIPE_STEPS = 10;
    private static final int REPEAT = 3;
    private static final int MAX_SCROLL = 50;
    private static final int SCROLL_STEPS = 15;

    public Launcher() {
        super("com.google.android.googlequicksearchbox", "launcher");
    }

    public Launcher(String metricFolderName) {
        super("com.google.android.googlequicksearchbox", metricFolderName);
    }

    @Override
    public void setUp() {
        setUpMetricThread();
    }

    @Override
    public void tearDown() throws UiObjectNotFoundException, InterruptedException {
        stopMetricThread();
        mDevice.pressHome();
    }

    protected UiObject getAppMenuButton() throws UiObjectNotFoundException {
        return new UiScrollable(byPackageResource("hotseat"))
                .getChildByDescription(byClass(TextView.class.getName()), "Apps");
    }

    @Override
    public void testSteps() throws UiObjectNotFoundException, InterruptedException {
        // Scroll launcher.
        UiObject launcher = find(byPackageResource("workspace"));
        annotate("launcher swipe right");
        launcher.swipeRight(SWIPE_STEPS);
        annotate("launcher swipe left");
        launcher.swipeLeft(SWIPE_STEPS);
        annotate("launcher swipe left");
        launcher.swipeLeft(SWIPE_STEPS);
        annotate("launcher swipe right");
        launcher.swipeRight(SWIPE_STEPS);

        // Go to apps menu and out three times.
        UiObject app = getAppMenuButton();
        for (int i = 0; i < REPEAT; i++) {
            annotate("click app menu button");
            app.click();
            if (i < REPEAT - 1) {
                annotate("click home");
                mDevice.pressHome();
            }
        }
        // Swipe apps menu.
        for (int i = 0; i < REPEAT; ++i) {
            annotate("display swipe left");
            displaySwipeLeft();
            annotate("display swipe right");
            displaySwipeRight();
        }
        annotate("done");

    }
}
