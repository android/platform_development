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

import android.app.TabActivity;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.location.GpsSatellite;
import android.location.GpsStatus;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.location.LocationProvider;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.widget.TabHost;

import java.util.ArrayList;

public class GpsTestActivity extends TabActivity 
        implements LocationListener, GpsStatus.Listener {
    private static final String TAG = "GpsTestActivity";

    private LocationManager mService;
    private LocationProvider mProvider;
    private GpsStatus mStatus;
    private ArrayList<SubActivity> mSubActivities = new ArrayList<SubActivity>();
    boolean mNavigating;
    boolean mStartOnResume = true;  // run at startup by default
    private Location mLastLocation;
    
    private Menu mMenu;
    private MenuItem mStartItem;
    private MenuItem mSendLocationItem;

    private static GpsTestActivity sInstance;
    
    interface SubActivity extends LocationListener {
        public void gpsStart();
        public void gpsStop();
        public void onGpsStatusChanged(int event, GpsStatus status);
    }
    
    static GpsTestActivity getInstance() {
        return sInstance;
    }
    
    void addSubActivity(SubActivity activity) {
        mSubActivities.add(activity);
    }

    void gpsStart() {
        if (!mNavigating) {
            mService.requestLocationUpdates(mProvider.getName(), 0L, 0.0f, this);
            mStartOnResume = true;
        }
        for (SubActivity activity : mSubActivities) {
            activity.gpsStart();
        }
    }

    void gpsStop() {
        if (mNavigating) {
            mService.removeUpdates(this);
            mStartOnResume = false;
        }
        for (SubActivity activity : mSubActivities) {
            activity.gpsStop();
        }
    }
    
    void deleteAidingData() {
        mService.sendExtraCommand(LocationManager.GPS_PROVIDER, "delete_aiding_data", null);
    }

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        sInstance = this;
        
        mService = (LocationManager)getSystemService(Context.LOCATION_SERVICE);
        mProvider = mService.getProvider(LocationManager.GPS_PROVIDER);
        if (mProvider == null) {
            // FIXME - fail gracefully here
            Log.e(TAG, "Unable to get GPS_PROVIDER");
        }
        
        final TabHost tabHost = getTabHost();
        final Resources res = getResources();
        
        tabHost.addTab(tabHost.newTabSpec("tab1")
                .setIndicator(res.getString(R.string.gps_status_tab))
                .setContent(new Intent(this, GpsStatusActivity.class)));

/* Maps support is disabled
        tabHost.addTab(tabHost.newTabSpec("tab2")
                .setIndicator(res.getString(R.string.gps_map_tab))
                .setContent(new Intent(this, GpsMapActivity.class)));
*/

        tabHost.addTab(tabHost.newTabSpec("tab3")
                .setIndicator(res.getString(R.string.gps_sky_tab))
                .setContent(new Intent(this, GpsSkyActivity.class)));
    }
    
    @Override
    public void onPause() {
        super.onPause();
        if (mNavigating) {
            gpsStop();
            mStartOnResume = true;
        }
        mService.removeGpsStatusListener(this);
    }

    @Override
    public void onResume() {
        super.onResume();
        mService.addGpsStatusListener(this);
        if (mStartOnResume) {
            gpsStart();
        }
    }

    @Override
    protected void onDestroy() {
        mService.removeUpdates(this);
        super.onDestroy();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        mMenu = menu;
        
        // Inflate the currently selected menu XML resource.
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.gps_menu, menu);

        mStartItem = menu.findItem(R.id.gps_start);
        if (mNavigating) {
            mStartItem.setTitle(R.string.gps_stop);
        }

        mSendLocationItem = menu.findItem(R.id.send_location);
        mSendLocationItem.setEnabled(mLastLocation != null);

        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            // For "Title only": Examples of matching an ID with one assigned in
            //                   the XML
            case R.id.gps_start:
                if (mNavigating) {
                    gpsStop();
                } else {
                    gpsStart();
                }
                return true;

            case R.id.delete_aiding_data:
                deleteAidingData();
                return true;

            case R.id.send_location:
                sendLocation();
                return true;
        }
        
        return false;
    }

    public void onLocationChanged(Location location) {
        mLastLocation = location;
        if (mSendLocationItem != null) {
            mSendLocationItem.setEnabled(true);
        }

        for (SubActivity activity : mSubActivities) {
            activity.onLocationChanged(location);
        }
    }

    public void onStatusChanged(String provider, int status, Bundle extras) {
        for (SubActivity activity : mSubActivities) {
            activity.onStatusChanged(provider, status, extras);
        }
    }

    public void onProviderEnabled(String provider) {
        for (SubActivity activity : mSubActivities) {
            activity.onProviderEnabled(provider);
        }
    }

    public void onProviderDisabled(String provider) {
        for (SubActivity activity : mSubActivities) {
            activity.onProviderDisabled(provider);
        }
    }

    public void onGpsStatusChanged(int event) {
        mStatus = mService.getGpsStatus(mStatus);
        switch (event) {
            case GpsStatus.GPS_EVENT_STARTED:
                mNavigating = true;
                if (mStartItem != null) {
                    mStartItem.setTitle(R.string.gps_stop);
                }
                break;

            case GpsStatus.GPS_EVENT_STOPPED:
                mNavigating = false;
                if (mStartItem != null) {
                    mStartItem.setTitle(R.string.gps_start);
                }
                break;
        }

        for (SubActivity activity : mSubActivities) {
            activity.onGpsStatusChanged(event, mStatus);
        }
    }

    private void sendLocation() {
        if (mLastLocation != null) {
            Intent intent = new Intent(Intent.ACTION_SENDTO,
                                Uri.fromParts("mailto", "", null));
            String location = "http://maps.google.com/maps?geocode=&q=" + 
                    Double.toString(mLastLocation.getLatitude()) + "," +
                    Double.toString(mLastLocation.getLongitude());
            intent.putExtra(Intent.EXTRA_TEXT, location);
            startActivity(intent);
        }
    }
}
