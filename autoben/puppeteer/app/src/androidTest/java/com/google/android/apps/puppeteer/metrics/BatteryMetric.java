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

import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class BatteryMetric extends UiMetric {

    private static final Pattern BATTERY_LEVEL_PATTERN;

    static {
        BATTERY_LEVEL_PATTERN = Pattern.compile("level: (\\d+)", Pattern.MULTILINE);
    }

    public BatteryMetric() {
        super("battery");
    }

    public void measure() {
        String batteryInfo = executeShellCommand("dumpsys battery");
        Matcher matcher = BATTERY_LEVEL_PATTERN.matcher(batteryInfo);
        if (matcher.find()) {
            int batteryLevel = Integer.parseInt(matcher.group(1));
            addRecord(batteryLevel, "battery_level");
        }
    }
}
