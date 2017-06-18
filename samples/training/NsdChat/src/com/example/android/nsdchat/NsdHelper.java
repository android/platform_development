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

package com.example.android.nsdchat;

import android.content.Context;
import android.net.nsd.NsdServiceInfo;
import android.net.nsd.NsdManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.provider.Settings;
import android.util.Log;

public class NsdHelper {

    Context mContext;
    Handler mUpdateHandler;

    NsdManager mNsdManager;
    NsdManager.ResolveListener mResolveListener;
    NsdManager.DiscoveryListener mDiscoveryListener;
    NsdManager.RegistrationListener mRegistrationListener;

    public static final String SERVICE_TYPE = "_http._tcp.";

    public static final String TAG = "NsdHelper";
    public static final String mServicePrefix = "NsdChat-";
    public String mServiceName;

    NsdServiceInfo mService;

    public NsdHelper(Context context, Handler updateHandler) {
        mContext = context;
        mUpdateHandler = updateHandler;
        mNsdManager = (NsdManager) context.getSystemService(Context.NSD_SERVICE);
    }

    private void statusUpdate(String msg) {
        Bundle messageBundle = new Bundle();
        messageBundle.putString(NsdChatActivity.REMOTE_MSG_TYPE, NsdChatActivity.REMOTE_MSG_TYPE_STATUS);
        messageBundle.putString(NsdChatActivity.REMOTE_MSG_STRING, msg);

        Message message = new Message();
        message.setData(messageBundle);
        mUpdateHandler.sendMessage(message);

    }

    public void initializeNsd() {
        initializeResolveListener();

        //mNsdManager.init(mContext.getMainLooper(), this);

    }

    public void initializeDiscoveryListener() {
        mDiscoveryListener = new NsdManager.DiscoveryListener() {

            @Override
            public void onDiscoveryStarted(String regType) {
                Log.d(TAG, "Service discovery started");
            }

            @Override
            public void onServiceFound(NsdServiceInfo service) {
                if (!service.getServiceType().equals(SERVICE_TYPE)) {
                    Log.d(TAG, "Discovered unknown service (ignoring): " + service);
                } else if (service.getServiceName().equals(mServiceName)) {
                    Log.d(TAG, "Discovered my own service (ignoring)");
                } else if (service.getServiceName().contains(mServicePrefix)) {
                    statusUpdate(mContext.getString(R.string.discovered_peer,
                            service.getServiceName()));
                    Log.d(TAG, "Discovered a chat peer: " + service);
                    mNsdManager.resolveService(service, mResolveListener);
                }
            }

            @Override
            public void onServiceLost(NsdServiceInfo service) {
                Log.e(TAG, "service lost: " + service);
                if (mService == service) {
                    mService = null;
                }
            }

            @Override
            public void onDiscoveryStopped(String serviceType) {
                Log.i(TAG, "Discovery stopped: " + serviceType);
            }

            @Override
            public void onStartDiscoveryFailed(String serviceType, int errorCode) {
                Log.e(TAG, "Discovery failed: Error code:" + errorCode);
            }

            @Override
            public void onStopDiscoveryFailed(String serviceType, int errorCode) {
                Log.e(TAG, "Discovery failed: Error code:" + errorCode);
            }
        };
    }

    public void initializeResolveListener() {
        mResolveListener = new NsdManager.ResolveListener() {

            @Override
            public void onResolveFailed(NsdServiceInfo serviceInfo, int errorCode) {
                Log.e(TAG, "Resolve failed: " + errorCode);
            }

            @Override
            public void onServiceResolved(NsdServiceInfo serviceInfo) {
                String name = serviceInfo.getServiceName();
                if (name.equals(mServiceName)) {
                    Log.d(TAG, "Resolved my own service (ignoring)");
                } else if (name.contains(mServicePrefix)) {
                    statusUpdate(mContext.getString(R.string.resolved_peer,
                            serviceInfo.getServiceName()));
                    Log.d(TAG, "Resolved a chat peer: " + serviceInfo);
                    mService = serviceInfo;
                } else {
                    Log.d(TAG, "Resolved unknown service (ignoring): " + serviceInfo);
                }
            }
        };
    }

    public void initializeRegistrationListener() {
        mRegistrationListener = new NsdManager.RegistrationListener() {

            @Override
            public void onServiceRegistered(NsdServiceInfo NsdServiceInfo) {
                mServiceName = NsdServiceInfo.getServiceName();
                statusUpdate(mContext.getString(R.string.registered_service, mServiceName));
                Log.d(TAG, "Service registered: " + mServiceName);
            }

            @Override
            public void onRegistrationFailed(NsdServiceInfo arg0, int arg1) {
                Log.d(TAG, "Service registration failed: " + arg1);
            }

            @Override
            public void onServiceUnregistered(NsdServiceInfo arg0) {
                Log.d(TAG, "Service unregistered: " + arg0.getServiceName());
            }

            @Override
            public void onUnregistrationFailed(NsdServiceInfo serviceInfo, int errorCode) {
                Log.d(TAG, "Service unregistration failed: " + errorCode);
            }

        };
    }

    public void registerService(int port) {
        tearDown();  // Cancel any previous registration request
        String client_id =
                Settings.Secure.getString(mContext.getContentResolver(), Settings.Secure.ANDROID_ID);

        mServiceName = mServicePrefix + client_id;

        initializeRegistrationListener();
        NsdServiceInfo serviceInfo  = new NsdServiceInfo();
        serviceInfo.setPort(port);
        serviceInfo.setServiceName(mServiceName);
        serviceInfo.setServiceType(SERVICE_TYPE);

        mNsdManager.registerService(
                serviceInfo, NsdManager.PROTOCOL_DNS_SD, mRegistrationListener);

    }

    public void discoverServices() {
        stopDiscovery();  // Cancel any existing discovery request
        initializeDiscoveryListener();
        mNsdManager.discoverServices(
                SERVICE_TYPE, NsdManager.PROTOCOL_DNS_SD, mDiscoveryListener);
    }

    public void stopDiscovery() {
        if (mDiscoveryListener != null) {
            try {
                mNsdManager.stopServiceDiscovery(mDiscoveryListener);
            } finally {
            }
            mDiscoveryListener = null;
        }
    }

    public NsdServiceInfo getChosenServiceInfo() {
        return mService;
    }

    public void tearDown() {
        if (mRegistrationListener != null) {
            try {
                mNsdManager.unregisterService(mRegistrationListener);
            } finally {
            }
            mRegistrationListener = null;
        }
    }
}
