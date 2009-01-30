/*
 * Copyright (C) 2008 The Android Open Source Project
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

package com.android.gpstest;

/* Maps support is disabled

import android.location.GpsStatus;
import android.location.Location;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;

import com.google.android.maps.MapActivity;
import com.google.android.maps.MapView;
import com.google.android.maps.MyLocationOverlay;

public class GpsMapActivity extends MapActivity 
        implements GpsTestActivity.SubActivity, View.OnClickListener {

    private static final String TAG = "GpsMapActivity";
    private MapView mMapView;
    private MyLocationOverlay mMyLocationOverlay;
    private Button mZoomInButton;
    private Button mZoomOutButton;
    private Button mModeButton;
    private boolean mSatellite = false;
    private int mZoomLevel;
    private boolean mGotFix;

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        setContentView(R.layout.gps_map);
        mMapView = (MapView)findViewById(R.id.map_view);
        mMyLocationOverlay = new MyLocationOverlay(this, mMapView);
        mMapView.getOverlays().add(mMyLocationOverlay);

        mZoomInButton = (Button)findViewById(R.id.zoom_in);
        mZoomInButton.setOnClickListener(this);
        mZoomOutButton = (Button)findViewById(R.id.zoom_out);
        mZoomOutButton.setOnClickListener(this);
        mModeButton = (Button)findViewById(R.id.mode);
        mModeButton.setOnClickListener(this);
        
        mZoomLevel = mMapView.getZoomLevel();
        mSatellite = mMapView.isSatellite();
        mModeButton.setText(mSatellite ? R.string.mode_map : R.string.mode_satellite);
 
        GpsTestActivity.getInstance().addSubActivity(this);
    }

    public void onClick(View v) {
        GpsTestActivity gta = GpsTestActivity.getInstance();
        
        if (v == mZoomInButton) {
            zoomIn();
        } else if (v == mZoomOutButton) {
            zoomOut();
        } else if (v == mModeButton) {
            toggleSatellite();
        }
    }

    private void zoomIn() {
        if (mZoomLevel < mMapView.getMaxZoomLevel()) {
            mZoomLevel++;
            mMapView.getController().setZoom(mZoomLevel);
        }
    }

    private void zoomOut() {
        if (mZoomLevel > 0) {
            mZoomLevel--;
            mMapView.getController().setZoom(mZoomLevel);
        }
    }

    private void toggleSatellite() {
        mSatellite = !mSatellite;
        mMapView.setSatellite(mSatellite);
        mModeButton.setText(mSatellite ? R.string.mode_map : R.string.mode_satellite);
    }

    public void gpsStart() {
        mMyLocationOverlay.enableMyLocation();
        mGotFix = false;
    }

    public void gpsStop() {
        mMyLocationOverlay.disableMyLocation();
    }

    public void onLocationChanged(Location loc) {
        mMyLocationOverlay.enableMyLocation();
        if (!mGotFix && mMapView.getZoomLevel() < mMapView.getMaxZoomLevel() / 2) {
            // zoom in if we are backed out too much
            mZoomLevel = 20;
            mMapView.getController().setZoom(mZoomLevel);
        }
        mGotFix = true;
        mMyLocationOverlay.runOnFirstFix(new Runnable() { public void run() {
            mMapView.getController().animateTo(mMyLocationOverlay.getMyLocation());
        }});
    }

    public void onStatusChanged(String provider, int status, Bundle extras) {
    }

    public void onProviderEnabled(String provider) {
    }

    public void onProviderDisabled(String provider) {
    }

    public void onGpsStatusChanged(int event, GpsStatus status) {
    }

   @Override
    protected boolean isRouteDisplayed() {
        return false;
    }
}

Maps support is disabled
*/
