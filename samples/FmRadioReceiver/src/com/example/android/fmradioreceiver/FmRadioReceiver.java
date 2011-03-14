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
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.example.android.fmradioreceiver;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.hardware.fm.FmBand;
import android.hardware.fm.FmReceiver;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.SubMenu;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import java.io.IOException;

public class FmRadioReceiver extends Activity {

    // The string to find in android logs
    private static final String LOG_TAG = "FM Radio Demo App";

    // The string to show that the station list is empty
    private static final String EMPTY_STATION_LIST = "No stations available";

    // The 50kHz channel offset
    private static final int CHANNEL_OFFSET_50KHZ = 50;

    // The base menu identifier
    private static final int BASE_OPTION_MENU = 0;

    // The band menu identifier
    private static final int BAND_SELECTION_MENU = 1;

    // The station menu identifier
    private static final int STATION_SELECTION_MENU = 2;

    // Handle to the Media Player that plays the audio from the selected station
    private MediaPlayer mMediaPlayer;

    // The scan listener that receives the return values from the scans
    private FmReceiver.OnScanListener mReceiverScanListener;

    // The listener that receives the RDS data from the current channel
    private FmReceiver.OnRDSDataFoundListener mReceiverRdsDataFoundListener;

    // The started listener is activated when the radio has started
    private FmReceiver.OnStartedListener mReceiverStartedListener;

    // Displays the currently tuned frequency
    private TextView mFrequencyTextView;

    // Displays the current station name if there is adequate RDS data
    private TextView mStationNameTextView;

    // Handle to the FM radio Band object
    private FmBand mFmBand;

    // Handle to the FM radio receiver object
    private FmReceiver mFmReceiver;

    // Indicates if we are in the initialization sequence
    private boolean mInit = true;

    // Indicates that we are restarting the app
    private boolean mRestart = false;

    // Protects the MediaPlayer and FmReceiver against rapid muting causing
    // errors
    private boolean mPauseMutex = false;

    // Array of the available stations in MHz
    private ArrayAdapter<CharSequence> mMenuAdapter;

    // The name of the storage string
    public static final String PREFS_NAME = "FMRadioPrefsFile";

    // The menu items
    public static final int FM_BAND = Menu.FIRST;

    public static final int BAND_US = Menu.FIRST + 1;

    public static final int BAND_EU = Menu.FIRST + 2;

    public static final int BAND_JAPAN = Menu.FIRST + 3;

    public static final int BAND_CHINA = Menu.FIRST + 4;

    public static final int STATION_SELECT = Menu.FIRST + 5;

    public static final int STATION_SELECT_MENU_ITEMS = STATION_SELECT + 1;

    // The currently selected FM Radio band
    private int mSelectedBand;

