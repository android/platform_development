package com.android.vndk;
public class Test {
    final static String LIB_NAME = "libc";
    static String mLibName = "libd";
    public static void main(String args[]) {
        String libName;
        if (args.length > 5) {
            libName = "liba";
        } else {
            libName = "libb";
        }
        System.loadLibrary(libName);
        System.loadLibrary(LIB_NAME);
        System.loadLibrary(mLibName);
        System.loadLibrary("libfoo");
    }
}

