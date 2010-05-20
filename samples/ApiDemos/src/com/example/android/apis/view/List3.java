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

import android.app.ListActivity;
import android.database.Cursor;
import android.os.Bundle;
import android.provider.ContactsContract;
import android.view.View;
import android.widget.SimpleCursorAdapter;
import android.widget.TextView;

 /**
 * A list view example where the
 * data comes from a cursor, and a
 * SimpleCursorListAdapter is used to map each item to a two-line
 * display.
 */
public class List3 extends ListActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Get a cursor with all phones
        Cursor c = getContentResolver().query(ContactsContract.CommonDataKinds.Phone.CONTENT_URI,
                PHONE_PROJECTION, null, null, null);
        startManagingCursor(c);

        mColumnType = c.getColumnIndex(ContactsContract.CommonDataKinds.Phone.TYPE);
        mColumnLabel = c.getColumnIndex(ContactsContract.CommonDataKinds.Phone.LABEL);

        // Map Cursor columns to views defined in simple_list_item_2.xml
        SimpleCursorAdapter adapter = new SimpleCursorAdapter(this,
                android.R.layout.simple_list_item_2, c,
                        new String[] {
                            ContactsContract.CommonDataKinds.Phone.TYPE,
                            ContactsContract.CommonDataKinds.Phone.NUMBER
                        },
                        new int[] { android.R.id.text1, android.R.id.text2 });
        //Used to display a readable string for the phone type
        adapter.setViewBinder(new SimpleCursorAdapter.ViewBinder() {
            public boolean setViewValue(View view, Cursor cursor, int columnIndex) {
                //Let the adapter handle the binding if the column is not TYPE
                if (columnIndex != mColumnType) {
                    return false;
                }
                int type = cursor.getInt(mColumnType);
                String label = null;
                //Custom type? Then get the custom label
                if (type == ContactsContract.CommonDataKinds.Phone.TYPE_CUSTOM) {
                    label = cursor.getString(mColumnLabel);
                }
                //Get the readable string
                String text = (String) ContactsContract.CommonDataKinds.Phone.getTypeLabel(
                        getResources(), type, label);
                //Set text
                ((TextView) view).setText(text);
                return true;
            }
        });
        setListAdapter(adapter);
    }

    private static final String[] PHONE_PROJECTION = new String[] {
        ContactsContract.CommonDataKinds.Phone._ID,
        ContactsContract.CommonDataKinds.Phone.TYPE,
        ContactsContract.CommonDataKinds.Phone.LABEL,
        ContactsContract.CommonDataKinds.Phone.NUMBER
    };

    private int mColumnType;
    private int mColumnLabel;
}