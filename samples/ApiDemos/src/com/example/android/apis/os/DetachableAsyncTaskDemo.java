package com.example.android.apis.os;

import com.example.android.apis.R;

import android.app.Activity;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.DetachableAsyncTask;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.Toast;

import java.util.Random;

/**
 * Shows a practical example of lifecycle management with a background thread in
 * use.
 * <p>This is useful for applications which have a background task which reports
 * results to the main thread and must reattach to a new activity on
 * configuration change (for example, if the device is rotated).  Without this
 * pattern, it is possible to leak {@link Activity} instances or introduce
 * racing bugs that occur when the user exits an {@link Activity} prior to the
 * completion of the task.</p>
 *
 * <p>The example has a simple {@link DetachableAsyncTask} sleeping in the
 * background and updating a progress dialog after each second. The task and
 * dialog persist across orientation change by reattaching a new set of
 * callbacks (associated with the newly created {@link Activity}.</p>

<h4>Demo</h4>
App/Misc/DetachableAsyncTask
 */
public class DetachableAsyncTaskDemo extends Activity implements OnClickListener {
    private SleepingTask mTask;
    private ProgressDialog mProgress;

    private static final int DIALOG_PROGRESS = 0;

    @Override
    protected void onCreate(Bundle bundle) {
        super.onCreate(bundle);
        setContentView(R.layout.detachable_async_task_demo);

        // Retrieve the retained non-configuration instance object if we're
        // being restarted due to a configuration change.
        mTask = (SleepingTask)getLastNonConfigurationInstance();
        if (mTask != null) {
            // Attach the recovered task to the current activity to respond to
            // callbacks.
            mTask.setCallback(mTaskCallback);
        }

        Button button = (Button)findViewById(R.id.button);
        button.setOnClickListener(this);
    }

    // Restore the original task on configuration change (such as device
    // rotation).
    @Override
    public Object onRetainNonConfigurationInstance() {
        return mTask;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        // This is an important part of the pattern where the relationship
        // between the Activity and the AsyncTask is severed. When performing an
        // orientation change code in onCreate will set the new callback
        // immediately following onDestroy, but if instead the activity is being
        // destroyed by user interaction (such as pressing the back button),
        // this will prevent a leak of the activity. Most importantly, in this
        // case, main thread callbacks will not occur once the thread completes
        // or updates progress since no Activity instance remains to deal with
        // them.
        if (mTask != null) {
            mTask.clearCallback();
        }
    }

    public void onClick(View v) {
        // Begin the timer and the modal dialog showing the number of seconds
        // counted down.
        showDialog(DIALOG_PROGRESS);
        mTask = new SleepingTask();
        mTask.setCallback(mTaskCallback);
        mTask.execute();
    }

    @Override
    protected Dialog onCreateDialog(int id) {
        switch (id) {
            case DIALOG_PROGRESS:
                // Create our progress dialog.
                mProgress = new ProgressDialog(this);
                mProgress.setProgressStyle(ProgressDialog.STYLE_SPINNER);
                mProgress.setMessage(getResources().getQuantityString(R.plurals.run_length,
                        0, 0));
                mProgress.setCancelable(true);
                mProgress.setOnCancelListener(new DialogInterface.OnCancelListener() {
                    public void onCancel(DialogInterface dialog) {
                        Toast.makeText(DetachableAsyncTaskDemo.this, R.string.task_aborted,
                                Toast.LENGTH_SHORT).show();

                        // If the dialog is canceled (for instance, if back is
                        // pushed), go ahead and jettison the sleeper task and
                        // reset the activity state accordingly.
                        mTask.clearCallback();

                        // Canceling the task is not strictly necessary here.
                        // The thread could continue to run until it finishes
                        // and it would not affect our application after the
                        // callbacks are cleared. In our case though it is
                        // simple and convenient to interrupt the sleep and exit
                        // the thread quickly.
                        mTask.cancel(true);

                        // Reset activity state so that a new sleeper task can
                        // be started.
                        mTask = null;
                        removeDialog(DIALOG_PROGRESS);
                    }
                });
                return mProgress;
            default:
                throw new IllegalArgumentException("Unknown dialog id=" + id);
        }
    }

    // Receive callbacks with an implicit reference to the outer class.
    private final DetachableAsyncTask.TaskCallbacks<Integer, Integer> mTaskCallback =
            new DetachableAsyncTask.TaskCallbacks<Integer, Integer>() {
        @Override
        protected void onProgressUpdate(Integer... progress) {
            mProgress.setMessage(getResources().getQuantityString(R.plurals.run_length,
                    progress[0], progress[0]));
        }

        @Override
        protected void onPostExecute(Integer result) {
            Toast.makeText(DetachableAsyncTaskDemo.this, "Ran for " + result + " seconds",
                    Toast.LENGTH_SHORT).show();
            mTask = null;
            removeDialog(DIALOG_PROGRESS);
        }
    };

    /**
     * Simple task which sleeps a random length of time to simulate a
     * long-running background task. Sends a progress update each second that it
     * has slept.
     * <p>
     * Note that with a {@link DetachableAsyncTask} it is not possible to
     * override the onPostExecute, onProgressUpdate, etc methods. It is expected
     * that a callback is attached to respond to these events. This relationship
     * permits the {@link DetachableAsyncTask} class to be declared static which
     * in this case means that there will not be a (potentially leaking)
     * reference to the outer class.
     */
    private static class SleepingTask extends DetachableAsyncTask<Void, Integer, Integer> {
        private static final Random sRandom = new Random();

        private final int mSleepSeconds;

        private static final int MIN_SLEEP_SEC = 3;
        private static final int MAX_SLEEP_SEC = 12;

        public SleepingTask() {
            mSleepSeconds = sRandom.nextInt(MAX_SLEEP_SEC - MIN_SLEEP_SEC) + MIN_SLEEP_SEC;            
        }

        @Override
        protected Integer doInBackground(Void... params) {
            for (int i = 0; i < mSleepSeconds; i++) {
                try {
                    // Crude timings, just for demonstration purposes.
                    Thread.sleep(1000);
                } catch (InterruptedException e) {
                }
                if (isCancelled()) {
                    return i + 1;
                }
                publishProgress(i + 1);
            }
            return mSleepSeconds;
        }
    }
}
