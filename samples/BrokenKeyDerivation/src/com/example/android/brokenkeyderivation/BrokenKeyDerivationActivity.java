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

package com.example.android.brokenkeyderivation;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.view.WindowManager;
import android.widget.EditText;

import java.nio.charset.StandardCharsets;
import java.security.SecureRandom;
import java.security.spec.KeySpec;

import javax.crypto.Cipher;
import javax.crypto.SecretKey;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.PBEKeySpec;
import javax.crypto.spec.SecretKeySpec;


/**
 * Example showing how to decrypt data that was encrypted using SHA1PRNG
 *
 * The Crypto provider providing the SHA1PRNG algorithm for random number
 * generation was deprecated in SDK 24.
 *
 * There was an usual anti-pattern of using that algorithm to derive keys.

 * This example provides a helper class ({@link InsecureSHA1PRNGKeyDerivator} and shows how to treat
 * data that was encrypted in the incorrect way and re-encrypt it in a proper way,
 * by using a KeyDerivationFunction.
 */
public class BrokenKeyDerivationActivity extends Activity {
    /**
     * Method used to derive an *insecure* key by emulating the SHA1PRNG algorithm from the
     * deprecated Crypto provider.
     *
     * Do not use it to encrypt new data, just to decrypt encrypted data that would be unrecoverable
     * otherwise.
     */
    private static SecretKey deriveKeyInsecurely(String password, int keySizeInBytes) {
        byte[] passwordBytes = password.getBytes(StandardCharsets.US_ASCII);
        return new SecretKeySpec(
                InsecureSHA1PRNGKeyDerivator.deriveInsecureKey(passwordBytes, keySizeInBytes),
                "AES");
    }

    /**
     * Example use of a key derivation function
     */
    private SecretKey deriveKeySecurely(String password, int keySizeInBytes) {
        if (retrieveSalt() == null) {
            // When first creating the key, obtain a salt with this:
            SecureRandom random = new SecureRandom();
            // Salt should be the same size as the key.
            byte[] salt = new byte[keySizeInBytes];
            random.nextBytes(salt);
            // Salt can be stored on disk.
            storeSalt(salt);
        }

        // Use this to derive the key from the password:
        KeySpec keySpec = new PBEKeySpec(password.toCharArray(), retrieveSalt(),
                100 /* iterationCount */, keySizeInBytes * 8 /* key size in bits */);
        try {
            SecretKeyFactory keyFactory = SecretKeyFactory.getInstance("PBKDF2WithHmacSHA1");
            byte[] keyBytes = keyFactory.generateSecret(keySpec).getEncoded();
            return new SecretKeySpec(keyBytes, "AES");
        } catch (Exception e) {
            throw new RuntimeException("Deal with exceptions properly!", e);
        }
    }

    /**
     * If data is stored with an insecure key, reencrypt with a secure key.
     */
    private String retrieveData(String password) {
        final int keySize = 32;
        String decryptedString;

        if (isDataStoredWithInsecureKey()) {
            SecretKey insecureKey = deriveKeyInsecurely(password, keySize);
            byte[] decryptedData = decryptData(retrieveEncryptedData(), retrieveIv(), insecureKey);
            SecretKey secureKey = deriveKeySecurely(password, keySize);
            storeDataEncryptedWithSecureKey(encryptData(decryptedData, retrieveIv(), secureKey));
            decryptedString = "Warning: data was encrypted with insecure key\n"
                    + new String(decryptedData, StandardCharsets.US_ASCII);
        } else {
            SecretKey secureKey = deriveKeySecurely(password, keySize);
            byte[] decryptedData = decryptData(retrieveEncryptedData(), retrieveIv(), secureKey);
            decryptedString = "Great!: data was encrypted with secure key\n"
                    + new String(decryptedData, StandardCharsets.US_ASCII);
        }
        return decryptedString;
    }

    /**
     * Called when the activity is first created.
     */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Set the layout for this activity.  You can find it
        // in res/layout/brokenkeyderivation_activity.xml
        View view = getLayoutInflater().inflate(R.layout.brokenkeyderivation_activity, null);
        setContentView(view);

        // Find the text editor view inside the layout.
        EditText mEditor = (EditText) findViewById(R.id.text);

        String password = "unguessable";
        String firstResult = retrieveData(password);
        String secondResult = retrieveData(password);

        mEditor.setText("First result: " + firstResult + "\nSecond result: " + secondResult);

    }

    private static byte[] encryptOrDecrypt(byte[] data, SecretKey key, byte[] iv, boolean isEncrypt) {
        try {
            Cipher cipher = Cipher.getInstance("AES/CBC/PKCS7PADDING");
            cipher.init(isEncrypt ? Cipher.ENCRYPT_MODE : Cipher.DECRYPT_MODE, key,
                    new IvParameterSpec(iv));
            return cipher.doFinal(data);
        } catch (Exception e) {
            throw new RuntimeException("This is unconceivable!", e);
        }
    }

    /**
     * Everything below this is a succession of mocks and getters that would rarely interest
     * someone on Earth.
     */

    private static byte[] encryptData(byte[] data, byte[] iv, SecretKey key) {
        return encryptOrDecrypt(data, key, iv, true);
    }

    private static byte[] decryptData(byte[] data, byte[] iv, SecretKey key) {
        return encryptOrDecrypt(data, key, iv, false);
    }


    private boolean isDataStoredWithInsecureKey = true;

    private boolean isDataStoredWithInsecureKey() {
        return isDataStoredWithInsecureKey;
    }

    private byte[] iv = null;

    private byte[] retrieveIv() {
        // Mock iv. The data that you have stored should ideally have been encrypted with an
        // iv obtained from SecureRandom#next, same as with the salt.
        if (iv == null) {
            iv = new byte[16];
            new SecureRandom().nextBytes(iv);
        }
        return iv;
    }

    private byte[] salt = null;

    private void storeSalt(byte[] salt) {
        this.salt = salt;
    }

    private byte[] retrieveSalt() {
        return salt;
    }

    // Mock initial data.
    private byte[] encryptedData = encryptData(
            "I hope it helped!".getBytes(), retrieveIv(), deriveKeyInsecurely("unguessable", 32));

    private byte[] retrieveEncryptedData() {
        return encryptedData;
    }

    private void storeDataEncryptedWithSecureKey(byte[] encryptedData) {
        // Mock implementation
        this.encryptedData = encryptedData;
        isDataStoredWithInsecureKey = false;
    }
}

