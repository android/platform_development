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

package com.example.android.notepad;

import android.app.Activity;
import android.app.Fragment;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

/**
 * This fragment handles "editing" a note, where editing is responding to
 * {@link Intent#ACTION_VIEW} (request to view data), edit a note
 * {@link Intent#ACTION_EDIT}, create a note {@link Intent#ACTION_INSERT}, or
 * create a new note from the current contents of the clipboard.
 * {@link Intent#ACTION_PASTE}.
 *
 * For small screens this is displayed in{@link NoteEditor} activity and for large
 * screens this is displayed in the main activity along with {@link NotesListFragment}
 *
 * NOTE: Notice that the provider operations in this Activity are taking place on
 * the UI thread. This is not a good practice. It is only done here to make the
 * code more readable. A real application should use the
 * {@link android.content.AsyncQueryHandler} or {@link android.os.AsyncTask} object
 * to perform operations asynchronously on a separate thread.
 */
public class NoteEditorFragment extends Fragment {

    // For logging and debugging purposes
    private static final String TAG = "NoteEditorFragment";

    // A label for the saved state of the activity
    private static final String ORIGINAL_CONTENT = "originalContent";

    /*
     * Creates a projection that returns the note ID and the note contents.
     */
    private static final String[] PROJECTION =
            new String[] {
                    NotePad.Notes._ID,
                    NotePad.Notes.COLUMN_NAME_TITLE,
                    NotePad.Notes.COLUMN_NAME_NOTE
            };

    // This Fragment can be started by more than one action. Each action is
    // represented as a "state" constant
    private static final int STATE_LAUNCHED = 0;
    private static final int STATE_INSERT = 1;
    private static final int STATE_EDIT = 2;

    // State of the fragment in terms of current action being performed
    private int mState;

    // Field to hold last save note value for currently visible note
    // This is initialized to an empty string to start editor view with a blank note
    private String mOriginalContent = "";

    // Global mutable fields
    private Cursor mCursor;
    private Uri mUri;
    private LinedEditText mNoteText;
    private TextView mTitle;

    private boolean mIsDualPane;

