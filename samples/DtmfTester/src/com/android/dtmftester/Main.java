/*
 * Copyright (C) 2012 The Android Open Source Project
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
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.telephony.PhoneStateListener;
import android.telephony.PreciseCallState;
import android.telephony.TelephonyManager;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.TabHost;
import android.widget.TextView;

/**
 * Test DTMF functionality.
 */
public class Main extends Activity {
    private static final String LOG_TAG = "DmtfTester";
    private EditText editDtmf;
    private EditText editNumber;
    private TextView textStatus;
    private TextView textResult;
    private TextView currentNumber;
    private TextView eventLog;
    private Button copyNumber;
    private Button startDtmf;
    private Button stopDtmf;
    private Button sendDtmf;
    private Button sendDtmfCode;
    private TelephonyManager mgr;

    @Override
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        setContentView(R.layout.main);
        TabHost tabs = (TabHost) findViewById(R.id.tabhost);
        tabs.setup();
        TabHost.TabSpec spec = tabs.newTabSpec("senderTab");
        spec.setContent(R.id.senderTab);
        spec.setIndicator("DTMF Sender");
        tabs.addTab(spec);
        spec = tabs.newTabSpec("logTab");
        spec.setContent(R.id.logTab);
        spec.setIndicator("Event log");
        tabs.addTab(spec);

