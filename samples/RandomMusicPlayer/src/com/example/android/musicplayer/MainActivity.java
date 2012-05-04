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

package com.example.android.musicplayer;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.EditText;
import android.widget.ImageButton;

/**
 * Main activity: shows media player buttons. This activity shows the media player buttons and lets
 * the user click them. No media handling is done here -- everything is done by passing Intents to
 * our {@link MusicService}.
 * */
public class MainActivity extends Activity implements OnClickListener {
    /**
     * The URL we suggest as default when adding by URL. This is just so that the user doesn't have
     * to find an URL to test this sample.
     */
    final String SUGGESTED_URL = "http://www.archive.org/download/AcousticMalitia/02_Synthesize.mp3";

    ImageButton mPlayButton;
    ImageButton mPauseButton;
    ImageButton mSkipButton;
    ImageButton mRewindButton;
    ImageButton mStopButton;
    ImageButton mEjectButton;

    View mSelectedButton = null;

    /**
     * Called when the activity is first created. Here, we simply set the event listeners and start
     * the background service ({@link MusicService}) that will handle the actual media playback.
     */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        mPlayButton = (ImageButton) findViewById(R.id.playbutton);
        mPauseButton = (ImageButton) findViewById(R.id.pausebutton);
        mSkipButton = (ImageButton) findViewById(R.id.skipbutton);
        mRewindButton = (ImageButton) findViewById(R.id.rewindbutton);
        mStopButton = (ImageButton) findViewById(R.id.stopbutton);
        mEjectButton = (ImageButton) findViewById(R.id.ejectbutton);

        mPlayButton.setOnClickListener(this);
        mPauseButton.setOnClickListener(this);
        mSkipButton.setOnClickListener(this);
        mRewindButton.setOnClickListener(this);
        mStopButton.setOnClickListener(this);
        mEjectButton.setOnClickListener(this);
    }

    public void onClick(View target) {
        // Send the correct intent to the MusicService, according to the button that was clicked
        switch (target.getId()) {
            case R.id.playbutton:
                updateSelectedButtonState(target);
                startService(new Intent(MusicService.ACTION_PLAY));
                break;
            case R.id.pausebutton:
                updateSelectedButtonState(target);
                startService(new Intent(MusicService.ACTION_PAUSE));
                break;
            case R.id.skipbutton:
                startService(new Intent(MusicService.ACTION_SKIP));
                break;
            case R.id.rewindbutton:
                startService(new Intent(MusicService.ACTION_REWIND));
                break;
            case R.id.stopbutton:
                updateSelectedButtonState(target);
                startService(new Intent(MusicService.ACTION_STOP));
                break;
            case R.id.ejectbutton:
                showUrlDialog();
            default:
                Log.i(this.getClass().getName(), "Pressed unknown button: " + target);
        }
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE:
            case KeyEvent.KEYCODE_HEADSETHOOK:
                startService(new Intent(MusicService.ACTION_TOGGLE_PLAYBACK));
                return true;
            case KeyEvent.KEYCODE_MEDIA_PLAY:
                updateSelectedButtonState(mPlayButton);
                startService(new Intent(MusicService.ACTION_PLAY));
                return true;
            case KeyEvent.KEYCODE_MEDIA_PAUSE:
                updateSelectedButtonState(mPauseButton);
                startService(new Intent(MusicService.ACTION_PAUSE));
                return true;
            case KeyEvent.KEYCODE_MEDIA_FAST_FORWARD:
                startService(new Intent(MusicService.ACTION_SKIP));
                return true;
            case KeyEvent.KEYCODE_MEDIA_REWIND:
                startService(new Intent(MusicService.ACTION_REWIND));
                return true;
            case KeyEvent.KEYCODE_MEDIA_STOP:
                updateSelectedButtonState(mStopButton);
                startService(new Intent(MusicService.ACTION_STOP));
                return true;
            case KeyEvent.KEYCODE_MEDIA_EJECT:
                showUrlDialog();
                return true;
            default:
                return super.onKeyDown(keyCode, event);
        }
    }

    /**
     * Shows an alert dialog where the user can input a URL. After showing the dialog, if the user
     * confirms, sends the appropriate intent to the {@link MusicService} to cause that URL to be
     * played.
     */
    void showUrlDialog() {
        AlertDialog.Builder alertBuilder = new AlertDialog.Builder(this);
        alertBuilder.setTitle(getString(R.string.url_dialog_title));
        alertBuilder.setMessage(getString(R.string.url_dialog_msg));
        final EditText input = new EditText(this);
        alertBuilder.setView(input);

        input.setText(SUGGESTED_URL);

        alertBuilder.setPositiveButton(getString(R.string.url_dialog_ok),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dlg, int whichButton) {
                        updateSelectedButtonState(mPlayButton);
                        // Send an intent with the URL of the song to play. This is expected by
                        // MusicService.
                        Intent i = new Intent(MusicService.ACTION_URL);
                        Uri uri = Uri.parse(input.getText().toString());
                        i.setData(uri);
                        startService(i);
                    }
                });
        alertBuilder.setNegativeButton(getString(R.string.url_dialog_cancel),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dlg, int whichButton) {
                    }
                });

        alertBuilder.show();
    }

    /**
     * It de-selects the previously selected Button (if any) and updates the reference to point to
     * the newly selected Button.
     */
    private void updateSelectedButtonState(View newSelectedButton) {
        if (mSelectedButton != newSelectedButton) {
            if (mSelectedButton != null) {
                mSelectedButton.setSelected(false);
            }
            mSelectedButton = newSelectedButton;
            mSelectedButton.setSelected(true);
        }
    }
}
