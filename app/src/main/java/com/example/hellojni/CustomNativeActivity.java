package com.example.hellojni;

public class CustomNativeActivity extends android.app.NativeActivity {

    static {
        System.loadLibrary("jxcore");
        System.loadLibrary("simple");
    }

}