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

import android.os.ParcelFileDescriptor;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;


/**
 * Abstract class of metric recorder.
 *
 * Its subclasses should implement measure() to measure metric(s).
 */
public abstract class UiMetric extends RecordLogger implements Runnable {

    private MetricRecorder mMetricRecorder;

    /**
     * Constructor.
     * @param loggerName filename of log to save under baseDir.
     */
    public UiMetric(String loggerName) {
        super(loggerName);
    }

    /**
     * Measures metric.
     *
     * Derived classes should implement measurement procedure and store the result to attribute for
     * saveToFile() to write out.
     */
    public abstract void measure();

    /**
     * Runnable interface for ease of use with threads.
     */
    @Override
    public final void run() {
        measure();
    }

    void setMetricRecorder(MetricRecorder metricRecorder) {
        setBaseDir(metricRecorder.getWorkingDir());
        mMetricRecorder = metricRecorder;
    }

    protected String getPackageName() {
        return mMetricRecorder.getPackageName();
    }

    /**
     * Executes a command and returns output as String.
     * @param command command to execute
     * @return output
     */
    protected String executeShellCommand(String command) {
        ParcelFileDescriptor commandOutput = mMetricRecorder.getUiAuto().executeShellCommand(
                command);
        return InputStreamToString(
                new BufferedInputStream(new FileInputStream(commandOutput.getFileDescriptor())));
    }

    /**
     * Converts InputStream to String.
     * @param input InputStream
     * @return String
     */
    protected static String InputStreamToString(InputStream input) {
        BufferedReader reader = new BufferedReader(new InputStreamReader(input));
        StringBuilder builder = new StringBuilder();
        String string;
        try {
            while ((string = reader.readLine()) != null) {
                builder.append(string).append("\n");
            }
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            try {
                input.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        return builder.toString();
    }
}
