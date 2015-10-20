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
import android.support.test.uiautomator.UiScrollable;

import com.google.android.apps.puppeteer.UiTestCase;

/**
 * Launches Facebook and picks top post to like and comment.
 */
public class FacebookComment extends UiTestCase {

    private static final String TEXT_TO_SEND = "comment test";
    private static final int ACTION_WAIT = 1000;

    public FacebookComment() {
        super("com.facebook.katana", "facebook_comment");
    }

    @Override
    public void testSteps() throws UiObjectNotFoundException, InterruptedException {
        find(byPackageResource("bookmarks_tab")).click();
        find(byPackageResource("bookmarks_list")).getChild(byIndex(0)).click();
        UiScrollable list = new UiScrollable(byResource("android:id/list"));
        list.scrollIntoView(byPackageResource("feed_feedback_like_container"));

        // Click Like.
        find(byPackageResource("feed_feedback_like_container")).click();
        sleep(ACTION_WAIT);

        // Send comment.
        find(byPackageResource("feed_feedback_comment_container")).click();
        find(byPackageResource("comment_edit_text")).setText(TEXT_TO_SEND);
        find(byPackageResource("comment_post_button")).click();
        sleep(ACTION_WAIT);
    }
}
