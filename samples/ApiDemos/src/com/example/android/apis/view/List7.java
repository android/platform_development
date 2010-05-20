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

package com.example.android.apis.view;

// Need the following import to get access to the app resources, since this
// class is in a sub-package.
import com.example.android.apis.R;

import android.app.ListActivity;
import android.database.Cursor;
import android.provider.ContactsContract;
import android.os.Bundle;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ListAdapter;
import android.widget.SimpleCursorAdapter;
import android.widget.TextView;

/**
 * A list view example where the data comes from a cursor.
 */
public class List7 extends ListActivity implements OnItemSelectedListener {
    private static final String[] PEOPLE_PROJECTION = new String[] {
        ContactsContract.Contacts._ID,
        ContactsContract.Contacts.DISPLAY_NAME,
        ContactsContract.Contacts.HAS_PHONE_NUMBER
    };

    private static final String[] PHONE_PROJECTION = new String[] {
        ContactsContract.CommonDataKinds.Phone._ID,
        ContactsContract.CommonDataKinds.Phone.CONTACT_ID,
        ContactsContract.CommonDataKinds.Phone.TYPE,
        ContactsContract.CommonDataKinds.Phone.LABEL,
        ContactsContract.CommonDataKinds.Phone.NUMBER
    };

    private int mColumnHasPhoneNumber;
    private int mColumnContactId;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.list_7);
        mPhone = (TextView) findViewById(R.id.phone);
        getListView().setOnItemSelectedListener(this);

        // Get a cursor with all people
        Cursor c = getContentResolver().query(ContactsContract.Contacts.CONTENT_URI,
                PEOPLE_PROJECTION, null, null, null);
        startManagingCursor(c);

        mColumnHasPhoneNumber = c.getColumnIndex(ContactsContract.Contacts.HAS_PHONE_NUMBER);
        mColumnContactId = c.getColumnIndex(ContactsContract.Contacts._ID);

        ListAdapter adapter = new SimpleCursorAdapter(this,
                // Use a template that displays a text view
                android.R.layout.simple_list_item_1,
                // Give the cursor to the list adatper
                c,
                // Map the DISPLAY_NAME column in the people database to...
                new String[] {ContactsContract.Contacts.DISPLAY_NAME},
                // The "text1" view defined in the XML template
                new int[] {android.R.id.text1});
        setListAdapter(adapter);
    }

    public void onItemSelected(AdapterView<?> parent, View v, int position, long id) {
        if (position >= 0) {
            //Get current Contact cursor
            Cursor c = (Cursor) parent.getItemAtPosition(position);
            int hasPhone = c.getInt(mColumnHasPhoneNumber);
            String text = "Nothing...";
            //Only if contact has a PhoneNumber
            if (hasPhone == 1) {
                int contactId = c.getInt(mColumnContactId);
                //Filter the Phone cursor the the current Contact
                Cursor phoneCursor = getContentResolver().query(
                        ContactsContract.CommonDataKinds.Phone.CONTENT_URI,
                        PHONE_PROJECTION,
                        ContactsContract.CommonDataKinds.Phone.CONTACT_ID +" = "+ contactId,
                        null, null);
                startManagingCursor(phoneCursor);
                //Phone Number present?
                if (phoneCursor.getCount() > 0) {
                    //only display the first number
                    phoneCursor.moveToFirst();
                    int columnPhoneType = phoneCursor.getColumnIndex(ContactsContract.CommonDataKinds.Phone.TYPE);
                    int columnPhoneNumber = phoneCursor.getColumnIndex(ContactsContract.CommonDataKinds.Phone.NUMBER);
                    int type = phoneCursor.getInt(columnPhoneType);
                    String phone = phoneCursor.getString(columnPhoneNumber);
                    String label = null;
                    //Custom type? Then get the custom label
                    if (type == ContactsContract.CommonDataKinds.Phone.TYPE_CUSTOM) {
                        int columnPhoneLabel = phoneCursor.getColumnIndex(ContactsContract.CommonDataKinds.Phone.LABEL);
                        label = phoneCursor.getString(columnPhoneLabel);
                    }
                    //Get the readable string
                    String numberType = (String) ContactsContract.CommonDataKinds.Phone.getTypeLabel(
                            getResources(), type, label);
                    text = numberType + ": " + phone;
                }
                stopManagingCursor(phoneCursor);
                phoneCursor.close();
            }
            mPhone.setText(text);
        }
    }

    public void onNothingSelected(AdapterView<?> parent) {
        mPhone.setText(R.string.list_7_nothing);
    }

    private TextView mPhone;
}