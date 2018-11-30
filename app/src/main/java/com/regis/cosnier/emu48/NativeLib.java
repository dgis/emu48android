package com.regis.cosnier.emu48;

import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.view.View;

public class NativeLib {

    static {
        System.loadLibrary("native-lib");
    }

    public static native void start(AssetManager mgr, Bitmap bitmapMainScreen, MainScreenView view);
    public static native void stop();
    public static native void resize(int width, int height);
}
