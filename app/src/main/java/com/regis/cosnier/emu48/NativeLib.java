package com.regis.cosnier.emu48;

import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.os.ParcelFileDescriptor;
import android.view.View;

public class NativeLib {

    static {
        System.loadLibrary("native-lib");
    }

    public static native void start(AssetManager mgr, Bitmap bitmapMainScreen, MainActivity activity, MainScreenView view);
    public static native void stop();
    public static native void draw();
    public static native void buttonDown(int x, int y);
    public static native void buttonUp(int x, int y);
    public static native void keyDown(int virtKey);
    public static native void keyUp(int virtKey);


    public static native boolean isDocumentAvailable();
    public static native String getCurrentFilename();
    public static native int getCurrentModel();
    public static native boolean isBackup();
    public static native String getKMLLog();
    public static native String getKMLTitle();

    public static native int onFileNew(String kmlFilename);
    public static native int onFileOpen(String filename);
    public static native int onFileSave();
    public static native int onFileSaveAs(String newFilename);
    public static native int onFileClose();
    public static native int onObjectLoad(String filename);
    public static native int onObjectSave(String filename);
    public static native void onViewCopy(Bitmap bitmapScreen);
    public static native void onStackCopy();
    public static native void onStackPaste();
    public static native void onViewReset();
    public static native void onViewScript(String kmlFilename);
    public static native void onBackupSave();
    public static native void onBackupRestore();
    public static native void onBackupDelete();

    public static native void setConfiguration(String key, int isDynamic, int intValue1, int intValue2, String stringValue);
    public static native boolean isPortExtensionPossible();
    public static native int getState();
    public static native int getScreenWidth();
    public static native int getScreenHeight();
}
