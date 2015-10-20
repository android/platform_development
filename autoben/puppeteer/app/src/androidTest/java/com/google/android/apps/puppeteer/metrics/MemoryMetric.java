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

import android.app.ActivityManager;
import android.content.Context;
import android.os.Debug;
import android.os.ParcelFileDescriptor;
import android.support.test.InstrumentationRegistry;

import com.google.android.apps.puppeteer.UiMetric;

import java.util.ArrayList;
import java.util.List;

public class MemoryMetric extends UiMetric {

    public MemoryMetric() {
        super("memory");
    }

    public void measure() {
        List<Integer> pids = new ArrayList<>();
        final String packageName = getPackageName();
        String[] psResults = executeShellCommand("ps").split("\n");
        for (String psLine: psResults) {
            String[] fields = psLine.split("\\s+");
            String pid = fields[1];
            String processName = fields[fields.length - 1];
            if (packageName.equals(processName)) {
                pids.add(Integer.parseInt(pid));
            }
        }

        ActivityManager am = (ActivityManager) InstrumentationRegistry.getContext()
                .getSystemService(Context.ACTIVITY_SERVICE);
        Debug.MemoryInfo[] memoryInfos = am.getProcessMemoryInfo(toPrimitiveInt(pids));
        int totalPss = 0;
        for (Debug.MemoryInfo memoryInfo : memoryInfos) {
            totalPss += memoryInfo.getTotalPss();
        }
        addRecord(totalPss, "total_pss");
    }

    private static int[] toPrimitiveInt(List<Integer> list) {
        int[] newList = new int[list.size()];
        for (int i = 0; i < list.size(); i++) newList[i] = list.get(i);
        return newList;
    }
}
