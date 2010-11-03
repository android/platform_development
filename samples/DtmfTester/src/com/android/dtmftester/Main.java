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

package com.android.dtmftester;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.telephony.TelephonyManager;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

/**
 * Test DTMF functionality.
 */
public class Main extends Activity {

    private EditText editDtmf;
    private Button startDtmf;
    private Button stopDtmf;
    private Button sendDtmf;
    private Button sendDtmfBcast;
    private TelephonyManager mgr;

    @Override
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        
        setContentView(R.layout.main);
        mgr = (TelephonyManager) getSystemService(TELEPHONY_SERVICE);
        editDtmf = (EditText) findViewById(R.id.editDtmf);
        startDtmf = (Button) findViewById(R.id.startDtmf);
        startDtmf.setOnClickListener(new OnClickListener() {
            
            @Override
            public void onClick(View v) {
                if (!mgr.isCallActive()) {
                    showMsg("No active call");
                    return;
                }
                try {
                    char c = editDtmf.getText().charAt(0);
                    mgr.startDtmf(c);
                    showMsg("DTMF character started: " + c);                    
                } catch (IndexOutOfBoundsException e) {
                    showMsg("No character to send");
                }
            }

        });
        stopDtmf = (Button) findViewById(R.id.stopDtmf);
        stopDtmf.setOnClickListener(new OnClickListener() {
            
            @Override
            public void onClick(View v) {
                if (!mgr.isCallActive()) {
                    showMsg("No active call");
                    return;
                }
                mgr.stopDtmf();
                showMsg("DTMF character sending stopped");                    
            }

        });
        sendDtmf = (Button) findViewById(R.id.sendDtmf);
        sendDtmf.setOnClickListener(new OnClickListener() {
            
            @Override
            public void onClick(View v) {
                if (!mgr.isCallActive()) {
                    showMsg("No active call");
                    return;
                }
                String s = editDtmf.getText().toString();
                if (s.length() == 0) {
                    showMsg("No characters to send");
                }
                mgr.sendDtmfString(s, 0, 0, true);
                showMsg("Sent DTMF characters (default length and pause, local sound on): " + s);                    
            }

        });
        sendDtmfBcast = (Button) findViewById(R.id.sendDtmfStringBroadcast);
        sendDtmfBcast.setOnClickListener(new OnClickListener() {
            
            @Override
            public void onClick(View v) {
                if (!mgr.isCallActive()) {
                    showMsg("No active call");
                    return;
                }
                String s = editDtmf.getText().toString();
                if (s.length() == 0) {
                    showMsg("No characters to send");
                }
                Intent i = new Intent(TelephonyManager.ACTION_SEND_DTMF);
                i.putExtra(TelephonyManager.EXTRA_DTMF_STRING, s);
                i.putExtra(TelephonyManager.EXTRA_DTMF_SOUND, true);
                Main.this.sendBroadcast(i);
                showMsg("Sent broadcast DTMF characters (default length and pause, local sound on): " + s);
            }

        });
    }

    private void showMsg(String txt) {
        Toast t = Toast.makeText(Main.this, txt, Toast.LENGTH_SHORT);
        t.show();        
    }
    
}