    /**
     * Required method from parent class
     *
     * @param icicle - The previous instance of this app
     */
    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        setContentView(R.layout.main);
        mFmReceiver = (FmReceiver) getSystemService(Context.RADIO_FM_RECEIVER_SERVICE);
        SharedPreferences settings = getSharedPreferences(PREFS_NAME, 0);
        mSelectedBand = settings.getInt("selectedBand", 1);
        mFmBand = new FmBand(mSelectedBand);
        setupButtons();
    }

    /**
     * Starts up the listeners and the FM radio if it isn't already active
     */
    @Override
    protected void onStart() {
        super.onStart();
        mReceiverScanListener = new android.hardware.fm.FmReceiver.OnScanListener() {

            // FullScan results
            public void onFullScan(int[] frequency, int[] signalStrength, boolean aborted) {
                ((Button) findViewById(R.id.FullScan)).setEnabled(true);
                showToast("Fullscan complete", Toast.LENGTH_LONG);
                mMenuAdapter.clear();
                if (frequency.length == 0) {
                    mMenuAdapter.add(EMPTY_STATION_LIST);
                    return;
                }
                for (int i = 0; i < frequency.length; i++) {
                    String a = Double.toString((double) frequency[i] / 1000);
                    if (mFmBand.getChannelOffset() == CHANNEL_OFFSET_50KHZ) {
                        a = String.format(a, "%.2f");
                    } else {
                        a = String.format(a, "%.1f");
                    }
                    mMenuAdapter.add(a);
                }
                if (mInit) {
                    mInit = false;
                    try {
                        mFmReceiver.setFrequency(frequency[0]);
                        mFrequencyTextView.setText(mMenuAdapter.getItem(0).toString());
                    } catch (IOException e) {
                        showToast("Unable to set the receiver's frequency", Toast.LENGTH_LONG);
                    } catch (IllegalArgumentException e) {
                        showToast("Unable to set the receiver's frequency", Toast.LENGTH_LONG);
                    }
                }
            }

            // Returns the new frequency.
            public void onScan(int tunedFrequency, int signalStrength, boolean aborted) {
                String a = Double.toString((double) tunedFrequency / 1000);
                if (mFmBand.getChannelOffset() == CHANNEL_OFFSET_50KHZ) {
                    mFrequencyTextView.setText(String.format(a, "%.2f"));
                } else {
                    mFrequencyTextView.setText(String.format(a, "%.1f"));
                }
                ((Button) findViewById(R.id.ScanUp)).setEnabled(true);
                ((Button) findViewById(R.id.ScanDown)).setEnabled(true);
            }
        };
        mReceiverRdsDataFoundListener = new android.hardware.fm.FmReceiver.OnRDSDataFoundListener() {

            // Receives the current frequency's RDS Data
            public void onRDSDataFound(Bundle rdsData, int frequency) {
                if (rdsData.containsKey("PSN")) {
                    mStationNameTextView.setText(rdsData.getString("PSN"));
                }
            }
        };

        mReceiverStartedListener = new android.hardware.fm.FmReceiver.OnStartedListener() {

            public void onStarted() {
                // Activate all the buttons
                ((Button) findViewById(R.id.ScanUp)).setEnabled(true);
                ((Button) findViewById(R.id.ScanDown)).setEnabled(true);
                ((Button) findViewById(R.id.Pause)).setEnabled(true);
                ((Button) findViewById(R.id.FullScan)).setEnabled(true);
                initialBandscan();
                startAudio();
            }
        };

        mFmReceiver.addOnScanListener(mReceiverScanListener);
        mFmReceiver.addOnRDSDataFoundListener(mReceiverRdsDataFoundListener);
        mFmReceiver.addOnStartedListener(mReceiverStartedListener);

        if (!mRestart) {
            turnRadioOn();
        }
        mRestart = false;
    }

    /**
     * Stops the FM Radio listeners
     */
    @Override
    protected void onRestart() {
        super.onRestart();
        mRestart = true;
    }

    /**
     * Stops the FM Radio listeners
     */
    @Override
    protected void onStop() {
        super.onStop();

        if (mFmReceiver != null) {
            mFmReceiver.removeOnScanListener(mReceiverScanListener);
            mFmReceiver.removeOnRDSDataFoundListener(mReceiverRdsDataFoundListener);
            mFmReceiver.removeOnStartedListener(mReceiverStartedListener);
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
            mFmReceiver.reset();
        } catch (IOException e) {
            Log.e(LOG_TAG, "Unable to reset correctly", e);
        }
        if (mMediaPlayer != null) {
            mMediaPlayer.release();
            mMediaPlayer = null;
        }
    }

    /**
     * Starts the initial bandscan in it's own thread
     */
    private void initialBandscan() {
        Thread bandscanThread = new Thread() {
            public void run() {
                try {
                    mFmReceiver.startFullScan();
                } catch (IllegalStateException e) {
                    showToast("Unable to start the scan", Toast.LENGTH_LONG);
                    return;
                }
            }
        };
        bandscanThread.start();
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
     * Starts the FM receiver and makes the buttons appear inactive
     */
    private void turnRadioOn() {

        try {
            mFmReceiver.startAsync(mFmBand);
            // Darken the the buttons
            ((Button) findViewById(R.id.ScanUp)).setEnabled(false);
            ((Button) findViewById(R.id.ScanDown)).setEnabled(false);
            ((Button) findViewById(R.id.Pause)).setEnabled(false);
            ((Button) findViewById(R.id.FullScan)).setEnabled(false);
            showToast("Scanning initial stations", Toast.LENGTH_LONG);
        } catch (IOException e) {
            showToast("Unable to start the radio receiver.", Toast.LENGTH_LONG);
        } catch (IllegalStateException e) {
            showToast("Unable to start the radio receiver.", Toast.LENGTH_LONG);
        }
    }

    /**
     * Starts the FM receiver and makes the buttons appear inactive
     */
    private void startAudio() {

        mMediaPlayer = new MediaPlayer();
        try {
            mMediaPlayer.setDataSource("fmradio://rx");
            mMediaPlayer.prepare();
            mMediaPlayer.start();
        } catch (IOException e) {
            showToast("Unable to start the media player", Toast.LENGTH_LONG);
        }
    }

    /**
     * Sets up the buttons and their listeners
     */
    private void setupButtons() {

        mMenuAdapter = new ArrayAdapter<CharSequence>(this, android.R.layout.simple_spinner_item);
        mMenuAdapter.setDropDownViewResource(android.R.layout.simple_list_item_single_choice);
        mMenuAdapter.add("No stations available");
        mFrequencyTextView = (TextView) findViewById(R.id.FrequencyTextView);
        mStationNameTextView = (TextView) findViewById(R.id.PSNTextView);

        final Button scanUp = (Button) findViewById(R.id.ScanUp);
        scanUp.setOnClickListener(new OnClickListener() {

            public void onClick(View v) {
                try {
                    mFmReceiver.scanUp();
                } catch (IllegalStateException e) {
                    showToast("Unable to ScanUp", Toast.LENGTH_LONG);
                    return;
                }
                scanUp.setEnabled(false);
            }
        });
        final Button scanDown = (Button) findViewById(R.id.ScanDown);
        scanDown.setOnClickListener(new OnClickListener() {

            public void onClick(View v) {
                try {
                    mFmReceiver.scanDown();
                } catch (IllegalStateException e) {
                    showToast("Unable to ScanDown", Toast.LENGTH_LONG);
                    return;
                }
                scanDown.setEnabled(false);
            }
        });
        final Button pause = (Button) findViewById(R.id.Pause);
        pause.setOnClickListener(new OnClickListener() {

            public void onClick(View v) {
                if (mFmReceiver.getState() == FmReceiver.STATE_PAUSED && mPauseMutex != true) {
                    try {
                        mPauseMutex = true;
                        mFmReceiver.resume();
                        mMediaPlayer.start();
                        pause.setBackgroundResource(R.drawable.pausebutton);
                    } catch (IOException e) {
                        showToast("Unable to resume", Toast.LENGTH_LONG);
                    } catch (IllegalStateException e) {
                        showToast("Unable to resume", Toast.LENGTH_LONG);
                    }
                    mPauseMutex = false;
                } else if (mFmReceiver.getState() == FmReceiver.STATE_STARTED
                        && mPauseMutex != true) {
                    try {
                        mPauseMutex = true;
                        mMediaPlayer.pause();
                        mFmReceiver.pause();
                        pause.setBackgroundResource(R.drawable.playbutton);
                    } catch (IOException e) {
                        showToast("Unable to pause", Toast.LENGTH_LONG);
                    } catch (IllegalStateException e) {
                        showToast("Unable to pause", Toast.LENGTH_LONG);
                    }
                    mPauseMutex = false;
                } else if (mPauseMutex) {
                    showToast("MediaPlayer busy. Please wait and try again.", Toast.LENGTH_LONG);
                } else {
                    Log.i(LOG_TAG, "No action: incorrect state - " + mFmReceiver.getState());
                }
            }
        });
        final Button fullScan = (Button) findViewById(R.id.FullScan);
        fullScan.setOnClickListener(new OnClickListener() {

            public void onClick(View v) {
                try {
                    fullScan.setEnabled(false);
                    showToast("Scanning for stations", Toast.LENGTH_LONG);
                    mFmReceiver.startFullScan();
                } catch (IllegalStateException e) {
                    showToast("Unable to start the scan", Toast.LENGTH_LONG);
                }
            }
        });
    }

    /**
     * Sets up the options menu when the menu button is pushed, dynamic
     * population of the station select menu
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
        subMenu.add(BAND_SELECTION_MENU, BAND_JAPAN, Menu.NONE, R.string.band_ja);
        subMenu.add(BAND_SELECTION_MENU, BAND_CHINA, Menu.NONE, R.string.band_ch);
        subMenu.setGroupCheckable(BAND_SELECTION_MENU, true, true);
        subMenu.getItem(mSelectedBand).setChecked(true);

        subMenu = menu.addSubMenu(BASE_OPTION_MENU, STATION_SELECT, Menu.NONE,
                R.string.station_select);
        subMenu.setIcon(android.R.drawable.ic_menu_agenda);

        // Dynamically populate the station select menu each time the option
        // button is pushed
        if (mMenuAdapter.isEmpty()) {
            subMenu.setGroupEnabled(STATION_SELECTION_MENU, false);
        } else {
            subMenu.setGroupEnabled(STATION_SELECTION_MENU, true);
            for (int i = 0; i < mMenuAdapter.getCount(); i++) {
                subMenu.add(STATION_SELECTION_MENU, STATION_SELECT_MENU_ITEMS + i, Menu.NONE,
                        mMenuAdapter.getItem(i));
            }
            subMenu.setGroupCheckable(STATION_SELECTION_MENU, true, true);
        }
        return result;
    }

    public int getSelectStationMenuItem(MenuItem item) {
        return item.getItemId() - STATION_SELECT_MENU_ITEMS;
    }

    /**
     * React to a selection in the option menu
     */
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {

        switch (item.getGroupId()) {
            case BAND_SELECTION_MENU:
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
                    mFmReceiver.reset();
                } catch (IOException e) {
                    showToast("Unable to restart the FM Radio", Toast.LENGTH_LONG);
                }
                if (mMediaPlayer != null) {
                    mMediaPlayer.release();
                    mMediaPlayer = null;
                }
                turnRadioOn();
                break;
            case STATION_SELECTION_MENU:
                try {
                    if (!mMenuAdapter.getItem(getSelectStationMenuItem(item)).toString().matches(
                            EMPTY_STATION_LIST)) {
                        mFmReceiver.setFrequency((int) (Double.valueOf(mMenuAdapter.getItem(
                                getSelectStationMenuItem(item)).toString()) * 1000));
                        mFrequencyTextView.setText(mMenuAdapter.getItem(
                                getSelectStationMenuItem(item)).toString());
                    }
                } catch (IOException e) {
                    showToast("Unable to set the frequency", Toast.LENGTH_LONG);
                } catch (IllegalStateException e) {
                    showToast("Unable to set the frequency", Toast.LENGTH_LONG);
                } catch (IllegalArgumentException e) {
                    showToast("Unable to set the frequency", Toast.LENGTH_LONG);
                }

                break;
            default:
                break;
        }
        return super.onOptionsItemSelected(item);
    }
}
