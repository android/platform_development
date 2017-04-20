/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.android.sample.Embms;

import android.app.Service;
import android.content.Intent;
import android.net.Uri;
import android.os.IBinder;
import android.telephony.mbms.vendor.IMbmsStreamingService;
import android.telephony.mbms.IMbmsStreamingManagerListener;
import android.telephony.mbms.IStreamingListener;
import android.telephony.mbms.StreamingService;
import android.telephony.mbms.StreamingServiceInfo;
import android.telephony.SignalStrength;
import android.util.Log;

import java.util.List;

class SampleEmbmsStream extends Service {
    private final static String TAG = "SampleEmbmsStream";

    SampleEmbmsStream() {
    }

    @Override
    public void onCreate() {
        super.onCreate();
        logd("SampleEmbmsStream service onCreate");
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    private final IMbmsStreamingService.Stub mBinder = new IMbmsStreamingService.Stub() {
        @Override
        public void initialize(IMbmsStreamingManagerListener listener, String appName, int subId) {
            logd("initialize " + appName + ":" + subId);
        }

        @Override
        public void getStreamingServices(String appName, int subId, List<String> serviceClasses) {
            logd("getStreamingService " + appName + ":" + subId);
        }

        @Override
        public StreamingService startStreaming(String appName, int subId, String serviceId,
                IStreamingListener listener) {
            logd("startStreaming " + appName + ":" + subId + " starting [" + serviceId + "]");
            return null;
        }

        @Override
        public List<StreamingServiceInfo> getActiveStreamingServices(String appName, int subId) {
            logd("getActiveStreamingService " + appName + ":" + subId);
            return null;
        }

        @Override
        public Uri getPlaybackUri(String appName, int subId, String serviceId) {
            logd("getPlaybackUri " + appName + ":" + subId + " for [" + serviceId + "]");
            return null;
        }

        @Override
        public void switchStreams(String appName, int subId, String oldServiceId,
                String newServiceId) {
            logd("switchStreams " + appName + ":" + subId + " [" + oldServiceId + "]->["
                    + newServiceId + "]");
        }

        @Override
        public int getState(String appName, int subId, String serviceId) {
            logd("getState " + appName + ":" + subId + " [" + serviceId + "]");
            return 0;
        }

        @Override
        public void stopStreaming(String appName, int subId, String serviceId) {
            logd("stopStreaming " + appName + ":" + subId + " [" + serviceId + "]");
        }

        @Override
        public SignalStrength getSignalStrength(String appName, int subId, String serviceId) {
            logd("getSignalStrength " + appName + ":" + subId + " [" + serviceId + "]");
            return null;
        }

        @Override
        public void disposeStream(String appName, int subId, String serviceId) {
            logd("disposeStream " + appName + ":" + subId + " [" + serviceId + "]");
        }

        @Override
        public void dispose(String appName, int subId) {
            logd("dispose " + appName + ":" + subId);
        }
    };

    private static void logd(String s) {
        Log.d(TAG, s);
    }
}
