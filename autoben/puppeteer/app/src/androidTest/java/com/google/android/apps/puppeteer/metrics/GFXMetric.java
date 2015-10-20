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

import android.content.Loader;
import android.util.ArrayMap;

import com.google.android.apps.puppeteer.UiMetric;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class GFXMetric extends UiMetric {

    private static final Pattern GFX_FRAME_TIME_PATTERN, GFX_JANKY_FRAME_PATTERN,
            GFX_TOTAL_FRAME_PATTERN;

    static {
        GFX_FRAME_TIME_PATTERN = Pattern.compile(
                "\\s+([0-9,.]+)\\s+([0-9,.]+)\\s+([0-9,.]+)\\s+([0-9,.]+)", Pattern.MULTILINE);
        GFX_JANKY_FRAME_PATTERN = Pattern.compile("Janky frames: (\\d+)");
        GFX_TOTAL_FRAME_PATTERN = Pattern.compile("Total frames rendered: (\\d+)");
    }

    public GFXMetric() {
        super("gfx");
    }

    public void measure() {
        String gfxInfo = executeShellCommand("dumpsys gfxinfo " + getPackageName());
        long recordTime = System.nanoTime();
        Map<String, Object> record = new ArrayMap<>();
        record.putAll(RetrieveFrameTime(gfxInfo));

        Matcher jankyMatcher = GFX_JANKY_FRAME_PATTERN.matcher(gfxInfo);
        if (jankyMatcher.find()) {
            record.put("janky frames", Integer.parseInt(jankyMatcher.group(1)));
        }

        Matcher totalFrameMatcher = GFX_TOTAL_FRAME_PATTERN.matcher(gfxInfo);
        if (totalFrameMatcher.find()) {
            record.put("total frames", Integer.parseInt(totalFrameMatcher.group(1)));
        }

        addRecord(record, "gfx");
    }

    private Map<String, Object> RetrieveFrameTime(String gfxInfo) {
        Map<String, Object> result = new ArrayMap<>();

        Matcher matcher = GFX_FRAME_TIME_PATTERN.matcher(gfxInfo);
        List<Float> frameTimes = new ArrayList<>();
        float sumFrameTime = 0;
        float numFrames = 0;
        while (matcher.find()) {
            float frameTime = 0;
            for (int i = 1; i < 5; ++i) {
                frameTime += Float.parseFloat(matcher.group(i));
            }
            frameTimes.add(frameTime);
            sumFrameTime += frameTime;
            numFrames += 1;
        }
        if (!frameTimes.isEmpty()) {
            result.put("frame_times", frameTimes);
            result.put("average_frame_time", sumFrameTime / numFrames);
        }
        return result;
    }
}
