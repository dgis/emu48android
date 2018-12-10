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
    //public static native void resize(int width, int height);
    public static native void draw();
    public static native void buttonDown(int x, int y);
    public static native void buttonUp(int x, int y);
    public static native void keyDown(int virtKey);
    public static native void keyUp(int virtKey);


    public static native String getCurrentFilename();

    public static native void onFileNew();
    public static native void onFileOpen(String filename);
    public static native void onFileSave();
    public static native void onFileSaveAs(String newFilename);
    public static native void onFileClose();
    public static native void onObjectLoad();
    public static native void onObjectSave();
    public static native void onViewCopy();
    public static native void onStackCopy();
    public static native void onStackPaste();
    public static native void onViewReset();
    public static native void onBackupSave();
    public static native void onBackupRestore();
    public static native void onBackupDelete();
}
