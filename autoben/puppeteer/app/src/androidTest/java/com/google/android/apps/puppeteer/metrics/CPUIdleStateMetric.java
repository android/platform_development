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

import android.util.ArrayMap;
import android.util.Log;

import com.google.common.io.Files;

import java.io.File;
import java.io.FileFilter;
import java.io.IOException;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.regex.Pattern;

/**
 * Measures CPUIdle stats provided by the kernel.
 * Records idle state distributions of each CPU by sampling cumulative residency time from
 * /sys/devices/system/cpu/cpu\d+/cpuidle/state\d+/time and calculating deltas.
 */
public class CPUIdleStateMetric extends CPUMetric {

    private static final String TAG = "CPUIdleStateMetric";
    private static final int NUM_AVAILABLE_IDLE_STATES;
    private static final String[] AVAILABLE_IDLE_STATE_NAMES;

    /**
     * Collects idle state info during static initialization.
     *
     * @throws RuntimeException on error reading CPUIdle state name
     */
    static {
        File dir = new File("/sys/devices/system/cpu/cpu0/cpuidle");
        File[] files = dir.listFiles(new CPUIdleStateFilter());
        NUM_AVAILABLE_IDLE_STATES = files == null ? 0 : files.length;
        AVAILABLE_IDLE_STATE_NAMES = new String[NUM_AVAILABLE_IDLE_STATES];
        for (int i = 0; i < NUM_AVAILABLE_IDLE_STATES; i++) {
            String path = "/sys/devices/system/cpu/cpu0/cpuidle/state" + i + "/name";
            File file = new File(path);
            try {
                AVAILABLE_IDLE_STATE_NAMES[i] = Files.readFirstLine(file, Charset.defaultCharset());
            } catch (IOException e) {
                throw new RuntimeException("Cannot get CPUIdle state name information");
            }
        }
    }

    private boolean mFirstSample = true;
    private long[][] mLastTimeInState = null;

    public CPUIdleStateMetric() {
        super("cpu_idle_time_in_state");
    }

    /**
     * Reads CPUIdle time in state stats of specified CPU.
     *
     * @param cpu the specified CPU id
     * @return a long array of times in each state
     */
    private static long[] getTimeInState(int cpu) {
        long[] timeInState = new long[NUM_AVAILABLE_IDLE_STATES];
        for (int i = 0; i < NUM_AVAILABLE_IDLE_STATES; i++) {
            String path = "/sys/devices/system/cpu/cpu" + cpu + "/cpuidle/state" + i + "/time";
            File file = new File(path);
            String line = null;
            try {
                line = Files.readFirstLine(file, Charset.defaultCharset());
            } catch (IOException e) {
                Log.e(TAG, "Cannot open file " + path);
            }
            if (line == null) {
                timeInState[i] = 0;
            } else {
                timeInState[i] = Long.parseLong(line);
            }
        }
        return timeInState;
    }

    private static long[][] getAllCpuTimeInState() {
        long[][] result = new long[NUM_CPUS][NUM_AVAILABLE_IDLE_STATES];
        for (int cpu = 0; cpu < NUM_CPUS; cpu++) {
            long[] timeInState = getTimeInState(cpu);
            System.arraycopy(timeInState, 0, result[cpu], 0, NUM_AVAILABLE_IDLE_STATES);
        }
        return result;
    }

    public void measure() {
        if (mFirstSample) {
            mLastTimeInState = getAllCpuTimeInState();
            mFirstSample = false;
            return;
        }

        List<Object> allCpuRecord = new ArrayList<>();
        long[][] currentTimeInState = getAllCpuTimeInState();
        long recordTime = System.nanoTime();
        for (int cpu = 0; cpu < NUM_CPUS; cpu++) {
            List<Map> record = new ArrayList<>();
            allCpuRecord.add(record);
            for (int i = 0; i < NUM_AVAILABLE_IDLE_STATES; i++) {
                long diffTimeInState = currentTimeInState[cpu][i] - mLastTimeInState[cpu][i];
                if (diffTimeInState > 0) {
                    Map<String, Object> stateCount = new ArrayMap<>();
                    stateCount.put("c_state", AVAILABLE_IDLE_STATE_NAMES[i]);
                    stateCount.put("time_us", diffTimeInState);
                    record.add(stateCount);
                }
            }
            System.arraycopy(currentTimeInState[cpu], 0, mLastTimeInState[cpu], 0,
                    NUM_AVAILABLE_IDLE_STATES);
        }
        addRecord(recordTime, allCpuRecord, "idle_histogram_cpus");
    }

    private static class CPUIdleStateFilter implements FileFilter {
        @Override
        public boolean accept(File pathname) {
            return Pattern.matches("state\\d+", pathname.getName());
        }
    }
}
