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
 * Basic Chrome browser test.
 *
 * Launches Chrome, navigates to a website (amazon.com) and then scrolls down/up twice.
 * Measures memory and FPS.
 */
public class Browser extends UiTestCase {

    private static final String TEXT_TO_OMNI_BAR = "amazon.com";

    private static final int WAIT_OMNI_BAR = 1000;
    private static final int WAIT_PAGE_LOAD = 5000;
    private static final int SCROLL_REPEAT = 2;
    private static final int MAX_SWIPES = 5;
    private static final int SCROLL_STEPS = 15;

    public Browser() {
        super("com.android.chrome", "browser");
    }

    private void navigateToOmniBarFirstResult(String omniBarText) throws UiObjectNotFoundException,
            InterruptedException {
        // Input text into OmniBar.
        find(byPackageResource("search_box")).click();
        find(byPackageResource("url_bar")).setText(omniBarText);
        sleep(WAIT_OMNI_BAR);

        // Navigate to the first OmniBar result.
        find(byPackageResource("omnibox_results_container"))
                .getChild(byIndex(0)).getChild(byIndex(0)).click();
        sleep(WAIT_PAGE_LOAD);
    }

    @Override
    public void testSteps() throws UiObjectNotFoundException, InterruptedException {
        navigateToOmniBarFirstResult(TEXT_TO_OMNI_BAR);
        scrollDownUp(getPackageResourceId("compositor_view_holder"),
                MAX_SWIPES, SCROLL_STEPS, SCROLL_REPEAT);
    }
}
