/*
 * Copyright (C) 2008 The Android Open Source Project
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

package com.android.prefs;

import java.io.File;

/**
 * Manages the location of the android files (including emulator files, ddms config, debug keystore)
 */
public final class AndroidLocation {
    
    /**
     * Used to know where to store the user data image.
     * <p/>
     * This <em>must</em> match the constant ANDROID_SDK_VERSION used by the emulator
     * to find its own emulator images. It is defined in tools/qemu/android.h
     */
    private static final String ANDROID_SDK_VERSION = "SDK-1.0";

    /**
     * VM folder inside the path returned by {@link #getFolder()}
     */
    public static final String FOLDER_VMS = "vm";

    /**
     * Throw when the location of the android folder couldn't be found.
     */
    public static final class AndroidLocationException extends Exception {
        private static final long serialVersionUID = 1L;

        public AndroidLocationException(String string) {
            super(string);
        }
    }
    
    private static String sPrefsLocation = null;
    
    /**
     * Returns the folder used to store android related files.
     * @return an OS specific path, terminated by a separator.
     * @throws AndroidLocationException 
     */
    public final static String getFolder() throws AndroidLocationException {
        if (sPrefsLocation == null) {
            String home = findValidPath("user.home", "HOME");
            
            // if the above failed, we throw an exception.
            if (home == null) {
                throw new AndroidLocationException(
                        "Unable to get the home directory. Make sure the user.home property is set up");
            } else {
                sPrefsLocation = home + File.separator + ".android" + File.separator;

                // make sure the folder exists!
                File f = new File(sPrefsLocation);
                if (f.exists() == false) {
                    f.mkdir();
                } else if (f.isFile()) {
                    throw new AndroidLocationException(sPrefsLocation +
                            " is not a directory! This is required to run Android tools.");
                }
            }
        }

        return sPrefsLocation;
    }

    /**
     * Returns the folder where the emulator is going to find its android related files.
     * @return an OS specific path, terminated by a separator.
     * @throws AndroidLocationException 
     */
    public final static String getEmulatorFolder() throws AndroidLocationException {
        String path = getFolder() + ANDROID_SDK_VERSION + File.separator;
        
        File f = new File(path);
        if (f.exists() == false) {
            f.mkdir();
        } else if (f.isFile()) {
            throw new AndroidLocationException(path +
                    " is not a directory! This is required to run Android tools.");
        }

        return path;
    }

    /**
     * Checks a list of system properties and/or system environment variables for validity, and
     * existing director, and returns the first one.
     * @param names
     * @return the content of the first property/variable.
     */
    private static String findValidPath(String... names) {
        for (String name : names) {
            String path;
            if (name.indexOf('.') != -1) {
                path = System.getProperty(name);
            } else {
                path = System.getenv(name);
            }

            if (path != null) {
                File f = new File(path);
                if (f.isDirectory()) {
                    return path;
                }
            }
        }
        
        return null;
    }
}
