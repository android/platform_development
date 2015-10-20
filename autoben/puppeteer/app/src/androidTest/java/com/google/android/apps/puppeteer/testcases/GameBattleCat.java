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
 * [Work in progress] Replay game: BattleCat.
 */
public class GameBattleCat extends UiTestCase {
    private static final int WAIT_APP_LAUNCH = 10000;
    private static final int PAUSE = 3000;
    private static final String[] REVENT_FILES = {"catst_s.revent", "cat1_s.revent",
            "cat2_s.revent"};

    public GameBattleCat() {
        super("jp.co.ponos.battlecatstw", "battle_cat");
    }

    @Override
    public void testSteps() throws UiObjectNotFoundException, InterruptedException {
        sleep(WAIT_APP_LAUNCH);

        for (String reventFile: REVENT_FILES) {
            replayEvent(reventFile);
            sleep(PAUSE);
        }
    }
}
