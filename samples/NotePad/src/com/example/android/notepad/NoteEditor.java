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

package com.example.android.notepad;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;

/**
 * This Activity handles "editing" a note, where editing is responding to
 * {@link Intent#ACTION_VIEW} (request to view data), edit a note
 * {@link Intent#ACTION_EDIT}, create a note {@link Intent#ACTION_INSERT},
 * or create a new note from the current contents of the clipboard.
 * {@link Intent#ACTION_PASTE}.
 *
 * This activity is used only for small screen devices where a new activity is
 * opened for editing or viewing the notes.
 * For large screens {@link NoteEditorFragment} is displayed in the main
 * activity along with {@link NotesListFragment}
 */

public class NoteEditor extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Sets the layout for this Activity.
        setContentView(R.layout.note_editor_activity);

    }

}
