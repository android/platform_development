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

public class FacebookMessenger extends UiTestCase {
    private static final String TEXT_TO_SEND = "test hi";
    private static final int ACTION_WAIT = 1000;

    public FacebookMessenger() {
        super("com.facebook.orca", "facebook_messenger");
    }

    @Override
    public void testSteps() throws UiObjectNotFoundException, InterruptedException {
        // Send message.
        find(byPackageResource("thread_list")).getChild(byIndex(0)).click();
        find(byPackageResource("edit_text")).setText(TEXT_TO_SEND);
        find(byPackageResource("send_button")).click();
        sleep(ACTION_WAIT);

        // Send icon.
        find(byPackageResource("stickers_button")).click();
        waitForResource("tab_container", ACTION_WAIT);

        find(byPackageResource("tab_container")).getChild(byIndex(4)).click();
        find(byPackageResource("sticker_grid")).getChild(byIndex(10)).click();
        sleep(ACTION_WAIT);

    }
}
