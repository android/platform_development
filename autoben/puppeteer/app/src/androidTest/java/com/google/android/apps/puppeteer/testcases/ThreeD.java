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
 * [Work in progress] Runs EpicCitadel for 3D benchmarking.
 */
public class ThreeD extends UiTestCase {

    public ThreeD() {
        super("com.epicgames.EpicCitadel", "three_d");
    }

    @Override
    public void testSteps() throws UiObjectNotFoundException, InterruptedException {
        sleep(15000);
        replayEvent("3d_s.revent");
        /*** benchmarking for 30s ***/
        sleep(30000);
    }
}
