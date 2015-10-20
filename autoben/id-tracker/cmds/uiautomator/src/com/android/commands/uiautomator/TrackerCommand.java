/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.commands.uiautomator;

import android.app.UiAutomation;
import android.graphics.Point;
import android.graphics.Rect;
import android.util.MutableInt;
import android.util.Pair;
import android.view.accessibility.AccessibilityNodeInfo;

import com.android.commands.uiautomator.Launcher.Command;
import com.android.uiautomator.core.UiAutomationShellWrapper;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.Stack;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Implementation of the tracker subcommand.
 * <p/>
 * Log UI elements' resource ID and the path from root in layout.
 */
public class TrackerCommand extends Command {

    private UiAutomation mUiAutomation;
    private Object mQuitLock = new Object();
    private int mMaxX, mMaxY, mDisplayX, mDisplayY;
    private double mScaleX, mScaleY;
    private String mTouchDevice;

    public TrackerCommand() {
        super("tracker");
    }

    @Override
    public String shortHelp() {
        return "log targeted object's resource-id and position in tree";
    }

    @Override
    public String detailedOptions() {
        return null;
    }

    @Override
    public void run(String[] args) {
        UiAutomationShellWrapper automationWrapper = new UiAutomationShellWrapper();
        automationWrapper.connect();
        mUiAutomation = automationWrapper.getUiAutomation();

        setUp();

        readEvent();

        // There's really no way to stop, essentially we just block indefinitely here and wait
        // for user to press Ctrl+C.
        synchronized (mQuitLock) {
            try {
                mQuitLock.wait();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        automationWrapper.disconnect();
    }

    private static final String POSITION_X = "ABS_MT_POSITION_X";
    private static final String POSITION_Y = "ABS_MT_POSITION_Y";
    private static final String TOUCH_TYPE_A = "BTN_TOUCH";
    private static final String TOUCH_TYPE_B = "ABS_MT_TRACKING_ID";
    private static final String TOUCH_DOWN_TYPE_A = "DOWN";
    private static final String TOUCH_UP_TYPE_B = "ffffffff";

    private void readEvent() {

        String cmdLine[] = {"getevent2", "-lt", mTouchDevice};
        String line;
        boolean startTouch = false;
        int positionX = 0, positionY = 0;

        try {
            Process process = Runtime.getRuntime().exec(cmdLine);

            try (BufferedReader reader = new BufferedReader(new InputStreamReader(
                    process.getInputStream()))) {

                while ((line = reader.readLine()) != null) {
                    line = line.trim();
                    String[] element = line.split(" ");
                    List elementList = Arrays.asList(element);
                    Set<String> elementSet = new HashSet<String>(elementList);

                    // Only get the first input coordinate after touch down event.
                    if (startTouch) {
                        // Try getting x, y.
                        if (elementSet.contains(POSITION_X)) {
                            positionX = Integer.parseInt(element[element.length - 1], 16);
                            positionX = (int) (positionX * mScaleX);
                        } else if (elementSet.contains(POSITION_Y)) {
                            positionY = Integer.parseInt(element[element.length - 1], 16);
                            positionY = (int) (positionY * mScaleY);
                            Stack<Pair<Integer, String>> bestPath = new Stack();
                            Stack<Pair<Integer, String>> path = new Stack();
                            MutableInt minArea = new MutableInt(Integer.MAX_VALUE);
                            Point position = new Point(positionX, positionY);
                            getSmallestNodeOnPosition(bestPath,
                                    mUiAutomation.getRootInActiveWindow(), 0, path, position,
                                    minArea);
                            outputTouchedElements(bestPath, position);
                            startTouch = false;
                        }
                    } else {
                        // Expect a touch down event to set startTouch.
                        // For type-A touch down event, we expect TOUCH_DOWN_TYPE_A.
                        // For type-B touch down event, we expect not to see TOUCH_UP_TYPE_B.
                        if (elementSet.contains(TOUCH_TYPE_A) && elementSet.contains(
                                TOUCH_DOWN_TYPE_A)) {
                            startTouch = true;
                        } else if (elementSet.contains(TOUCH_TYPE_B) && !elementSet.contains(
                                TOUCH_UP_TYPE_B)) {
                            startTouch = true;
                        }
                    }
                }
            } catch (IOException e) {
                e.printStackTrace();
            }

            process.waitFor();

        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void outputTouchedElements(Stack<Pair<Integer, String>> result, Point position) {

        System.out.println("==============================");
        System.out.println(String.format("(x, y): (%d, %d)", position.x, position.y));
        StringBuilder indent = new StringBuilder();

        for (Pair<Integer, String> x : result) {
            // x.second-th child of its parent
            System.out.println(String.format("%s %d %s", indent.toString(), x.first, x.second));
            indent.append(" ");
        }
    }

    /**
     * Searches the smallest element in which the position is in the boundary.
     *
     * @param bestPath    stores the best path from root node to the smallest UI element which
     *                    contains the input coordinates.
     * @param currentNode searches the subtree of the node.
     * @param ithChild    the index-th child of its parent.
     * @param position    touched position.
     * @param minArea     the minimum area of boundary of visited elements which contains the input
     *                    coordinates.
     */
    private void getSmallestNodeOnPosition(
            Stack<Pair<Integer, String>> bestPath, AccessibilityNodeInfo currentNode, int ithChild,
            Stack<Pair<Integer, String>> path, Point position, MutableInt minArea) {
        AccessibilityNodeInfo childNode = null;

        int childCount = currentNode.getChildCount();
        path.add(Pair.create(ithChild, currentNode.getViewIdResourceName()));
        int currentNodeArea = getBoundArea(currentNode);
        if (currentNodeArea < minArea.value) {
            minArea.value = currentNodeArea;
            bestPath.clear();
            bestPath.addAll(path);
        }
        for (int i = 0; i < childCount; i++) {
            childNode = currentNode.getChild(i);
            if (childNode == null || !positionInNode(childNode, position)) {
                continue;
            }
            // Find the smallest object that contains input coordinates.
            getSmallestNodeOnPosition(bestPath, childNode, i, path, position, minArea);
        }
        path.pop();
    }

    private int getBoundArea(AccessibilityNodeInfo node) {
        Rect rect = new Rect();
        node.getBoundsInScreen(rect);
        return rect.width() * rect.height();
    }

    private boolean positionInNode(AccessibilityNodeInfo node, Point position) {
        if (node == null) {
            return false;
        }
        Rect boundRect = new Rect();
        node.getBoundsInScreen(boundRect);
        return boundRect.contains(position.x, position.y);
    }

    private void setUp() {

        mDisplayX = -1;
        mDisplayY = -1;
        getDisplayXY();
        assert mDisplayX != -1 && mDisplayY != -1 : "error finding display";

        mMaxX = -1;
        mMaxY = -1;
        mTouchDevice = null;
        getMaxXYAndTouchDevice();
        assert mMaxX != -1 && mMaxY != -1 && mTouchDevice != null : "error finding max";

        mScaleX = (float) mDisplayX / mMaxX;
        mScaleY = (float) mDisplayY / mMaxY;

        System.out.println(String.format("%d %d %d %d %s", mDisplayX, mDisplayY, mMaxX, mMaxY,
                mTouchDevice));
        System.out.println("Start tracker command:");
    }

    // Get max value of touch event coordinate and touch device.
    private void getMaxXYAndTouchDevice() {

        final String POS_X = "0035";
        final String POS_Y = "0036";
        String[] cmd = {"/system/bin/sh", "-c", "getevent -p"};
        final Pattern pattern = Pattern.compile("^(\\d+)\\s+:.*max (\\d+),");

        try {
            Process process = Runtime.getRuntime().exec(cmd);

            try (BufferedReader reader = new BufferedReader(new InputStreamReader(
                    process.getInputStream()))) {

                String line;
                while ((line = reader.readLine()) != null && (mMaxX == -1 || mMaxY == -1)) {
                    // Parse touch device id.
                    // e.g. add device 1: /dev/input/event0
                    line = line.trim();
                    if (line.contains("/dev/input/event")) {
                        String[] content = line.split(" ");
                        mTouchDevice = content[3];
                    }
                    // Parse ABS_MT_POSITION_X and ABS_MT_POSITION_Y max value.
                    // e.g. 0036  : value 0, min 0, max 2304, fuzz 0, flat 0, resolution 0
                    Matcher matcher = pattern.matcher(line);
                    if (matcher.find()) {
                        String property = matcher.group(1);
                        int maxValue = Integer.parseInt(matcher.group(2));
                        if (property.equals(POS_X)) {
                            mMaxX = maxValue;
                        } else if (property.equals(POS_Y)) {
                            mMaxY = maxValue;
                        }
                    }
                }

            } catch (IOException e) {
                e.printStackTrace();
            }

            process.waitFor();

        } catch (Exception e) {
            e.printStackTrace();
        }

    }

    // Get max value of screen display width and length
    private void getDisplayXY() {

        String[] cmd = {"/system/bin/sh", "-c", "dumpsys window | grep mOverscanScreen"};
        final Pattern pattern = Pattern.compile("\\s(\\d+)x(\\d+)");

        try {
            Process process = Runtime.getRuntime().exec(cmd);

            try (BufferedReader reader = new BufferedReader(new InputStreamReader(
                    process.getInputStream()))) {

                String line;
                while ((line = reader.readLine()) != null) {
                    line = line.trim();
                    // Parse screen pixel size
                    // e.g. mOverscanScreen=(0,0) 1536x2048
                    Matcher matcher = pattern.matcher(line);
                    if (matcher.find()) {
                        mDisplayX = Integer.parseInt(matcher.group(1));
                        mDisplayY = Integer.parseInt(matcher.group(2));
                    }
                }
            } catch (IOException e) {
                e.printStackTrace();
            }

            process.waitFor();

        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}