        mgr = (TelephonyManager) getSystemService(TELEPHONY_SERVICE);
        editDtmf = (EditText) findViewById(R.id.editDtmf);
        editNumber = (EditText) findViewById(R.id.editNumber);
        textStatus = (TextView) findViewById(R.id.textStatus);
        textResult = (TextView) findViewById(R.id.textResult);
        eventLog = (TextView) findViewById(R.id.logTab);
        eventLog.setMovementMethod(new ScrollingMovementMethod());
        currentNumber = (TextView) findViewById(R.id.currentNumber);
        copyNumber = (Button) findViewById(R.id.copyNumber);
        copyNumber.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                if(currentNumber.getText() != "") {
                    editNumber.setText(currentNumber.getText());
                }
            }
        });

        startDtmf = (Button) findViewById(R.id.startDtmf);
        startDtmf.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                PreciseCallState[] state = mgr.getPreciseCallState();
                if (!hasActiveCall(state)) {
                    return;
                }
                try {
                    final char c = editDtmf.getText().charAt(0);
                    int result = mgr.startDtmf(editNumber.getText().toString(), c);
                    logDtmfResult(result);
                    showMsg("Start DTMF " + c);
                } catch (IndexOutOfBoundsException e) {
                    showMsg("No character to send");
                }
            }

        });
        stopDtmf = (Button) findViewById(R.id.stopDtmf);
        stopDtmf.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                PreciseCallState[] state = mgr.getPreciseCallState();
                if (!hasActiveCall(state)) {
                    return;
                }
                int result = mgr.stopDtmf(editNumber.getText().toString());
                showMsg("Stop DTMF");
                logDtmfResult(result);
            }

        });
        sendDtmf = (Button) findViewById(R.id.sendDtmf);
        sendDtmf.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                PreciseCallState[] state = mgr.getPreciseCallState();
                if (!hasActiveCall(state)) {
                    return;
                }
                String s = editDtmf.getText().toString();
                if (s.length() == 0) {
                    showMsg("No characters to send");
                    return;
                }

                int result = TelephonyManager.DTMF_STATUS_OK;
                for (int i = 0; i != editDtmf.getText().length(); i++) {
                    result = mgr.sendDtmf(editNumber.getText().toString(), editDtmf.getText().charAt(i));
                    if (result != TelephonyManager.DTMF_STATUS_OK) {
                        break;
                    }
                    Log.d("DtmfTester", "sent char " + editDtmf.getText().charAt(i));
                }
                showMsg("Send DTMF string ");
                logDtmfResult(result);

            }

        });

        sendDtmfCode = (Button) findViewById(R.id.sendDtmfCode);
        sendDtmfCode.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                PreciseCallState[] state = mgr.getPreciseCallState();
                if (!hasActiveCall(state)) {
                    return;
                }
                String s = editDtmf.getText().toString();
                if (s.length() == 0) {
                    showMsg("No characters to send");
                    return;
                }

                try {
                    final char c = editDtmf.getText().charAt(0);
                    int result = mgr.sendDtmf(editNumber.getText().toString(), c);
                    showMsg("Send DTMF code " + c);
                    logDtmfResult(result);
                } catch (IndexOutOfBoundsException e) {
                    showMsg("No character to send");
                }
            }

        });

        PreciseCallState[] state = mgr.getPreciseCallState();
        if (!hasActiveCall(state)) {
            textStatus.setText("no active call");
            setControlsEnabled(false);
        } else {
            textStatus.setText("call in progress");
            setControlsEnabled(true);
        }

        PhoneStateListener callListener = new PhoneStateListener() {

            @Override
            public void onPreciseCallStateChanged(PreciseCallState state) {
                super.onPreciseCallStateChanged(state);

                if(state.addresses.length == 1) {
                    logPreciseCallEvent(state.state, state.addresses[0]);
                } else {
                    logPreciseCallEvent(state.state, null);
                }

                if (state.state == TelephonyManager.CALL_PRECISE_STATE_ACTIVE) {
                    textStatus.setText("call in progress");
                    if(state.addresses != null && state.addresses.length == 1) {
                        currentNumber.setText(state.addresses[0]);
                    }
                    setControlsEnabled(true);
                } else {
                    currentNumber.setText("");
                    textStatus.setText("no active call");
                    setControlsEnabled(false);
                }
            }

        };

        mgr.listen(callListener, PhoneStateListener.LISTEN_PRECISE_CALL_STATE);
    }

    private void showMsg(String txt) {
        textStatus.setText(txt);
    }

    private void setControlsEnabled(boolean en) {
        editDtmf.setEnabled(en);
        editNumber.setEnabled(en);
        copyNumber.setEnabled(en);
        startDtmf.setEnabled(en);
        stopDtmf.setEnabled(en);
        sendDtmf.setEnabled(en);
        sendDtmfCode.setEnabled(en);
    }

    private void logDtmfResult(int result) {
        String eventMsg = "";
        switch (result) {
            case TelephonyManager.DTMF_STATUS_INVALID_ADDRESS:
                eventMsg += "Error, invalid address";
                break;
            case TelephonyManager.DTMF_STATUS_NO_CONNECTION:
                eventMsg += "Error, no connection";
                break;
            case TelephonyManager.DTMF_STATUS_MULTIPLE_CONNECTIONS:
                eventMsg += "Error, multiple connections";
                break;
            case TelephonyManager.DTMF_STATUS_NO_ACTIVE_CALL:
                eventMsg += "Error, no active call";
                break;
            case TelephonyManager.DTMF_STATUS_INVALID_DTMF_CHARACTER:
                eventMsg += "Error, invalid character";
                break;
            case TelephonyManager.DTMF_STATUS_GENERAL_ERROR :
                eventMsg += "General error";
                break;
            default:
                eventMsg += "OK";
        }
        Log.d(LOG_TAG, eventMsg);
        textResult.setText(eventMsg);
    }

    private void logPreciseCallEvent(int event, String address) {
        String eventMsg = "State ";
        switch (event) {
            case TelephonyManager.CALL_PRECISE_STATE_ACTIVE:
                eventMsg += "ACTIVE";
                break;
            case TelephonyManager.CALL_PRECISE_STATE_HOLDING:
                eventMsg += "HOLDING";
                break;
            case TelephonyManager.CALL_PRECISE_STATE_DIALING:
                eventMsg += "DIALING";
                break;
            case TelephonyManager.CALL_PRECISE_STATE_ALERTING:
                eventMsg += "ALERTING";
                break;
            case TelephonyManager.CALL_PRECISE_STATE_INCOMING:
                eventMsg += "INCOMING";
                break;
            case TelephonyManager.CALL_PRECISE_STATE_WAITING:
                eventMsg += "WAITING";
                break;
            case TelephonyManager.CALL_PRECISE_STATE_DISCONNECTED:
                eventMsg += "DISCONNECTED";
                break;
            case TelephonyManager.CALL_PRECISE_STATE_DISCONNECTING:
                eventMsg += "DISCONNECTING";
                break;
            default:
                eventMsg += "IDLE";
        }
        if(address != null) {
            eventMsg += ", address " + address;
        }
        Log.d(LOG_TAG, eventMsg);
        eventLog.append(eventMsg + "\n");
        // Scroll TextView do the bottom
        if(eventLog.getLayout() != null) {
            final int scroll = eventLog.getLayout().getLineTop(eventLog.getLineCount())
                - eventLog.getHeight();
            eventLog.post(new Runnable()
            {
                public void run()
                {
                    if(scroll > 0)
                        eventLog.scrollTo(0, scroll);
                    else
                        eventLog.scrollTo(0,0);
                }
            });
        }
    }

    private static boolean hasActiveCall(PreciseCallState[] callStates) {
        if (callStates == null || callStates.length == 0) {
            return false;
        }
        for (int i = 0; i != callStates.length; i++) {
            if(callStates[i] != null
                    && callStates[i].state == TelephonyManager.CALL_PRECISE_STATE_ACTIVE) {
                return true;
            }
        }
        return false;
    }
}
