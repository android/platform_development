/*
 * Copyright (C) 2019 The Android Open Source Project
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

package com.example.icu4ctestapp;

import android.os.Bundle;
import android.app.Activity;
import android.util.Log;
import android.widget.TextView;
import java.io.File;
import java.io.IOException;

public class MainActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        try {
            System.loadLibrary("icu4c_test_app_native_lib");
        } catch (Throwable e) {
            System.loadLibrary("icu4c_test_app_native_lib_2");
        }

        try {
            Runtime.getRuntime().exec(new File(getApplicationInfo().nativeLibraryDir, "libicu4ctest.so").getAbsolutePath());
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
        String result = String.format("libicuuc: %s\n", testLibicuuc() ? "OK" : "NOT_FOUND");
        result += String.format("libicui18n: %s\n", testLibicui18n() ? "OK" : "NOT_FOUND");
        TextView v = findViewById(R.id.textview);
        v.setText(result);
    }

    private native boolean testLibicuuc();
    private native boolean testLibicui18n();
}
