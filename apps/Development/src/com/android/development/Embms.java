/*
 * Copyright (C) 2007 The Android Open Source Project
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

package com.android.development;

import android.app.Activity;
import android.os.Bundle;
import android.telephony.MbmsStreamingManager;
import android.telephony.mbms.MbmsStreamingManagerListener;
import android.telephony.mbms.StreamingServiceInfo;
import android.util.Log;
import android.view.View;
import android.widget.EditText;
import android.widget.TextView;

import java.lang.StringBuilder;
import java.util.Arrays;
import java.util.List;

public class Embms extends Activity
{
    private static final String TAG = "DevEmbms";
    private MbmsStreamingManager mStreamingManager;
    private MbmsStreamingManagerListener mStreamingManagerListener =
            new MbmsStreamingManagerListener() {
                private static final String LOGTAG = TAG + " MbmsStreamingManagerListener";
                public void error(int errorCode, String message) {
                    Log.d(LOGTAG, message + ": " + errorCode);
                }
                public void streamingServicesUpdated(List<StreamingServiceInfo> services) {
                    TextView textView = ((TextView)findViewById(R.id.streamServiceList));
                    StringBuilder sb = new StringBuilder("Available Stream Services:");
                    for (StreamingServiceInfo info : services) {
                        sb.append("\n").append(info.toString());
                    }
                    textView.setText(sb.toString());
                }
                public void disposed() {
                }
            };

    public Embms() {
    }

    /** Called when the activity is first created or resumed. */
    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        setContentView(R.layout.embms);

        mStreamingManagerListener = mStreamingManagerListener;
        mStreamingManager = MbmsStreamingManager.createManager(this, mStreamingManagerListener,
            "Embms Development");

        findViewById(R.id.getStreamingServices).setOnClickListener(mClickListener);

    }

    /** Called when the activity going into the background or being destroyed. */
    @Override
    public void onDestroy() {
        super.onDestroy();

        if (mStreamingManager != null) {
            mStreamingManager.dispose();
            mStreamingManager = null;
        }
    }

    private List<String> toStringList(String string) {
        String[] split = string.split("\\s+");
        return Arrays.asList(split);
    }

    private View.OnClickListener mClickListener = new View.OnClickListener() {
        public void onClick(View v) {
            switch (v.getId()) {
                case R.id.getStreamingServices:
                {
                    final String serviceClasses =
                            ((EditText)findViewById(R.id.streaming_service_classes)).getText().
                            toString();
                    mStreamingManager.getStreamingServices(toStringList(serviceClasses));
                    break;
                }
                default:
            }
        }
    };

}
