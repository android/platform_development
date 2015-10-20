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
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.support.test.InstrumentationRegistry;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.support.test.uiautomator.UiScrollable;
import android.support.test.uiautomator.UiSelector;
import android.support.test.uiautomator.Until;

import com.google.android.apps.puppeteer.metrics.BatteryMetric;
import com.google.android.apps.puppeteer.metrics.CPUFreqMetric;
import com.google.android.apps.puppeteer.metrics.CPUIdleStateMetric;
import com.google.android.apps.puppeteer.metrics.GFXMetric;
import com.google.android.apps.puppeteer.metrics.MemoryMetric;

/**
 * Base class to test Android app using UI Automation.
 */
public abstract class UiTestCase {

    private static final int LAUNCH_TIMEOUT = 10000;
    private static final int METRIC_MEASURE_INTERVAL = 500;
    private static final int SWIPE_STEPS = 10;
    private static final String REVENT_BIN = "/data/local/revent";
    private static final String EVENT_BASEPATH = "/data/local";

    protected UiDevice mDevice;
    protected UiAutomation mUiAuto;

    // Stores arguments passed by -e.
    protected Bundle mArguments;

    protected MetricRecorder mMetricRecorder;
    protected RecordLogger mAnnotator;

    protected final String mPackageName;
    protected final String mMetricFolderName;

    private int displayHeight, displayWidth;

    /**
     * Constructor.
     * Subclasses should provide package name and folder name to store metrics.
     * @param packageName app package name
     * @param metricFolderName folder name to store metrics
     */
    public UiTestCase(String packageName, String metricFolderName) {
        mPackageName = packageName;
        mMetricFolderName = metricFolderName;
    }

    /**
     * Inits UiDevice and UiAutomation object.
     * @param device an UiDevice object
     * @param uiAuto an UiAutomation object
     */
    public final void init(UiDevice device, UiAutomation uiAuto, Bundle arguments) {
        mDevice = device;
        mUiAuto = uiAuto;
        mArguments = arguments;
    }

    /**
     * Runs the test.
     * @throws UiObjectNotFoundException
     * @throws InterruptedException
     */
    public final void runTest() throws UiObjectNotFoundException, InterruptedException {
        setUp();
        testSteps();
        tearDown();
    }

    /**
     * Steps interacting with UI.
     */
    protected abstract void testSteps() throws UiObjectNotFoundException, InterruptedException;

    /**
     * Sets up environment to test.
     * It launches the target app (specified by {#mPackageName}).
     */
    protected void setUp() {
        mDevice.pressHome();

        // Launch package.
        Context context = InstrumentationRegistry.getContext();
        final Intent intent = context.getPackageManager().getLaunchIntentForPackage(mPackageName);
        context.startActivity(intent);
        mDevice.wait(Until.hasObject(By.pkg(mPackageName).depth(0)), LAUNCH_TIMEOUT);

        setUpMetricThread();
    }

    /**
     * Backs to home screen and terminates the target app.
     * @throws UiObjectNotFoundException
     */
    protected void tearDown() throws UiObjectNotFoundException, InterruptedException {
        stopMetricThread();
        mDevice.pressBack();
        mDevice.pressBack();
        mDevice.pressBack();
        mDevice.pressHome();
        mUiAuto.executeShellCommand("am force-stop " + mPackageName);
    }

    /**
     * Initializes metric recorder thread to measure metrics periodically.
     */
    protected final void setUpMetricThread() {
        mMetricRecorder = new MetricRecorder(mUiAuto, METRIC_MEASURE_INTERVAL, mPackageName,
                mMetricFolderName);
        addMetrics();
        mMetricRecorder.start();

        mAnnotator = new RecordLogger("annotation");
        mAnnotator.setBaseDir(mMetricRecorder.getWorkingDir());
    }

    /**
     * Adds metrics into measuement thread.
     * Subclasses may override it if they need metrics other than MemoryMetric and GFXMetric.
     */
    protected void addMetrics() {
        mMetricRecorder.addMetric(new BatteryMetric());
        mMetricRecorder.addMetric(new CPUFreqMetric());
        mMetricRecorder.addMetric(new CPUIdleStateMetric());
        mMetricRecorder.addMetric(new GFXMetric());
        mMetricRecorder.addMetric(new MemoryMetric());
    }

