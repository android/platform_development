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

package com.google.android.apps.puppeteer.metrics;

import com.google.android.apps.puppeteer.UiMetric;

import java.io.File;
import java.io.FileFilter;
import java.util.regex.Pattern;

/**
 * Base abstract class for CPU-related metrics.
 * Common runtime attributes are collected in static initializer for future usage.
 */
public abstract class CPUMetric extends UiMetric {

    protected static final int NUM_CPUS;

    static {
        // Find number of CPUs in the system.
        File dir = new File("/sys/devices/system/cpu");
        File[] files = dir.listFiles(new CPUFilter());
        NUM_CPUS = files.length;
    }

    public CPUMetric(String loggerName) {
        super(loggerName);
    }

    private static class CPUFilter implements FileFilter {
        @Override
        public boolean accept(File pathname) {
            return Pattern.matches("cpu\\d+", pathname.getName());
        }
    }
}
