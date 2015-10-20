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
 * [Work in progress] Replay game: BrainDots.
 */
public class GameBrainDots extends UiTestCase {

    public GameBrainDots() {
        super("jp.co.translimit.braindots", "brain_dots");
    }

    @Override
    public void testSteps() throws UiObjectNotFoundException, InterruptedException {
        /*** brain dots stage 1-6 ***/
        replayEvent("dots1-6.revent");
        sleep(3000);
    }
}
