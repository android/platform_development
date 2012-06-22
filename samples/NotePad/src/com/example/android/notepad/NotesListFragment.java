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
import android.app.ListFragment;
import android.app.LoaderManager;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.ComponentName;
import android.content.ContentUris;
import android.content.Context;
import android.content.CursorLoader;
import android.content.Intent;
import android.content.Loader;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.AbsListView;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.SimpleCursorAdapter;

/**
 * Fragment to display a list of notes. It is hosted by {@link NotesList}
 * activity.
 *
 * NOTE: Notice that the provider operations in this Activity are
 * taking place on the UI thread. This is not a good practice. It is only done
 * here to make the code more readable. A real application should use the
 * {@link android.content.AsyncQueryHandler} or {@link android.os.AsyncTask}
 * object to perform operations asynchronously on a separate thread. *
 */
public class NotesListFragment extends ListFragment
        implements LoaderManager.LoaderCallbacks<Cursor> {

    // For logging and debugging
    private static final String TAG = "NotesListFragment";

    /** The index of the title column */
    private static final int COLUMN_INDEX_TITLE = 1;

    /**
     * The columns needed by the cursor adapter
     */
    private static final String[] PROJECTION = new String[] {
            NotePad.Notes._ID, // 0
            NotePad.Notes.COLUMN_NAME_TITLE, // 1
    };

    // Adapter to show data fetched from a cursor in a list view
    private SimpleCursorAdapter mAdapter;

    //Handle to the editor fragment loaded in same activity- only for large screens
    private NoteEditorFragment mEditorFragment;

    private boolean mIsEditorFragmentLoaded;

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        // Gets the intent that started this Activity.
        Intent intent = getActivity().getIntent();

        /*
         * If no data is given in the Intent that started this Activity, then
         * this Activity was started when the intent filter matched a MAIN
         * action. We should use the default provider URI which accesses a
         * list of notes.
         */
        if (intent.getData() == null) {
            intent.setData(NotePad.Notes.CONTENT_URI);
        }

        /*
         * Sets the callback for context menu activation for the ListView. The
         * listener is set to be this Activity. The effect is that context menus
         * are enabled for items in the ListView, and the context menu is
         * handled by a method in NotesListFragment.
         */
        getListView().setOnCreateContextMenuListener(this);

        // Set a round black border to highlight the list
        getListView().setBackgroundResource(R.drawable.listborder);

        /*
         * The following two arrays create a "map" between columns in the cursor
         * and view IDs for items in the ListView. Each element in the
         * dataColumns array represents a column name; each element in the
         * viewID array represents the ID of a View. The SimpleCursorAdapter
         * maps them in ascending order to determine where each column value
         * will appear in the ListView.
         */

        // The names of the cursor columns to display in the view,
        // initialized to the title column
        String[] dataColumns = {
                NotePad.Notes.COLUMN_NAME_TITLE };

        // The view IDs that will display the cursor columns, initialized to the
        // TextView in noteslist_item.xml
        int[] viewIDs = {
                android.R.id.text1 };

        // Create an empty adapter we will use to display the loaded data.
        // Data would be filled in this adapter in the loader
        mAdapter = new SimpleCursorAdapter(getActivity(),
                R.layout.noteslist_item, null,
                dataColumns,
                viewIDs, 0);

        setListAdapter(mAdapter);

        //Check if editor fragment is loaded and hence we are using a dual pane layout
        mEditorFragment = (NoteEditorFragment) getActivity().getFragmentManager()
                .findFragmentById(R.id.editorFragment);
        mIsEditorFragmentLoaded = (mEditorFragment != null && mEditorFragment.isInLayout());

        if (mIsEditorFragmentLoaded) {
            // In dual-pane mode, the list view highlights the selected item.
            getListView().setChoiceMode(AbsListView.CHOICE_MODE_SINGLE);
        }

        // Prepare the loader.
        // Either re-connect with an existing one, or start a new one.
        getLoaderManager().initLoader(0, null, this);
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        /* The only menu items that are actually handled are DELETE and COPY.
         * Anything else is an alternative option, for which default handling should be done.
         */

        // The data from the menu item.
        AdapterView.AdapterContextMenuInfo info;

        /*
         * Gets the extra info from the menu item. When an note in the Notes
         * list is long-pressed, a context menu appears. The menu items for the
         * menu automatically get the data associated with the note that was
         * long-pressed. The data comes from the provider that backs the list.
         * The note's data is passed to the context menu creation routine in a
         * ContextMenuInfo object. When one of the context menu items is
         * clicked, the same data is passed, along with the note ID, to
         * onContextItemSelected() via the item parameter.
         */
        try {
            // Casts the data object in the item into the type for AdapterView
            // objects.
            info = (AdapterView.AdapterContextMenuInfo) item.getMenuInfo();
        } catch (ClassCastException e) {

            // If the object can't be cast, logs an error
            Log.e(TAG, "bad menuInfo", e);

            // Triggers default processing of the menu item.
            return false;
        }
        // Appends the selected note's ID to the URI sent with the incoming Intent
        Uri noteUri = ContentUris.withAppendedId(getActivity().getIntent().getData(), info.id);

        /*
         * Gets the menu item's ID and compares it to known actions.
         */
        switch (item.getItemId()) {
            case R.id.context_open:
                openNote(noteUri);
                return true;
//BEGIN_INCLUDE(copy)
            case R.id.context_copy:
                // Gets a handle to the clipboard service.
                ClipboardManager clipboard = (ClipboardManager)
                        getActivity().getSystemService(Context.CLIPBOARD_SERVICE);

                // Copies the notes URI to the clipboard.
                // In effect, this copies the note itself
                clipboard.setPrimaryClip(ClipData.newUri(/*new clipboard item holding a URI*/
                        getActivity().getContentResolver(), /*resolver to retrieve URI info */
                        "Note", /* label for the clip */
                        noteUri) /* the URI*/
                        );
                return true;
//END_INCLUDE(copy)
            case R.id.context_delete:
                deleteNote(noteUri);
                return true;

            default:
                return super.onContextItemSelected(item);
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Since we want to display options menu in this fragment so we need to call
        // the following method
        setHasOptionsMenu(true);

    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View view, ContextMenuInfo menuInfo) {
        /*
         * This method is called when the user context-clicks a note in the list.
         * NotesList registers itself as the handler for context menus in its
         * ListView (this is done in onCreate()). The only available options are
         * COPY and DELETE. Context-click is equivalent to long-press.
         */

        // The data from the menu item.
        AdapterView.AdapterContextMenuInfo info;

        // Tries to get the position of the item in the ListView that was long-pressed

        try {
            // Casts the incoming data object into the type for AdapterView objects
            info = (AdapterView.AdapterContextMenuInfo) menuInfo;
        } catch (ClassCastException e) {
            // If the menu object can't be cast, logs an error.
            Log.e(TAG, "bad menuInfo", e);
            return;
        }

        /*
         * Gets the data associated with the item at the selected position.
         * getItem() returns whatever the backing adapter of the ListView has
         * associated with the item. In NotesList, the adapter associated all of
         * the data for a note with its list item. As a result, getItem()
         * returns that data as a Cursor.
         */
        Cursor cursor = (Cursor) getListAdapter().getItem(info.position);

        // If the cursor is empty, then for some reason the adapter can't get
        // the data from the provider, so returns null to the caller.
        if (cursor == null) {
            // For some reason the requested item isn't available, do nothing
            return;
        }

        // Inflate menu from XML resource
        MenuInflater inflater = getActivity().getMenuInflater();
        inflater.inflate(R.menu.list_context_menu, menu);

        // Sets the menu header to be the title of the selected note.
        menu.setHeaderTitle(cursor.getString(COLUMN_INDEX_TITLE));

        // Append to the menu items for any other activities that can do stuff with it
        // as well. This does a query on the system for any activities that
        // implement the ALTERNATIVE_ACTION for our data, adding a menu item
        // for each one that is found.
        Intent intent = new Intent(null, Uri.withAppendedPath(getActivity().getIntent().getData(),
                Integer.toString((int) info.id)));
        intent.addCategory(Intent.CATEGORY_ALTERNATIVE);
        menu.addIntentOptions(Menu.CATEGORY_ALTERNATIVE, 0, 0,
                new ComponentName(getActivity(), NotesList.class), null, intent, 0, null);
    }

    @Override
    public Loader<Cursor> onCreateLoader(int id, Bundle args) {
        // This is called when a new Loader needs to be created. This
        // sample only has one Loader, so we don't care about the ID.
        // First, pick the base URI to use which is present in the current intent

        Uri baseUri = getActivity().getIntent().getData();

        // Now create and return a CursorLoader that will take care of
        // creating a Cursor for the data being displayed.
        return new CursorLoader(getActivity(), baseUri,
                PROJECTION, null, null,
                NotePad.Notes.DEFAULT_SORT_ORDER);

    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        super.onCreateOptionsMenu(menu, inflater);

        /*
         * Sets up a menu that provides the Insert option plus a list of
         * alternative actions for this Activity. Other applications that want to
         * handle notes can "register" themselves in Android by providing an intent
         * filter that includes the category ALTERNATIVE and the mimeTYpe
         * NotePad.Notes.CONTENT_TYPE. If they do this, the code in
         * onCreateOptionsMenu() will add the Activity that contains the intent
         * filter to its list of options. In effect, the menu will offer the user
         * other applications that can handle notes.
         */

        inflater.inflate(R.menu.list_options_menu, menu);

        // Generate any additional actions that can be performed on the
        // overall list. In a normal install, there are no additional
        // actions found here, but this allows other applications to extend
        // our menu with their own actions.
        Intent intent = new Intent(null, getActivity().getIntent().getData());
        intent.addCategory(Intent.CATEGORY_ALTERNATIVE);
        menu.addIntentOptions(Menu.CATEGORY_ALTERNATIVE, 0, 0,
                new ComponentName(getActivity(), NotesList.class), null, intent, 0, null);

    }

    @Override
    public void onListItemClick(ListView l, View v, int position, long id) {
        // Constructs a new URI from the incoming URI and the row ID
        Uri uri = ContentUris.withAppendedId(getActivity().getIntent().getData(), id);
        openNote(uri);
    }

    @Override
    public void onLoaderReset(Loader<Cursor> loader) {
        // This is called when the last Cursor provided to onLoadFinished()
        // above is about to be closed. We need to make sure we are no
        // longer using it.
        mAdapter.swapCursor(null);

    }

    @Override
    public void onLoadFinished(Loader<Cursor> loader, Cursor cursor) {
        // Swap the new cursor in. (The framework will take care of closing the
        // old cursor once we return.)
        mAdapter.swapCursor(cursor);

    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.menu_add:
                addNote();
                return true;
            case R.id.menu_paste:
                pasteNote();
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    @Override
    public void onPrepareOptionsMenu(Menu menu) {
        super.onPrepareOptionsMenu(menu);

        // The paste menu item is enabled if there is data on the clipboard.
        ClipboardManager clipboard = (ClipboardManager)
                getActivity().getSystemService(Context.CLIPBOARD_SERVICE);

        MenuItem mPasteItem = menu.findItem(R.id.menu_paste);

        // If the clipboard contains an item, enables the Paste option on the menu
        if (clipboard.hasPrimaryClip()) {
            mPasteItem.setEnabled(true);
        } else {
            // If the clipboard is empty, disables the menu's Paste option.
            mPasteItem.setEnabled(false);
        }

        // If there are any notes in the list (which implies that one of
        // them is selected), then we need to generate the actions that
        // can be performed on the current selection. This will be a combination
        // of our own specific actions along with any extensions that can be
        // found.
        if (mAdapter != null &&
                mAdapter.getCount() > 0 &&
                mIsEditorFragmentLoaded == false) {

            // This is the selected item.
            Uri uri = ContentUris.withAppendedId(
                    getActivity().getIntent().getData(), getSelectedItemId());

            // Creates an array of Intents with one element. This will be
            // used to send an Intent based on the selected menu item.
            Intent[] specifics = new Intent[1];

            // Sets the Intent in the array to be an EDIT action on the URI
            // of the selected note.
            specifics[0] = new Intent(Intent.ACTION_EDIT, uri);

            // Creates an array of menu items with one element. This will
            // contain the EDIT option.
            MenuItem[] items = new MenuItem[1];

            // Creates an Intent with no specific action, using the URI of
            // the selected note.
            Intent intent = new Intent(null, uri);

            /*
             * Adds the category ALTERNATIVE to the Intent, with the note ID URI
             * as its data. This prepares the Intent as a place to group
             * alternative options in the menu.
             */
            intent.addCategory(Intent.CATEGORY_ALTERNATIVE);

            /*
             * Add alternatives to the menu
             */
            menu.addIntentOptions(
                    Menu.CATEGORY_ALTERNATIVE, // Add the Intents as options in alternatives group.
                    Menu.NONE, // A unique item ID is not required.
                    Menu.NONE, // The alternatives don't need to be in order.
                    null, // The caller's name is not excluded from the group.
                    specifics, // These specific options must appear first.
                    intent, // These Intent objects map to the options in specifics.
                    Menu.NONE, // No flags are required.
                    items // The menu items generated from the specifics-to- Intents mapping
                );
        } else {
            // If the list is empty, removes any existing alternative actions from the menu
            menu.removeGroup(Menu.CATEGORY_ALTERNATIVE);
        }
    }

    /**
     * Adds a note in notes list by creating an Intent with ACTION_INSERT and
     * starting a new activity if EditorFragment is not loaded in the same
     * activity or by sending a trigger to the EditorFragment if it is loaded.
     */
    private void addNote() {
        Intent intent = new Intent(Intent.ACTION_INSERT, getActivity().getIntent().getData());
        // for dual pane layout, do not start a new activity just trigger the editor fragment
        if (mIsEditorFragmentLoaded) {
            mEditorFragment.handleTrigger(intent);
            mEditorFragment.updateEditorUI();
        } else {
            /*
             * Launches a new Activity using an Intent. The intent filter for
             * the Activity has to have action ACTION_INSERT. No category is
             * set, so DEFAULT is assumed. In effect, this starts the NoteEditor
             * Activity in NotePad.
             */
            startActivity(intent
            );
        }

    }

    /**
     * Deletes a note from the notes list by calling delete method on the content resolver
     * @param noteUri URI of the note to be deleted
     */
    private void deleteNote(Uri noteUri) {
        // Deletes the note from the provider by passing in a URI in
        // note ID format.

        // Please see the introductory note about performing provider
        // operations on the UI thread.
        getActivity().getContentResolver().delete(
                noteUri, // The URI of the provider
                null, // No where clause is needed, since only a single note ID is being passed in.
                null // No where clause is used, so no where arguments are needed.
                );
        /*
        * If NoteEditor is already open in case of dual pane layout then we need to reset the editor
        * so that the deleted note's text is not displayed in the editor.
        */
        if (mIsEditorFragmentLoaded) {
            mEditorFragment.resetEditor();
        }

    }

    /**
     * Opens a note in notes list by creating an Intent with ACTION_EDIT and
     * starting a new activity if EditorFragment is not loaded in the same
     * activity or by sending a trigger to the EditorFragment if it is loaded.
     *
     * @param uri Uri of the note to be edited/opened
     */
    private void openNote(Uri uri) {
        // Gets the action from the Intent which started this activity
        String action = getActivity().getIntent().getAction();

        // Handles requests for note data
        if (Intent.ACTION_PICK.equals(action) || Intent.ACTION_GET_CONTENT.equals(action)) {

            // Sets the result to return to the component that called this
            // Activity. The result contains the new URI
            getActivity().setResult(Activity.RESULT_OK, new Intent().setData(uri));
        } else {
            // for dual pane layout, do not start a new activity just trigger the editor fragment
            if (mIsEditorFragmentLoaded) {
                mEditorFragment.openNote(uri);
            } else {
                // Sends out an Intent to start an Activity that can handle ACTION_EDIT.
                // The Intent's data is the note ID URI. The effect is to call NoteEditor
                startActivity(new Intent(Intent.ACTION_EDIT, uri));
            }
        }
    }

    /**
     * Pastes the copied note whose data is present in the clipboard in the
     * notes list by creating an Intent with ACTION_PASTE and starting a new
     * activity if EditorFragment is not loaded in the same activity or by
     * sending a trigger to the EditorFragment if it is loaded.
     */
    private void pasteNote() {

        Intent intent = new Intent(Intent.ACTION_PASTE, getActivity().getIntent().getData());
        // for dual pane layout, do not start a new activity just trigger the editor fragment
        if (mIsEditorFragmentLoaded) {
            mEditorFragment.handleTrigger(intent);
            mEditorFragment.updateEditorUI();
        } else {
            /*
             * Launches a new Activity using an Intent. The intent filter for
             * the Activity has to have action ACTION_PASTE. No category is set,
             * so DEFAULT is assumed. In effect, this starts the NoteEditor
             * Activity in NotePad.
             */
            startActivity(intent);
        }

    }
}

