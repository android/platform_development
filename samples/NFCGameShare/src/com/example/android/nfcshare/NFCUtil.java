/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

package com.example.android.nfcshare;

import android.content.Context;
import android.content.Intent;
import android.nfc.NdefMessage;
import android.nfc.NdefRecord;
import android.nfc.NfcAdapter;
import android.nfc.Tag;
import android.nfc.tech.Ndef;
import android.nfc.tech.NdefFormatable;
import android.os.Parcelable;
import android.util.Log;
import android.widget.Toast;

import java.io.IOException;

public class NFCUtil implements Constants {

    private static final String TAG = "NFCUtil";

    public static NdefMessage[] getNdefMessages(Intent intent) {

        // Parse the intent
        NdefMessage[] msgs = null;
        String action = intent.getAction();
        if (NfcAdapter.ACTION_NDEF_DISCOVERED.equals(action)) {
            Parcelable[] rawMsgs = intent
                    .getParcelableArrayExtra(NfcAdapter.EXTRA_NDEF_MESSAGES);
            if (rawMsgs != null) {
                msgs = new NdefMessage[rawMsgs.length];
                for (int i = 0; i < rawMsgs.length; i++) {
                    msgs[i] = (NdefMessage) rawMsgs[i];
                }
            } else {
                // Unknown tag type
                byte[] empty = new byte[] {};
                NdefRecord record = new NdefRecord(NdefRecord.TNF_UNKNOWN,
                        empty, empty, empty);
                NdefMessage msg = new NdefMessage(new NdefRecord[] {
                        record
                });
                msgs = new NdefMessage[] {
                        msg
                };
            }
        } else if (NfcAdapter.ACTION_TECH_DISCOVERED.equals(action)) {
            Log.d(TAG, "ACTION_TECH_DISCOVERED intent.");
        } else if (NfcAdapter.ACTION_TAG_DISCOVERED.equals(action)) {
            Log.d(TAG, "Unknown intent. ACTION_TAG_DISCOVERED Discovered.");
        }
        return msgs;
    }

    public static boolean writeTag(NdefMessage message, Tag tag, Context ctx) {
        int size = message.toByteArray().length;
        try {
            Ndef ndef = Ndef.get(tag);
            if (ndef != null) {
                ndef.connect();

                if (!ndef.isWritable()) {
                    toast("Tag is read-only.", ctx);
                    return false;
                }
                if (ndef.getMaxSize() < size) {
                    toast("Tag capacity is " + ndef.getMaxSize()
                            + " bytes, message is " + size + " bytes.", ctx);
                    return false;
                }

                ndef.writeNdefMessage(message);
                toast("Wrote message to pre-formatted tag.", ctx);
                return true;
            } else {
                NdefFormatable format = NdefFormatable.get(tag);
                if (format != null) {
                    try {
                        format.connect();
                        format.format(message);
                        toast("Formatted tag and wrote message", ctx);
                        return true;
                    } catch (IOException e) {
                        toast("Failed to format tag.", ctx);
                        return false;
                    }
                } else {
                    toast("Tag doesn't support NDEF.", ctx);
                    return false;
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
            toast("Failed to write to tag!", ctx);
        }

        return false;
    }

    public static void toast(String text, Context ctx) {
        Toast.makeText(ctx, text, Toast.LENGTH_SHORT).show();
    }

    public static NdefMessage createNdefMsg(int[] mGameMatrix) {
        NdefMessage ndefMsg = NFCUtil.getIntArrayAsNdef(
                NdefRecord.TNF_MIME_MEDIA, MIME_TYPE, "", mGameMatrix,
                DELIMETER);
        return ndefMsg;
    }

    // With AAR
    public static NdefMessage createNdefMsg(int[] mGameMatrix, NdefRecord AAR) {
        NdefMessage ndefMsg = NFCUtil.getIntArrayAsNdef(
                NdefRecord.TNF_MIME_MEDIA, MIME_TYPE, "", mGameMatrix,
                DELIMETER, AAR);

        return ndefMsg;
    }

    public static NdefMessage getIntArrayAsNdef(short tnf, String type,
            String id, int[] payload, String delimiter) {
        byte[] idBytes = id.getBytes();

        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < payload.length - 1; i++) {
            sb.append(payload[i] + DELIMETER);
        }
        sb.append(payload[payload.length - 1] + "");

        Log.d(TAG, "Payload for ndefmsg = " + sb.toString());

        byte[] payloadBytes = sb.toString().getBytes();
        byte[] typeBytes = type.getBytes();
        NdefRecord textRecord = new NdefRecord(tnf, typeBytes, idBytes,
                payloadBytes);

        return new NdefMessage(new NdefRecord[] {
                textRecord
        });
    }

    // with AAR
    public static NdefMessage getIntArrayAsNdef(short tnf, String type,
            String id, int[] payload, String delimiter, NdefRecord AAR) {
        byte[] idBytes = id.getBytes();

        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < payload.length - 1; i++) {
            sb.append(payload[i] + DELIMETER);
        }
        sb.append(payload[payload.length - 1] + "");

        Log.d(TAG, "Payload for ndefmsg = " + sb.toString());

        byte[] payloadBytes = sb.toString().getBytes();
        byte[] typeBytes = type.getBytes();
        NdefRecord textRecord = new NdefRecord(tnf, typeBytes, idBytes,
                payloadBytes);

        return new NdefMessage(new NdefRecord[] {
                textRecord, AAR
        });
    }

    public static NdefMessage getStringAsNdef(short tnf, String type,
            String id, String payload) {
        byte[] idBytes = id.getBytes();
        byte[] payloadBytes = payload.getBytes();
        byte[] typeBytes = type.getBytes();
        NdefRecord textRecord = new NdefRecord(tnf, typeBytes, idBytes,
                payloadBytes);
        return new NdefMessage(new NdefRecord[] {
                textRecord
        });
    }

}
