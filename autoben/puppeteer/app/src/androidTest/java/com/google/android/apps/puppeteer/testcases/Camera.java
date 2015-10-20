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

import com.google.android.apps.puppeteer.UiTestCase;

/**
 * Basic Google Camera test.
 * Launches Google Camera and takes two photos then browse them.
 */
public class Camera extends UiTestCase {
    private static final String ARG_NUM_SHOT = "num_shot";
    private static final String ARG_SHOOT_INTERVAL = "shoot_interval";

    protected static final String LOG_TAG = "Camera";

    protected static final int WAIT_FOR_FILMSTRIP = 3000;

    private UiObject mModeOptionsOverlay;
    private UiObject mShutterButton;

    public Camera() {
        super("com.google.android.GoogleCamera", "camera");
    }

    public Camera(String metricFolderName) {
        super("com.google.android.GoogleCamera", metricFolderName);
    }

    @Override
    protected String getPackageResourceId(String resourceId) {
        return "com.android.camera2:id/" + resourceId;
    }

    protected void retrieveCameraUiObjects() throws UiObjectNotFoundException {
        mModeOptionsOverlay = find(byPackageResource("mode_options_overlay"));
        mShutterButton = find(byPackageResource("shutter_button"));
    }

    protected void shotPhoto() throws UiObjectNotFoundException, InterruptedException {
        annotate("shot photo");
        mModeOptionsOverlay.click();
        mShutterButton.click();
    }

    protected void browsePhotos() throws UiObjectNotFoundException, InterruptedException {
        mModeOptionsOverlay.swipeLeft(30);
        Log.i(LOG_TAG, "browse photos");
        annotate("browse photo");
        find(byClass("com.android.camera.widget.FilmstripView")).swipeLeft(30);
        sleep(WAIT_FOR_FILMSTRIP);
    }

    /**
     * Retrieves "num_shot" from instrument argument.
     * @return number of shoots.
     */
    protected final int getNumShot() {
        return Integer.valueOf(mArguments.getString(ARG_NUM_SHOT, "2"));
    }

    /**
     * Retrieves "shoot_interval" (in milliseconds) from instrument argument.
     * @return shoot interval in milliseconds.
     */
    protected final int getShootInterval() {
        return Integer.valueOf(mArguments.getString(ARG_SHOOT_INTERVAL, "2000"));
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
        }
        browsePhotos();
    }
}
