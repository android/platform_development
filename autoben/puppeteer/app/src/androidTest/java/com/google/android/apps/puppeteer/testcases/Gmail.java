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
 * Launches Gmail, sends a mail and scrolls list of mails.
 */
public class Gmail extends UiTestCase {
    private static final int MAX_SWIPES = 10;
    private static final int SCROLL_STEPS = 20;

    public Gmail() {
        super("com.google.android.gm", "gmail");
    }

    @Override
    public void testSteps() throws UiObjectNotFoundException, InterruptedException {
        // Send mail.
        find(byPackageResource("compose_button")).click();
        find(byPackageResource("to")).setText("crostestacct@gmail.com");
        find(byPackageResource("subject")).setText("This is a test!");
        find(byPackageResource("body")).setText(
                "123456789876543212345678987654321\nhahaha blahblahblah\nabcdefg\n");
        find(byPackageResource("send")).click();
        sleep(5000);

        // Scroll Gmail conversations.
        scrollDownUp(
                byClass("android.widget.ListView").resourceId(
                        getPackageResourceId("conversation_list_view")),
                MAX_SWIPES, SCROLL_STEPS);
        sleep(1000);

        // Click sidebar.
        find(byPackageResource("mail_toolbar")).getChild(byIndex(0)).click();
        sleep(2000);
        mDevice.pressBack();
        sleep(1000);
    }
}