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

package com.example.android.toyvpn;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.VpnService;
import android.os.Bundle;
import android.widget.RadioButton;
import android.widget.TextView;
import android.widget.Toast;

import java.util.Arrays;
import java.util.Collections;
import java.util.Set;
import java.util.stream.Collectors;

public class ToyVpnClient extends Activity {
    public interface Prefs {
        String NAME = "connection";
        String SERVER_ADDRESS = "server.address";
        String SERVER_PORT = "server.port";
        String SHARED_SECRET = "shared.secret";
        String PROXY_HOSTNAME = "proxyhost";
        String PROXY_PORT = "proxyport";
        String ALLOW = "allow";
        String PACKAGES = "packages";
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.form);

        final TextView serverAddress = (TextView) findViewById(R.id.address);
        final TextView serverPort = (TextView) findViewById(R.id.port);
        final TextView sharedSecret = (TextView) findViewById(R.id.secret);
        final TextView proxyHost = (TextView) findViewById(R.id.proxyhost);
        final TextView proxyPort = (TextView) findViewById(R.id.proxyport);

        final RadioButton allowed = (RadioButton) findViewById(R.id.allowed);
        final TextView packages = (TextView) findViewById(R.id.packages);

        final SharedPreferences prefs = getSharedPreferences(Prefs.NAME, MODE_PRIVATE);
        serverAddress.setText(prefs.getString(Prefs.SERVER_ADDRESS, ""));
        serverPort.setText(prefs.getString(Prefs.SERVER_PORT, ""));
        sharedSecret.setText(prefs.getString(Prefs.SHARED_SECRET, ""));
        proxyHost.setText(prefs.getString(Prefs.PROXY_HOSTNAME, ""));
        proxyPort.setText(prefs.getString(Prefs.PROXY_PORT, ""));

        allowed.setChecked(prefs.getBoolean(Prefs.ALLOW, true));
        packages.setText(
                String.join(", ", prefs.getStringSet(Prefs.PACKAGES, Collections.emptySet())));

        findViewById(R.id.connect).setOnClickListener(v -> {
            if (!checkProxyConfigs(proxyHost.getText().toString(),
                    proxyPort.getText().toString())) {
                return;
            }

            final Set<String> packageSet =
                    Arrays.stream(packages.getText().toString().split(","))
                            .map(String::trim)
                            .filter(s -> !s.isEmpty())
                            .collect(Collectors.toSet());
            if (!checkPackages(packageSet) {
                return;
            }

            prefs.edit()
                    .putString(Prefs.SERVER_ADDRESS, serverAddress.getText().toString())
                    .putString(Prefs.SERVER_PORT, serverPort.getText().toString())
                    .putString(Prefs.SHARED_SECRET, sharedSecret.getText().toString())
                    .putString(Prefs.PROXY_HOSTNAME, proxyHost.getText().toString())
                    .putString(Prefs.PROXY_PORT, proxyPort.getText().toString())
                    .putBoolean(Prefs.ALLOW, allowed.isChecked())
                    .putStringSet(Prefs.PACKAGES, packageSet)
                    .commit();
            Intent intent = VpnService.prepare(ToyVpnClient.this);
            if (intent != null) {
                startActivityForResult(intent, 0);
            } else {
                onActivityResult(0, RESULT_OK, null);
            }
        });
        findViewById(R.id.disconnect).setOnClickListener(v -> {
            startService(getServiceIntent().setAction(ToyVpnService.ACTION_DISCONNECT));
        });
    }

    private boolean checkProxyConfigs(String proxyHost, String proxyPort) {
        final boolean hasProxyConfigs = !proxyHost.isEmpty() && !proxyPort.isEmpty();
        if (!hasProxyConfigs) {
            Toast.makeText(this, R.string.incomplete_proxy_settings, Toast.LENGTH_SHORT).show();
        }
        return hasProxyConfigs;
    }

    private boolean checkPackages(Set<String> packageNames) {
        final boolean hasPackageNames = packageNames.isEmpty() ||
                getPackageManager().getInstalledPackages(0).stream()
                        .map(pi -> pi.packageName)
                        .collect(Collectors.toSet())
                        .containsAll(packageNames);
        if (!hasPackageNames) {
            Toast.makeText(this, "Package names missing.", Toast.LENGTH_SHORT).show();
        }
        return hasPackageNames;
    }

    @Override
    protected void onActivityResult(int request, int result, Intent data) {
        if (result == RESULT_OK) {
            startService(getServiceIntent().setAction(ToyVpnService.ACTION_CONNECT));
        }
    }

    private Intent getServiceIntent() {
        return new Intent(this, ToyVpnService.class);
    }
}
