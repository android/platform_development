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
import android.util.Log;

/**
 * Shoots photos using back and front cameras alternatively.
 */
public class CameraFrontBack extends Camera {

    private UiObject mToggleButton;
    private UiObject mModeOptionToggle;

    public CameraFrontBack() {
        super("camera_shoot_front_back");
    }

    @Override
    protected void retrieveCameraUiObjects() throws UiObjectNotFoundException {
        super.retrieveCameraUiObjects();
        mToggleButton = find(byPackageResource("camera_toggle_button"));
        mModeOptionToggle = find(byPackageResource("mode_options_toggle"));
    }

    protected void toggleCamera() throws UiObjectNotFoundException, InterruptedException {
        if (!mToggleButton.exists()) {
            mModeOptionToggle.click();
        }
        mToggleButton.click();
    }

    @Override
    public void testSteps() throws UiObjectNotFoundException, InterruptedException {
        int numShot = getNumShot();
        int shootInterval = getShootInterval();

        retrieveCameraUiObjects();
        for (int i = 0; i < numShot; ++i) {
            if (i > 0) {
                sleep(shootInterval);
            }
            shotPhoto();
            Log.i(LOG_TAG, String.format("shoot %d / %d photos", i + 1, numShot));
            toggleCamera();
            Log.i(LOG_TAG, "toggle camera");
        }
    }
}
