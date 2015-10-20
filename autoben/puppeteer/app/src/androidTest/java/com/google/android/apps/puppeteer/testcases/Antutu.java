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
 * Runs Antutu benchmark app for specific time.
 */
public class Antutu extends UiTestCase {
    private static final int TEST_DURATION = 60000;
    private static final int PACKAGE_LAUNCH_WAIT = 3000;

    public Antutu() {
        super("com.antutu.ABenchMark", "antutu");
    }

    @Override
    public void testSteps() throws UiObjectNotFoundException, InterruptedException {
        waitForResource("start_test_region", PACKAGE_LAUNCH_WAIT).click();
        find(byPackageResource("retest_btn")).click();
        // Wait for test end.
        waitForResource("start_test_region", TEST_DURATION).click();
    }
}
