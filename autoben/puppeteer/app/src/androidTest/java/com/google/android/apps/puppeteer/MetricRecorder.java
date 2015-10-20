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
import android.os.Environment;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;

/**
 * Contains a list of metrics to measure them periodically.
 * Use addMetric() to add metric to measure. Provides run() to invoke metrics' run() periodically
 * till stopRunning() is called.
 */
public class MetricRecorder extends Thread {

    // Path to store metric.
    private File mWorkingDir;

    // True to tell run() stop running metrics periodically.
    private boolean mIsStop;

    // Package name for metric to obtain package specific measure.
    private String mPackageName;

    // Interval between measurement.
    private int mSleepMs;

    // Used for metric to access UiAutomation object.
    private UiAutomation mUiAuto;

    private List<UiMetric> mMetrics;

    /**
     * Creates a MetricRecorder.
     * @param uiAuto UiAutomation object
     * @param sleepMs sleep interval in milliseconds
     * @param packageName package name
     * @param savedDirName saved directory name under /ext_storage/metrics
     */
    public MetricRecorder(UiAutomation uiAuto, int sleepMs, String packageName,
                          String savedDirName) {
        mIsStop = false;
        mMetrics = new ArrayList<>();
        this.mUiAuto = uiAuto;
        this.mSleepMs = sleepMs;
        this.mPackageName = packageName;

        File externalStorageDirectory = Environment.getExternalStorageDirectory();
        mWorkingDir = new File(externalStorageDirectory.getAbsolutePath() + File.separator +
                "metrics" + File.separator + savedDirName);
        mWorkingDir.mkdirs();
    }

    /**
     * Adds a UiMetric object for measurement.
     * @param metric UiMetric object
     */
    public void addMetric(UiMetric metric) {
        metric.setMetricRecorder(this);
        mMetrics.add(metric);
    }

    /**
     * Measures metrics till stopRunning() called.
     *
     * It creates threads for each metric to run measure() and waits for mSleepMs milliseconds to
     * join the threads. After that, it creates again till stopRunning() called.
     *
     * After stopRunning() is called, it waits for all metrics' thread stop and then tell them to
     * save result to file.
     */
    public void run() {
        ExecutorService threadPool = Executors.newFixedThreadPool(mMetrics.size());
        List<Future> futures = new ArrayList<>(mMetrics.size());

        while (!mIsStop) {
            for (UiMetric metric : mMetrics) {
                futures.add(threadPool.submit(metric));
            }
            try {
                Thread.sleep(mSleepMs);
                for (Future future : futures) {
                    future.get();
                }
            } catch (InterruptedException | ExecutionException e) {
                e.printStackTrace();
            }
            futures.clear();
        }

        // Tell metrics to save result to file.
        for (UiMetric metric : mMetrics) {
            metric.saveToFile();
        }

        threadPool.shutdown();
    }

    /**
     * Stops run().
     */
    public void stopRunning() {
        mIsStop = true;
    }

    public UiAutomation getUiAuto() {
        return mUiAuto;
    }

    public String getPackageName() {
        return mPackageName;
    }

    public String getWorkingDir() {
        return mWorkingDir.getPath();
    }

}