    /**
     * Handles triggers from {@link NotesListFragment} to add or paste a note and performs actions
     * depending upon the intent received while creating the activity which contains this fragment.
     *
     * @param intent Trigger to perform actions like edit, paste and create note
     */
    public void handleTrigger(final Intent intent) {
        /*
         * Sets up the editor, based on the action specified for the incoming Intent.
         */

        // Gets the action for this particular trigger
        final String action = intent.getAction();

        // For an edit action
        if (Intent.ACTION_EDIT.equals(action)) {

            // Sets the Fragment state to EDIT, and gets the URI for the data to
            // be edited.
            mState = STATE_EDIT;
            mUri = intent.getData();

            // For an insert or paste action:
        } else if (Intent.ACTION_INSERT.equals(action)
                || Intent.ACTION_PASTE.equals(action)) {

            // Sets the Fragment state to INSERT, gets the general note URI, and
            // inserts an empty record in the provider
            mState = STATE_INSERT;
            mUri = getActivity().getContentResolver().insert(intent.getData(), null);

            /*
             * If the attempt to insert the new note fails, shuts down this
             * Activity if in single pane mode. The originating Activity receives back
             * RESULT_CANCELED if it requested a result. Logs that the insert failed.
             */
            if (mUri == null) {

                // Writes the log identifier, a message, and the URI that failed.
                Log.e(TAG, "Failed to insert new note into " + getActivity().getIntent().getData());

                if (!mIsDualPane)  {
                    // For single pane close this activity
                    getActivity().finish();
                }
                return;
            }

            // Since the new entry was created, this sets the result to be returned
            getActivity().setResult(Activity.RESULT_OK, (new Intent()).setAction(
                    mUri.toString()));

            // If fragment is getting launched for the 1st time with the creation of the activity
            // Set current uri as null
        } else if (Intent.ACTION_MAIN.equals(action)) {
            mState = STATE_LAUNCHED;
            mUri = null;
            //If the action was other than EDIT or INSERT or MAIN
        } else {
            // Logs an error that the action was not understood, finishes the
            // Activity, and returns RESULT_CANCELED to an originating Activity.
            Log.e(TAG, "Unknown action, exiting");
            if (!mIsDualPane)  {
                // For single pane close this activity
                getActivity().finish();
            }
            return;
        }

        /*
         * Using the URI passed in with the triggering Intent, gets the note or
         * notes in the provider. Note: This is being done on the UI thread. It
         * will block the thread until the query completes. In a sample app,
         * going against a simple provider based on a local database, the block
         * will be momentary, but in a real app you should use
         * android.content.AsyncQueryHandler or android.os.AsyncTask.
         */
        if (mState != STATE_LAUNCHED) {
            mCursor = getActivity().getContentResolver().query(
                    mUri, // The URI that gets multiple notes from the provider.
                    PROJECTION, // A projection that returns the note ID and note
                                // content for each note.
                    null, // No "where" clause selection criteria.
                    null, // No "where" clause selection values.
                    null // Use the default sort order (modification date, descending)
                    );
        }

        // For a paste, initializes the data from clipboard.
        // (Must be done after mCursor is initialized.)
        if (Intent.ACTION_PASTE.equals(action)) {
            // Does the paste
            performPaste();
            // Switches the state to EDIT so the title can be modified.
            mState = STATE_EDIT;
        }

    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        mTitle = (TextView) getActivity().findViewById(R.id.noteTitle);
        mNoteText = (LinedEditText) getActivity().findViewById(R.id.note);
        // Check if list fragment is loaded and hence we are using a dual pane layout
        NotesListFragment listFragment = (NotesListFragment) getActivity()
                .getFragmentManager()
                .findFragmentById(R.id.listFragment);
        mIsDualPane = (listFragment != null && listFragment.isInLayout());

    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setHasOptionsMenu(true);

        /*
         * Creates an Intent to use when the Activity object's result is sent
         * back to the caller.
         */
        final Intent intent = getActivity().getIntent();
        /*
         * From the incoming Intent, it determines what kind of editing is
         * desired, and then does it.
         */
        handleTrigger(intent);
        /*
         * If this Activity had stopped previously, its state was written the
         * ORIGINAL_CONTENT location in the saved Instance state. This gets the
         * state.
         */
        if (savedInstanceState != null) {
            mOriginalContent = savedInstanceState.getString(ORIGINAL_CONTENT);
        }

    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        super.onCreateOptionsMenu(menu, inflater);

        /*
         * Builds the menus for editing and inserting, and adds in alternative actions that
         * registered themselves to handle the MIME types for this application.
         */

        // Inflate menu from XML resource
        inflater.inflate(R.menu.editor_options_menu, menu);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        // Inflate fragment view
        View view = inflater.inflate(R.layout.note_editor, container, false);
        return view;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle all of the possible menu actions.
        switch (item.getItemId()) {
            case R.id.menu_save:
                // Gets the current text in Editor
                String text = mNoteText.getText().toString();
                // Updates note retaining title. For new notes title will be same as note text
                updateNote(text, null);
                if (mIsDualPane) {
                    // For dual pane just update the UI
                    updateEditorUI();
                } else {
                    // For single pane close this activity and go back to list of notes
                    getActivity().finish();
                }
                break;

            case R.id.menu_delete:
                deleteNote();
                if (mIsDualPane) {
                    // For dual pane set the default text and title
                    mTitle.setText(R.string.title_create);
                    mNoteText.setText(mOriginalContent);
                } else {
                    // For single pane close this activity and go back to list of notes
                    getActivity().finish();
                }
                break;

            case R.id.menu_revert:
                cancelNote();
                item.setEnabled(false);
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onPause() {
        super.onPause();

        /*
         * If the user hasn't done anything, then this deletes or clears out the
         * note, otherwise it writes the user's work to the provider.
         */

        /*
         * Tests to see that the query operation didn't fail (see onCreate()).
         * The Cursor object will exist, even if no records were returned,
         * unless the query failed because of some exception or error.
         */
        if (mCursor != null) {

            // Get the current note text.
            String text = mNoteText.getText().toString();
            int length = text.length();
            /*
             * If the Activity is in the midst of finishing and there is no text
             * in the current note, returns a result of CANCELED to the caller,
             * and deletes the note. This is done even if the note was being
             * edited, the assumption being that the user wanted to "clear out"
             * (delete) the note.
             */
            if (getActivity().isFinishing() && (length == 0)) {
                getActivity().setResult(Activity.RESULT_CANCELED);
                // When we open the editor and without doing anything close it
                // then mUri would be  null so note needs to be deleted in that case
                if (mUri != null) {
                    deleteNote();
                }

                /*
                 * Writes the edits to the provider. The note has been edited if
                 * an existing note was retrieved into the editor *or* if a new
                 * note was inserted. In the latter case, onCreate() inserted a
                 * new empty note into the provider, and it is this new note
                 * that is being edited.
                 */
            } else if (mState == STATE_EDIT) {
                // Creates a map to contain the new values for the columns
                updateNote(text, null);
            } else if (mState == STATE_INSERT) {
                updateNote(text, text);
            }
        }
    }

    @Override
    public void onPrepareOptionsMenu(Menu menu) {
        super.onPrepareOptionsMenu(menu);
        // Only add extra menu items for a saved note
        if (mState == STATE_EDIT) {
            /*
             * Append to the menu items for any other activities that can do
             * stuff with it as well. This does a query on the system for any
             * activities that implement the ALTERNATIVE_ACTION for our data,
             * adding a menu item for each one that is found.
             */
            Intent intent = new Intent(null, mUri);
            intent.addCategory(Intent.CATEGORY_ALTERNATIVE);
            menu.addIntentOptions(Menu.CATEGORY_ALTERNATIVE, 0, 0,
                    new ComponentName(getActivity(), NoteEditor.class), null, intent, 0, null);
        }

        // Initially when fragment is created and this method is called we let
        // revert menu item to be disabled as nothing would have changed by then
        if (mNoteText != null) {
            //Check if note has changed and enable/disable the revert option
            String currentNote = mNoteText.getText().toString();
            if (mOriginalContent.length() != 0) {
                if (mOriginalContent.equals(currentNote)) {
                    menu.findItem(R.id.menu_revert).setEnabled(false);
                } else {
                    menu.findItem(R.id.menu_revert).setEnabled(true);
                }
            } else if (currentNote.length() != 0) {
                // mOriginalContent is empty here and currentNote has some text
                // so this is the case where a new note is being created
                menu.findItem(R.id.menu_revert).setEnabled(true);
            }
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        //Initialize or update the editor view
        updateEditorUI();
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        // Save away the original text, so we still have it if the activity
        // needs to be killed while paused.
        outState.putString(ORIGINAL_CONTENT, mOriginalContent);
    }

    /**
     * Opens a note in the editor view when triggered by the ListFragment to do so
     * @param uri URI of the note to be opened/edited
     */
    public void openNote(Uri uri) {
        mUri = uri;
        // Sets state as edit so that updating the text in editor view results in updating same note
        mState = STATE_EDIT;
        updateEditorUI();
    }

    /**
     * Resets Editor UI and its associated state to default values. This needs to be called after
     * deleting a note to clear out data associated with the last edited note.
     */
    public void resetEditor() {
        mUri = null;
        mState = STATE_INSERT;
        mOriginalContent = "";
        mTitle.setText(R.string.title_create);
        mNoteText.setText("");
    }

    /**
     * Sets an appropriate title for the action chosen by the user, puts the
     * note contents into the TextView, and saves the original text as a backup.
     */
    public void updateEditorUI() {
        // No update is required if we are just getting launched. Updates would be done
        // when any one of the note is selected
        if (mState != STATE_LAUNCHED) {
            mCursor = getActivity().getContentResolver().query(
                    mUri, // The URI that gets multiple notes from the provider.
                    PROJECTION, // A projection that returns the note ID and
                                // note content for each note.
                    null, // No "where" clause selection criteria.
                    null, // No "where" clause selection values.
                    null // Use the default sort order (modification date, descending)
                    );

            if (mCursor != null) {

                /*
                 * Moves to the first record. Always call moveToFirst() before
                 * accessing data in a Cursor for the first time. The semantics
                 * of using a Cursor are that when it is created, its internal
                 * index is pointing to a "place" immediately before the first
                 * record.
                 */
                mCursor.moveToFirst();

                // Modifies the window title for the Fragment according to the
                // current Fragment state.
                if (mState == STATE_EDIT) {
                    // Set the title of the Fragment to include the note title
                    int colTitleIndex = mCursor.getColumnIndex(NotePad.Notes.COLUMN_NAME_TITLE);
                    String title = mCursor.getString(colTitleIndex);

                    Resources res = getResources();
                    String text = String.format(res.getString(R.string.title_edit), title);
                    mTitle.setText(text);
                    // Sets the title to "create" for inserts
                } else if (mState == STATE_INSERT) {
                    mTitle.setText(getText(R.string.title_create));
                }

                /*
                 * onResume() may have been called after the Activity lost focus
                 * (was paused). The user was either editing or creating a note
                 * when the Activity paused. The Activity should re-display the
                 * text that had been retrieved previously, but it should not
                 * move the cursor. This helps the user to continue editing or
                 * entering.
                 */
                int colNoteIndex = mCursor.getColumnIndex(NotePad.Notes.COLUMN_NAME_NOTE);
                String note = mCursor.getString(colNoteIndex);

                /*
                 * Gets the note text from the Cursor and puts it in the
                 * TextView, but doesn't change the text cursor's position.
                 */
                mNoteText.setTextKeepState(note);

                // Stores the original note text, to allow the user to revert
                // changes.
                mOriginalContent = note;

                /*
                 * Something is wrong. The Cursor should always contain data.
                 * Report an error in the note.
                 */
            } else {
                mTitle.setText(getText(R.string.error_title));
                mNoteText.setText(getText(R.string.error_message));
            }
        }
    }

    /**
     * This helper method cancels the work done on a note. It deletes the note
     * if it was newly created, or reverts to the original text of the note i
     */
    protected final void cancelNote() {
        if (mCursor != null) {
            if (mState == STATE_EDIT) {
                // Put the original note text back into the database
                mCursor.close();
                mCursor = null;
                ContentValues values = new ContentValues();
                values.put(NotePad.Notes.COLUMN_NAME_NOTE, mOriginalContent);
                getActivity().getContentResolver().update(mUri, values, null, null);
            } else if (mState == STATE_INSERT) {
                // We inserted an empty note, make sure to delete it
                deleteNote();
            }
        }
        getActivity().setResult(Activity.RESULT_CANCELED);
        if (mIsDualPane) {
            // For dual pane just replace text in editor with last saved text
            mNoteText.setText(mOriginalContent);
        } else {
            // For single pane close this activity after reverting changes
            getActivity().finish();
        }
    }

    /**
     * Take care of deleting a note. Simply deletes the entry.
     */
    protected final void deleteNote() {
        if (mCursor != null) {
            mCursor.close();
            mCursor = null;
            getActivity().getContentResolver().delete(mUri, null, null);
            // Reset editor state to default after deleting the note
            resetEditor();
        }
    }
//BEGIN_INCLUDE(paste)
    /**
     * A helper method that replaces the note's data with the contents of the
     * clipboard.
     */
    protected final void performPaste() {

        // Gets a handle to the Clipboard Manager
        ClipboardManager clipboard = (ClipboardManager)
                getActivity().getSystemService(Context.CLIPBOARD_SERVICE);

        // Gets a content resolver instance
        ContentResolver contentResolver = getActivity().getContentResolver();

        // Gets the clipboard data from the clipboard
        ClipData clip = clipboard.getPrimaryClip();
        if (clip != null) {

            String text = null;
            String title = null;

            // Gets the first item from the clipboard data
            ClipData.Item item = clip.getItemAt(0);

            // Tries to get the item's contents as a URI pointing to a note
            Uri uri = item.getUri();

            // Tests to see that the item actually is an URI, and that the URI
            // is a content URI pointing to a provider whose MIME type is the
            // same as the MIME type supported by the Note pad provider.
            if (uri != null &&
                    NotePad.Notes.CONTENT_ITEM_TYPE.equals(contentResolver.getType(uri))) {

                // The clipboard holds a reference to data with a note MIME
                // type. This copies it.
                Cursor cursor = contentResolver.query(
                        uri, // URI for the content provider
                        PROJECTION, // Get the columns referred to in the projection
                        null, // No selection variables
                        null, // No selection variables, so no criteria are needed
                        null // Use the default sort order
                        );

                // If the Cursor is not null, and it contains at least one record
                // (moveToFirst() returns true), then this gets the note data from it.
                if (cursor != null) {
                    if (cursor.moveToFirst()) {
                        int colNoteIndex = mCursor.getColumnIndex(NotePad.Notes.COLUMN_NAME_NOTE);
                        int colTitleIndex = mCursor.getColumnIndex(NotePad.Notes.COLUMN_NAME_TITLE);
                        text = cursor.getString(colNoteIndex);
                        title = cursor.getString(colTitleIndex);
                    }

                    // Closes the cursor.
                    cursor.close();
                }
            }

            // If the contents of the clipboard wasn't a reference to a note,
            // then this converts whatever it is to text.
            if (text == null) {
                text = item.coerceToText(getActivity()).toString();
            }
            // If title is null because the data in clipboard did not point to a valid note
            // then this sets a default title for the editor fragment 
            if (title == null) {
                title = getResources().getString(R.string.coerced_note);
            }

            // Updates the current note with the retrieved title and text.
            updateNote(text, title);
        }
    }
//END_INCLUDE(paste)
    /**
     * Replaces the current note contents with the text and title provided as
     * arguments.
     *
     * @param text The new note contents to use.
     * @param title The new note title to use
     */
    protected final void updateNote(String text, String title) {
        mOriginalContent = text;

        // Sets up a map to contain values to be updated in the provider.
        ContentValues values = new ContentValues();
        values.put(NotePad.Notes.COLUMN_NAME_MODIFICATION_DATE, System.currentTimeMillis());

        // If the action is to insert a new note, this creates an initial title for it.
        if ((mState == STATE_INSERT) || (mState == STATE_LAUNCHED)) {

            // If no title was provided as an argument, create one from the note text.
            if (title == null) {
                // Get the note's length
                int length = text.length();

                // Sets the title by getting a substring of the text that is 31
                // characters long or the number of characters in the note plus one, whichever
                // is smaller.
                title = text.substring(0, Math.min(30, length));

                // If the title contains new lines then replace them with spaces
                if (title.contains("\n")) {
                    title = title.replace('\n', ' ');
                }

                // If the resulting length is more than 30 characters, chops off
                // any trailing spaces
                if (length > 30) {
                    int lastSpace = title.lastIndexOf(' ');
                    if (lastSpace > 0) {
                        title = title.substring(0, lastSpace);
                    }
                }
            }
            // In the values map, sets the value of the title
            values.put(NotePad.Notes.COLUMN_NAME_TITLE, title);
        } else if (title != null) {
            // In the values map, sets the value of the title
            values.put(NotePad.Notes.COLUMN_NAME_TITLE, title);
        }

        // This puts the desired notes text into the map.
        values.put(NotePad.Notes.COLUMN_NAME_NOTE, text);

        /*
         * Updates the provider with the new values in the map. The ListView is
         * updated automatically. The provider sets this up by setting the
         * notification URI for query Cursor objects to the incoming URI. The
         * content resolver is thus automatically notified when the Cursor for
         * the URI changes, and the UI is updated. Note: This is being done on
         * the UI thread. It will block the thread until the update completes.
         * In a sample app, going against a simple provider based on a local
         * database, the block will be momentary, but in a real app you should
         * use android.content.AsyncQueryHandler or android.os.AsyncTask.
         */
        if (mUri != null) {
            getActivity().getContentResolver().update(
                    mUri, // The URI for the record to update.
                    values, // The map of column names and new values to apply to them.
                    null, // No selection criteria are used, so no where columns are necessary.
                    null // No where columns are used, so no where arguments are necessary.
                    );
        } else {
            mUri = getActivity().getContentResolver().insert(NotePad.Notes.CONTENT_URI, values);
        }
        mState = STATE_EDIT;
    }
}

