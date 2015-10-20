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

import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.support.test.uiautomator.UiScrollable;
import android.support.test.uiautomator.UiSelector;

import com.google.android.apps.puppeteer.UiTestCase;

/**
 * Launches Facebook and posts a status, scrolls main page and friend list.
 */
public class FacebookPost extends UiTestCase {

    private static final int ACTION_WAIT = 1000;
    private static final int POST_MAX_SWIPE = 5;
    private static final int POST_SWIPE_STEPS = 20;
    private static final int FRIEND_MAX_SWIPE = 3;
    private static final int FRIEND_SWIPE_STEPS = 15;


    public FacebookPost() {
        super("com.facebook.katana", "facebook_post");
    }

    @Override
    public void testSteps() throws UiObjectNotFoundException, InterruptedException {
        // Post article.
        find(byPackageResource("feed_composer_hint")).click();
        find(byPackageResource("sprout_fab_list")).getChild(byIndex(0)).click();
        String postStatus = "test auto UI " + Double.toString(Math.random());
        find(byPackageResource("status_text")).setText(postStatus);

        // Wait for post complete.
        mDevice.findObject(By.res(mPackageName, "composer_primary_named_button")).click();
        sleep(ACTION_WAIT);

        // Scroll main page.
        scrollDownUp("android:id/list", POST_MAX_SWIPE, POST_SWIPE_STEPS);
        sleep(ACTION_WAIT * 2);

        // Click news feed.
        find(byPackageResource("news_feed_tab")).click();
        sleep(ACTION_WAIT * 2);

        // Scroll find friend.
        find(byPackageResource("friend_requests_tab")).click();
        mDevice.findObject(new UiSelector().text("FIND FRIENDS")).click();
        UiScrollable friendFrame = new UiScrollable(byClass("android.widget.ListView"));
        friendFrame.scrollToEnd(FRIEND_MAX_SWIPE, FRIEND_SWIPE_STEPS);
        sleep(ACTION_WAIT);
        friendFrame.scrollToBeginning(FRIEND_MAX_SWIPE, FRIEND_SWIPE_STEPS);
        sleep(ACTION_WAIT);
    }
}
