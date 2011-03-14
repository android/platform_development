/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.example.android.fmradiotransmitter;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.hardware.fm.FmBand;
import android.hardware.fm.FmTransmitter;
import android.graphics.drawable.AnimationDrawable;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.SubMenu;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

public class FmRadioTransmitter extends Activity {

    // Scan range that works for all bands
    private static final int MINSCANRANGE = 88000;

    private static final int MAXSCANRANGE = 90000;

    // The 50kHz channel offset
    private static final int CHANNEL_OFFSET_50KHZ = 50;

    // The scan listener that receives the return values from the scans
    private FmTransmitter.OnScanListener mTransmitterScanListener;

    // The started listener is activated when the radio has started
    private FmTransmitter.OnStartedListener mTransmitterStartedListener;

    // Displays the currently tuned frequency
    private TextView mFrequencyTextView;

    // Handle to the FM radio Band object
    private FmBand mFmBand;

    // Handle to the FM radio transmitter object
    private FmTransmitter mFmTransmitter;

    // The frequency that the radio is currently tuned to.
    private int mCurrentfrequency;

    // The current band's channel offset.
    private int mFrequencyIncrement;

    // Handle to the Transmitter's Animation.
    private AnimationDrawable mTransmitAnimation;

    // The name of the storage string
    public static final String PREFS_NAME = "FMRadioPrefsFile";

    // The currently selected FM Radio band
    private int mSelectedBand;

    // The base menu identifier
    private static final int BASE_OPTION_MENU = 0;

    // The band menu identifier
    private static final int BAND_SELECTION_MENU = 1;

    // The menu items
    public static final int FM_BAND = Menu.FIRST;

    public static final int BAND_US = Menu.FIRST + 1;

    public static final int BAND_EU = Menu.FIRST + 2;

    public static final int BAND_JAPAN = Menu.FIRST + 3;

    public static final int BAND_CHINA = Menu.FIRST + 4;

