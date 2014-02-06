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
package com.android.emulator.sms.test;

import android.content.Context;
import android.content.ContentResolver;
import android.net.Uri;
import android.database.Cursor;
import android.os.Bundle;
import android.os.HandlerThread;
import android.test.AndroidTestCase;

import junit.framework.Assert;

/**
 * Sms Test
 *
 * Test the SMS API by verifying the previously set location
 */
public class SmsTest extends AndroidTestCase {

   /**
     * Prior to running this test an sms must be sent 
     * via ddms
     */
    public final static String FROM = "5551213";
    public final static String BODY = "test sms";
    
       @Override
    protected void setUp() throws Exception {
   }

    /**
     * verify that the last location equals to the location set
     * via geo fix command
     */
    public void testRecievedSms(){
        ContentResolver r = getContext().getContentResolver();
        Uri message = Uri.parse("content://sms/");
        Cursor c = r.query(message,null,null,null,null);
        c.moveToFirst();
        String from = c.getString(c.getColumnIndexOrThrow("address"));
        String body = c.getString(c.getColumnIndexOrThrow("body"));
        c.close();
        assertEquals(FROM, from);
        assertEquals(BODY, body);
   }

}
