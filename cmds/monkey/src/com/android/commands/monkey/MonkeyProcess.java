package com.android.commands.monkey;

/**
 * Class defining special process settings for Monkey application.
 */
public class MonkeyProcess {

    /**
     * Loads the monkeyprocess library.
     */
    static {
        System.loadLibrary("monkeyprocess");
    }

    /**
     * Creates a new process session for Monkey process and makes
     * Monkey the session and group leader. Monkey will continue
     * execution as stand-alone application without controlling
     * terminal and therefore it receives no SIGHUP signal
     * (which might be the case when having controlling terminal
     * the terminal discovers remote client disconnection).
     *
     * This system call triggering was introduced to avoid Monkey
     * application crashes due to receiving SIGHUP signal from its
     * controlling terminal after the Monkey - MonkeyRunner
     * connection termination.
     *
     * {@hide}
     */
    public static final native void createNewProcessSession();

}