    /**
     * Stops metric thread if presents.
     * @throws InterruptedException
     */
    protected void stopMetricThread() throws InterruptedException {
        if (mMetricRecorder != null) {
            mMetricRecorder.stopRunning();
            mMetricRecorder.join();
        }
        if (mAnnotator != null) {
            mAnnotator.saveToFile();
        }
    }

    /**
     * Writes an annotation.
     * @param note annotation note.
     */
    protected void annotate(String note) {
        if (mAnnotator != null) {
            mAnnotator.addRecord(note, "note");
        }
    }

    /**
     * Gets resource ID of the package.
     * Subclasses may override it if its package ID's prefix is not "package_name:id/".
     * @param resourceId resource ID to be attached after "package_name:id/".
     * @return package id prefix
     */
    protected String getPackageResourceId(String resourceId) {
        return mPackageName + ":id/" + resourceId;
    }

    protected UiObject find(UiSelector uiSelector) throws UiObjectNotFoundException {
        return mDevice.findObject(uiSelector);
    }

    protected UiSelector byPackageResource(String packageResourcePostfix) throws
            UiObjectNotFoundException {
        return byResource(getPackageResourceId(packageResourcePostfix));
    }

    protected UiSelector byResource(String resourceId) throws UiObjectNotFoundException {
        return new UiSelector().resourceId(resourceId);
    }

    protected UiObject2 waitForResource(String resourceId, int waitDurationMs) {
        return mDevice.wait(
                Until.findObject(By.res(mPackageName, resourceId)),
                waitDurationMs);
    }

    protected UiSelector byClass(String className) throws UiObjectNotFoundException {
        return new UiSelector().className(className);
    }

    protected UiSelector byIndex(int index) throws UiObjectNotFoundException {
        return new UiSelector().index(index);
    }

    protected boolean scrollDownUp(UiSelector scrollableSelector, int maxSwipes, int steps,
                                   int repeat)
            throws UiObjectNotFoundException {
        boolean result = true;
        UiScrollable scroll = new UiScrollable(scrollableSelector);
        for (int i = 0; i < repeat; ++i) {
            result = result && scroll.scrollToEnd(maxSwipes, steps);
            result = result && scroll.scrollToBeginning(maxSwipes, steps);
        }
        return result;
    }

    protected boolean scrollDownUp(UiSelector scrollableSelector, int maxSwipes, int steps)
            throws UiObjectNotFoundException {
        return scrollDownUp(scrollableSelector, maxSwipes, steps, 1);
    }

    protected boolean scrollDownUp(String resourceId, int maxSwipes, int steps, int repeat)
            throws UiObjectNotFoundException {
        return scrollDownUp(new UiSelector().resourceId(resourceId), maxSwipes, steps, repeat);
    }

    protected boolean scrollDownUp(String resourceId, int maxSwipes, int steps)
            throws UiObjectNotFoundException {
        return scrollDownUp(resourceId, maxSwipes, steps, 1);
    }

    protected boolean displaySwipeLeft() {
        return displaySwipeImpl(0);
    }

    protected boolean displaySwipeRight() {
        return displaySwipeImpl(1);
    }

    private boolean displaySwipeImpl(int direction) {
        if (displayHeight == 0) {
            displayHeight = mDevice.getDisplayHeight();
        }
        if (displayWidth == 0) {
            displayWidth = mDevice.getDisplayWidth();
        }
        int y = displayHeight / 2;
        int startX, endX;
        if (direction == 0) {
            // Swipe left.
            startX = displayWidth * 3 / 4;
            endX = displayWidth / 4;
        } else {
            // Swipe right.
            startX = displayWidth / 4;
            endX = displayWidth * 3 / 4;
        }
        return mDevice.swipe(startX, y, endX, y, SWIPE_STEPS);
    }

    /**
     * Replay touch event using "revent".
     * @param eventFilename filename of event record file
     */
    protected final void replayEvent(String eventFilename) {
        mUiAuto.executeShellCommand(
                String.format("%s replay %s/%s", REVENT_BIN, EVENT_BASEPATH, eventFilename));
    }

    /**
     * Shortcut to Thread.sleep.
     * @param milliseconds sleep in milliseconds.
     */
    protected final void sleep(long milliseconds) throws InterruptedException {
        java.lang.Thread.sleep(milliseconds);
    }
}
