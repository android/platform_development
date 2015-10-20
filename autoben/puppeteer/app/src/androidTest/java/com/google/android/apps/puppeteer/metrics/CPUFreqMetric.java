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
import java.io.IOException;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * Measures CPUFreq stats provided by the kernel.
 * Records frequency distributions of each CPU by sampling cumulative residency time from
 * /sys/devices/system/cpu/cpu\d+/cpufreq/stats/time_in_state and calculating deltas.
 */
public class CPUFreqMetric extends CPUMetric {

    private static final String TAG = "CPUFreqMetric";
    private static final int NUM_AVAILABLE_FREQ;
    private static final int[] AVAILABLE_FREQ;

    /**
     * Collects frequency state info during static initialization.
     *
     * @throws RuntimeException on error reading CPUFreq time_in_state
     */
    static {
        FreqTime[] stateHistogram = getTimeInState(0);
        if (stateHistogram == null) {
            throw new RuntimeException("Cannot get CPUFreq time_in_state information");
        }
        NUM_AVAILABLE_FREQ = stateHistogram.length;
        AVAILABLE_FREQ = new int[NUM_AVAILABLE_FREQ];
        for (int i = 0; i < NUM_AVAILABLE_FREQ; i++) {
            AVAILABLE_FREQ[i] = stateHistogram[i].freq;
        }
    }

    private boolean mFirstSample = true;
    private long[][] mLastTimeInState = null;

    public CPUFreqMetric() {
        super("cpu_freq_time_in_state");
    }

    /**
     * Reads CPUFreq time in state stats of specified CPU.
     *
     * @param cpu the specified CPU id
     * @return list of FreqTime tuples of the specific CPU.
     */
    private static FreqTime[] getTimeInState(int cpu) {
        String path = "/sys/devices/system/cpu/cpu" + cpu + "/cpufreq/stats/time_in_state";
        File file = new File(path);
        List<String> lines = null;
        try {
            lines = Files.readLines(file, Charset.defaultCharset());
        } catch (IOException e) {
            Log.e(TAG, "Cannot open file " + path);
        }

        if (lines == null) {
            return new FreqTime[0];
        }

        FreqTime[] result = new FreqTime[lines.size()];
        for (int i = 0; i < result.length; ++i) {
            // Fields: freq (kHz), time in state (10ms unit).
            String[] fields = lines.get(i).split(" ");
            int freq = Integer.parseInt(fields[0]);
            long timeMs = Long.parseLong(fields[1]) * 10;
            result[i] = new FreqTime(freq, timeMs);
        }
        return result;
    }

    private static long[][] getAllCpuTimeInState() {
        long[][] result = new long[NUM_CPUS][NUM_AVAILABLE_FREQ];
        List<FreqTime[]> allStateHistogram = new ArrayList<>();
        for (int cpu = 0; cpu < NUM_CPUS; cpu++) {
            allStateHistogram.add(getTimeInState(cpu));
        }

        for (int cpu = 0; cpu < NUM_CPUS; cpu++) {
            FreqTime[] histogram = allStateHistogram.get(cpu);
            for (int i = 0; i < NUM_AVAILABLE_FREQ; i++) {
                result[cpu][i] = histogram[i].time;
            }
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

        long[] diffTimeInState = new long[NUM_AVAILABLE_FREQ];
        long totalDiffTime;
        for (int cpu = 0; cpu < NUM_CPUS; cpu++) {
            List<Map> record = new ArrayList<>();
            allCpuRecord.add(record);

            totalDiffTime = 0;
            for (int i = 0; i < NUM_AVAILABLE_FREQ; i++) {
                diffTimeInState[i] = currentTimeInState[cpu][i] - mLastTimeInState[cpu][i];
                totalDiffTime += diffTimeInState[i];
            }

            for (int i = 0; i < NUM_AVAILABLE_FREQ; i++) {
                if (diffTimeInState[i] > 0) {
                    Map<String, Object> freqTime = new ArrayMap<>();
                    freqTime.put("freq", AVAILABLE_FREQ[i]);
                    freqTime.put("time_us", diffTimeInState[i]);
                    freqTime.put("percent", 100.0 * diffTimeInState[i] / totalDiffTime);
                    record.add(freqTime);
                }
            }
            System.arraycopy(currentTimeInState[cpu], 0, mLastTimeInState[cpu], 0,
                    NUM_AVAILABLE_FREQ);
        }
        addRecord(recordTime, allCpuRecord, "freq_histogram_cpus");
    }

    private static class FreqTime {
        final int freq;
        final long time;

        FreqTime(int freq, long time) {
            this.freq = freq;
            this.time = time;
        }
    }
}
