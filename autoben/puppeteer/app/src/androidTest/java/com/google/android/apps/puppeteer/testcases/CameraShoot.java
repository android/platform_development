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
import android.util.Log;

/**
 * Launches Google Camera and takes ten shots without pause.
 */
public class CameraShoot extends Camera {

    public CameraShoot() {
        super("camera_shoot");
    }

    @Override
    public void testSteps() throws UiObjectNotFoundException, InterruptedException {
        int numShot = getNumShot();

        retrieveCameraUiObjects();
        for (int i = 0; i < numShot; ++i) {
            shotPhoto();
            Log.i(LOG_TAG, String.format("shoot %d / %d photos", i + 1, numShot));
        }
    }
}
