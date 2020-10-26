// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

package org.emulator.calculator;

import android.app.Activity;
import android.content.res.AssetManager;
import android.graphics.Bitmap;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class NativeLib {

    static {
        System.loadLibrary("native-lib");
    }

    static final int CALLBACK_TYPE_INVALIDATE = 0;
    static final int CALLBACK_TYPE_WINDOW_RESIZE = 1;

    public static native void start(AssetManager mgr, Activity activity);
    public static native void stop();
    public static native void changeBitmap(Bitmap bitmapMainScreen);
    public static native void draw();
    public static native boolean buttonDown(int x, int y);
    public static native void buttonUp(int x, int y);
    public static native void keyDown(int virtKey);
    public static native void keyUp(int virtKey);


    public static native boolean isDocumentAvailable();
    public static native String getCurrentFilename();
    public static native int getCurrentModel();
    public static native boolean isBackup();
    public static native String getKMLLog();
    public static native String getKMLTitle();
	public static native String getCurrentKml();
	public static native void setCurrentKml(String currentKml);
	public static native String getEmuDirectory();
	public static native void setEmuDirectory(String emuDirectory);
    public static native boolean getPort1Plugged();
    public static native boolean getPort1Writable();
    public static native boolean getSoundEnabled();
    public static native int getGlobalColor();
    public static native int getMacroState();

    public static native int onFileNew(String kmlFilename, String kmlFolder);
    public static native int onFileOpen(String filename, String kmlFolder);
    public static native int onFileSave();
    public static native int onFileSaveAs(String newFilename);
    public static native int onFileClose();
    public static native int onObjectLoad(String filename);

    public static native String[] getObjectsToSave();
    public static native int onObjectSave(String filename, boolean[] objectsToSaveItemChecked);
	public static native void onViewCopy(Bitmap bitmapScreen);
    public static native void onStackCopy();
    public static native void onStackPaste();
    public static native void onViewReset();
    public static native int onViewScript(String kmlFilename, String kmlFolder);
    public static native void onBackupSave();
    public static native void onBackupRestore();
    public static native void onBackupDelete();

    public static native void onToolMacroNew(String filename);
    public static native void onToolMacroPlay(String filename);
    public static native void onToolMacroStop();

	public static native boolean onLoadFlashROM(String filename);

    public static native void setConfiguration(String key, int isDynamic, int intValue1, int intValue2, String stringValue);
    public static native boolean isPortExtensionPossible();
    public static native int getState();
    public static native int getScreenPositionX();
    public static native int getScreenPositionY();
    public static native int getScreenWidth();
    public static native int getScreenHeight();
    public static native int getScreenWidthNative();
	public static native int getScreenHeightNative();
	public static native int getLCDBackgroundColor();
}
