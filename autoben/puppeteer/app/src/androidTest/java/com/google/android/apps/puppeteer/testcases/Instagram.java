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
 * Launches Instagram and upload a photo.
 */
public class Instagram extends UiTestCase {

    private static final int ACTION_WAIT = 2000;
    private static final int AFTER_SCROLL_WAIT = 1000;
    private static final int MAX_SCROLL = 50;
    private static final int SCROLL_STEPS = 15;

    public Instagram() {
        super("com.instagram.android", "instagram");
    }

    @Override
    public void testSteps() throws UiObjectNotFoundException, InterruptedException {
        // Upload photo.
        find(byResource("android:id/tabs")).getChild(byIndex(2)).click();
        find(byPackageResource("media_tab_bar")).getChild(byIndex(0)).click();
        find(byPackageResource("media_picker_grid_view")).getChild(byIndex(3)).click();
        find(byPackageResource("next")).click();
        sleep(ACTION_WAIT);
        find(byPackageResource("button_next")).click();
        sleep(ACTION_WAIT);

        scrollDownUp(byClass("android.widget.ListView"), MAX_SCROLL,
                SCROLL_STEPS);
        sleep(AFTER_SCROLL_WAIT);
    }
}
