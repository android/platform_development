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

import android.support.test.uiautomator.UiObject;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.widget.EditText;
import android.widget.RelativeLayout;

import com.google.android.apps.puppeteer.UiTestCase;

public class GoogleMaps extends UiTestCase {

    public GoogleMaps() {
        super("com.google.android.apps.maps", "google_maps");
    }

    @Override
    protected String getPackageResourceId(String resourceId) {
        return "com.google.android.apps.gmm:id/" + resourceId;
    }

    @Override
    public void testSteps() throws UiObjectNotFoundException, InterruptedException {
        // Search location.
        find(byClass(EditText.class.getName())).click();
        find(byPackageResource("search_omnibox_edit_text")).setText("taipei 101");
        find(byClass(RelativeLayout.class.getName())).click();

        // Pinch in then out.
        UiObject map = find(byPackageResource("slidingpane_container"));
        for (int i = 0; i < 4; ++i)
            map.pinchIn(50, 50);
        for (int i = 0; i < 4; ++i)
            map.pinchOut(50, 50);

        sleep(1500);
    }
}