    /**
     * Required method from parent class
     *
     * @param icicle - The previous instance of this app
     */
    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        setContentView(R.layout.main);
        mFmTransmitter = (FmTransmitter) getSystemService(Context.RADIO_FM_TRANSMITTER_SERVICE);
        SharedPreferences settings = getSharedPreferences(PREFS_NAME, 0);
        mSelectedBand = settings.getInt("selectedBand", 1);
        mFmBand = new FmBand(mSelectedBand);
        mFrequencyIncrement = mFmBand.getChannelOffset();
        setupButtons();
        setupAnimation();
    }

    /**
     * Starts up the listeners and the FM radio if it isn't already active
     */
    @Override
    protected void onStart() {
        super.onStart();
        mTransmitterStartedListener = new android.hardware.fm.FmTransmitter.OnStartedListener() {

            public void onStarted() {
                mTransmitAnimation.start();
                ((Button) findViewById(R.id.Transmit))
                        .setBackgroundResource(R.drawable.transmitstopbutton);
                mCurrentfrequency = mFmBand.getDefaultFrequency();
                mFrequencyIncrement = mFmBand.getChannelOffset();
                blockscan();
            }
        };

        mTransmitterScanListener = new android.hardware.fm.FmTransmitter.OnScanListener() {

            public void onBlockScan(int[] frequency, int[] signalStrength, boolean aborted) {
                ((Button) findViewById(R.id.BlockScan)).setEnabled(true);
                int minFreq = 0;
                int minSigStr = Integer.MAX_VALUE;
                for (int i = 0; i < frequency.length; i++) {
                    if (signalStrength[i] < minSigStr) {
                        minSigStr = signalStrength[i];
                        minFreq = frequency[i];
                    }
                }
                try {
                    mFmTransmitter.setFrequency(minFreq);
                } catch (Exception e) {
                    showToast("Unable to set the frequency after scanning", Toast.LENGTH_LONG);
                    return;
                }
                mCurrentfrequency = minFreq;
                displayFreq();
            }
        };
        mFmTransmitter.addOnScanListener(mTransmitterScanListener);
        mFmTransmitter.addOnStartedListener(mTransmitterStartedListener);
    }

    /**
     * Stops the FM Radio listeners
     */
    @Override
    protected void onStop() {
        super.onStop();
        if (mFmTransmitter != null) {
            mFmTransmitter.removeOnScanListener(mTransmitterScanListener);
            mFmTransmitter.removeOnStartedListener(mTransmitterStartedListener);
        }
    }

    /**
     * Saves the FmBand for next time the program is used and closes the radio
     * and media player.
     */
    @Override
    protected void onDestroy() {
        super.onDestroy();
        SharedPreferences settings = getSharedPreferences(PREFS_NAME, 0);
        SharedPreferences.Editor editor = settings.edit();
        editor.putInt("selectedBand", mSelectedBand);
        editor.commit();
        try {
            mFmTransmitter.reset();
        } catch (Exception e) {
            showToast("Unable to reset the radio hardware.", Toast.LENGTH_LONG);
        }
    }

    /**
     * Starts a blockscan in a seperate thread.
     */
    protected void blockscan() {
        showToast("Performining blockscan", Toast.LENGTH_LONG);
        Thread blockscanThread = new Thread() {
            public void run() {
                try {
                    mFmTransmitter.startBlockScan(MINSCANRANGE, MAXSCANRANGE);
                } catch (Exception e) {
                    showToast("Unable to start BlockScan between " + String.valueOf(MINSCANRANGE)
                            + "-" + String.valueOf(MAXSCANRANGE), Toast.LENGTH_LONG);
                    return;
                }
            }
        };
        blockscanThread.start();
    }

    /**
     * Helper function that formats and displays the current frequency.
     */
    private void displayFreq() {
        String a = Double.toString((double) mCurrentfrequency / 1000);
        if (mFmBand.getChannelOffset() == CHANNEL_OFFSET_50KHZ) {
            mFrequencyTextView.setText(String.format(a, "%.2f"));
        } else {
            mFrequencyTextView.setText(String.format(a, "%.1f"));
        }
    }

    /**
     * Sets up the button animation
     */
    private void setupAnimation() {
        ImageView transmitImage = (ImageView) findViewById(R.id.TransmitIcon);
        transmitImage.setBackgroundResource(R.drawable.anim);
        mTransmitAnimation = (AnimationDrawable) transmitImage.getBackground();
    }

    /**
     * Helper method to display toast
     */
    private void showToast(final String text, final int duration) {
        runOnUiThread(new Runnable() {
            public void run() {
                Toast.makeText(getApplicationContext(), text, duration).show();
            }
        });
    }

    /**
     * Sets up the button's onclick listeners
     */
    private void setupButtons() {
        mFrequencyTextView = (TextView) findViewById(R.id.FrequencyTextView);
        final Button tuneUp = (Button) findViewById(R.id.ScanUp);
        tuneUp.setOnClickListener(new OnClickListener() {

            public void onClick(View v) {
                try {
                    ((Button) findViewById(R.id.ScanUp)).setEnabled(false);
                    mFmTransmitter.setFrequency(mCurrentfrequency + mFrequencyIncrement);
                } catch (Exception e) {
                    showToast("Unable to scan up", Toast.LENGTH_LONG);
                    return;
                }
                mCurrentfrequency += mFrequencyIncrement;
                displayFreq();
                ((Button) findViewById(R.id.ScanUp)).setEnabled(true);
            }
        });
        final Button tuneDown = (Button) findViewById(R.id.ScanDown);
        tuneDown.setOnClickListener(new OnClickListener() {

            public void onClick(View v) {
                try {
                    ((Button) findViewById(R.id.ScanDown)).setEnabled(false);
                    mFmTransmitter.setFrequency(mCurrentfrequency - mFrequencyIncrement);
                } catch (Exception e) {
                    showToast("Unable to scan down", Toast.LENGTH_LONG);
                    return;
                }
                mCurrentfrequency -= mFrequencyIncrement;
                displayFreq();
                ((Button) findViewById(R.id.ScanDown)).setEnabled(true);
            }
        });
        final Button transmit = (Button) findViewById(R.id.Transmit);
        transmit.setBackgroundResource(R.drawable.transmitgobutton);
        transmit.setOnClickListener(new OnClickListener() {

            public void onClick(View v) {
                switch (mFmTransmitter.getState()) {

                    case FmTransmitter.STATE_IDLE:
                        try {
                            mFmTransmitter.startAsync(mFmBand);
                            showToast("Preparing radio", Toast.LENGTH_LONG);
                        } catch (Exception e) {
                            showToast("Unable to start radio", Toast.LENGTH_LONG);
                            return;
                        }

                        break;
                    case FmTransmitter.STATE_PAUSED:
                        try {
                            mFmTransmitter.resume();
                        } catch (Exception e) {
                            showToast("Unable to resume radio", Toast.LENGTH_LONG);
                            return;
                        }
                        mTransmitAnimation.start();
                        transmit.setBackgroundResource(R.drawable.transmitstopbutton);

                        break;
                    case FmTransmitter.STATE_STARTED:
                        try {
                            mFmTransmitter.pause();
                        } catch (Exception e) {
                            showToast("Unable to pause radio", Toast.LENGTH_LONG);
                            return;
                        }
                        mTransmitAnimation.stop();
                        transmit.setBackgroundResource(R.drawable.transmitgobutton);
                        break;
                    default:
                }
            }
        });
        final Button blockscan = (Button) findViewById(R.id.BlockScan);
        blockscan.setOnClickListener(new OnClickListener() {

            public void onClick(View v) {
                blockscan();
            }
        });
    }

    /**
     * Sets up the options menu when the menu button is push, dynamic population
     * of the station select menu
     */
    public boolean onPrepareOptionsMenu(Menu menu) {
        menu.clear();
        boolean result = super.onCreateOptionsMenu(menu);
        SubMenu subMenu = menu.addSubMenu(BASE_OPTION_MENU, FM_BAND, Menu.NONE,
                R.string.band_select);
        subMenu.setIcon(android.R.drawable.ic_menu_mapmode);
        // Populate the band selection menu
        subMenu.add(BAND_SELECTION_MENU, BAND_US, Menu.NONE, R.string.band_us);
        subMenu.add(BAND_SELECTION_MENU, BAND_EU, Menu.NONE, R.string.band_eu);
        subMenu.add(BAND_SELECTION_MENU, BAND_CHINA, Menu.NONE, R.string.band_ch);
        subMenu.add(BAND_SELECTION_MENU, BAND_JAPAN, Menu.NONE, R.string.band_ja);
        subMenu.setGroupCheckable(BAND_SELECTION_MENU, true, true);
        subMenu.getItem(mSelectedBand).setChecked(true);
        return result;
    }

    /**
     * React to a selection in the option menu
     */
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {

        if (item.getGroupId() == BAND_SELECTION_MENU) {
            switch (item.getItemId()) {
                case BAND_US:
                    mSelectedBand = FmBand.BAND_US;
                    item.setChecked(true);
                    break;
                case BAND_EU:
                    mSelectedBand = FmBand.BAND_EU;
                    item.setChecked(true);
                    break;
                case BAND_JAPAN:
                    mSelectedBand = FmBand.BAND_JAPAN;
                    item.setChecked(true);
                    break;
                case BAND_CHINA:
                    mSelectedBand = FmBand.BAND_CHINA;
                    item.setChecked(true);
                    break;
                default:
                    break;
            }
            mFmBand = new FmBand(mSelectedBand);
            try {
                mFmTransmitter.reset();
                ((Button) findViewById(R.id.Transmit))
                        .setBackgroundResource(R.drawable.transmitgobutton);
                ((TextView) findViewById(R.id.FrequencyTextView)).setText("----");
                mFrequencyIncrement = mFmBand.getChannelOffset() / 1000;
                mTransmitAnimation.stop();
            } catch (Exception e) {
                showToast("Unable to restart the FM Radio", Toast.LENGTH_LONG);
            }
        }
        return super.onOptionsItemSelected(item);
    }
}
