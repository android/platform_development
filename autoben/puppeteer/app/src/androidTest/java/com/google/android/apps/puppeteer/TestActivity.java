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
package com.google.android.apps.puppeteer;

import android.app.UiAutomation;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.os.Bundle;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.support.test.uiautomator.Until;
import android.util.Log;

import com.google.android.apps.puppeteer.testcases.*;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.ArrayList;
import java.util.List;

import static android.support.test.InstrumentationRegistry.getInstrumentation;
import static org.hamcrest.CoreMatchers.notNullValue;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.assertTrue;

@RunWith(AndroidJUnit4.class)
public class TestActivity {
    private static final int LAUNCH_TIMEOUT = 10000;
    private static final int NUM_ITERATIONS = 1;
    private static final String ALL_TEST_CASES = "launcher,launcher_app,browser,camera," +
            "camera_shoot,camera_front_back,facebook_post,facebook_comment,facebook_msg," +
            "facebook_swipe,gmail,google_maps,instagram,youtube,3d";
    private UiDevice mDevice;
    private UiAutomation mUiAuto;
    private List<UiTestCase> mTestCases = new ArrayList<>();
    private Bundle mArguments;

    @Before
    public void start() {
        mDevice = UiDevice.getInstance(getInstrumentation());
        mUiAuto = getInstrumentation().getUiAutomation();
        mArguments = InstrumentationRegistry.getArguments();

        final String launcherPackage = getLauncherPackageName();
        Log.e("test", launcherPackage);
        assertThat(launcherPackage, notNullValue());
        mDevice.wait(Until.hasObject(By.pkg(launcherPackage).depth(0)), LAUNCH_TIMEOUT);
    }

    @Test
    public void runTestCases() throws UiObjectNotFoundException, InterruptedException {
        // Arg "test" is a comma-separated string of test list.
        String[] testNames = mArguments.getString("test", ALL_TEST_CASES).split(",");
        for (String testName : testNames) {
            assertTrue(String.format("test %s does not exist", testName), addTestCase(testName));
        }

        for (int i = 0; i < NUM_ITERATIONS; i++) {
            for (UiTestCase testCase : mTestCases) {
                testCase.runTest();
            }
        }
    }

    private boolean addTestCase(String testName) {
        if (testName.equals("launcher")) {
            addTestCase(new Launcher());
        } else if (testName.equals("launcher_app")) {
            addTestCase(new LauncherApp());
        } else if (testName.equals("browser")) {
            addTestCase(new Browser());
        } else if (testName.equals("camera")) {
            addTestCase(new Camera());
        } else if (testName.equals("camera_shoot")) {
            addTestCase(new CameraShoot());
        } else if (testName.equals("camera_front_back")) {
            addTestCase(new CameraFrontBack());
        } else if (testName.equals("facebook_post")) {
            addTestCase(new FacebookPost());
        } else if (testName.equals("facebook_comment")) {
            addTestCase(new FacebookComment());
        } else if (testName.equals("facebook_msg")) {
            addTestCase(new FacebookMessenger());
        } else if (testName.equals("facebook_swipe")) {
            addTestCase(new FacebookSwipe());
        } else if (testName.equals("gmail")) {
            addTestCase(new Gmail());
        } else if (testName.equals("google_maps")) {
            addTestCase(new GoogleMaps());
        } else if (testName.equals("instagram")) {
            addTestCase(new Instagram());
        } else if (testName.equals("youtube")) {
            addTestCase(new YouTube());
        } else if (testName.equals("3d")) {
            addTestCase(new ThreeD());
        } else {
            return false;
        }
        return true;
    }

    private void addTestCase(UiTestCase testCase) {
        testCase.init(mDevice, mUiAuto, mArguments);
        mTestCases.add(testCase);
    }

    private String getLauncherPackageName() {
        // Create launcher Intent
        final Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.addCategory(Intent.CATEGORY_HOME);

        // Use PackageManager to get the launcher package name
        PackageManager pm = InstrumentationRegistry.getContext().getPackageManager();
        ResolveInfo resolveInfo = pm.resolveActivity(intent, PackageManager.MATCH_DEFAULT_ONLY);
        return resolveInfo.activityInfo.packageName;
    }

}
