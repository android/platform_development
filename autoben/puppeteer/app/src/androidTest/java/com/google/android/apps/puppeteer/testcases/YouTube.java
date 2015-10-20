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

import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.UiObjectNotFoundException;

import com.google.android.apps.puppeteer.UiTestCase;

/**
 * Launches YouTube , searches video and plays for 30 seconds.
 */
public class YouTube extends UiTestCase {
    private static final int WAIT_SEARCH_BUTTON = 2000;
    private static final int PLAY_TIME = 30000;
    private static final String QUERY_STRING = "charlie the unicorn";

    public YouTube() {
        super("com.google.android.youtube", "youtube");
    }

    @Override
    public void testSteps() throws UiObjectNotFoundException, InterruptedException {
        // Search video.
        annotate("search video");
        UiObject2 searchButton = waitForResource("menu_search", WAIT_SEARCH_BUTTON);
        searchButton.click();
        find(byPackageResource("search_edit_text")).setText(QUERY_STRING);
        find(byClass("android.widget.ListView").resourceId(getPackageResourceId("results")))
                .getChild(byIndex(0))
                .click();

        // Find video and wait/play for 30s.
        annotate("video playing");
        find(byPackageResource("results")).getChild(byIndex(4)).click();
        sleep(PLAY_TIME);
    }
}
