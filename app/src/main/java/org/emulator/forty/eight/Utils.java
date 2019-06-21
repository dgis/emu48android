package org.emulator.forty.eight;

import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.provider.OpenableColumns;
import android.util.Log;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;

public class Utils {
    static String getFileName(Context context, String url) {
        String result = null;
        try {
            Uri uri = Uri.parse(url);
            if(uri != null) {
                String scheme = uri.getScheme();
                if (scheme != null && scheme.equals("content")) {
                    Cursor cursor = context.getContentResolver().query(uri, null, null, null, null);
                    if(cursor != null) {
                        try {
                            if (cursor.moveToFirst())
                                result = cursor.getString(cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME));
                        } finally {
                            cursor.close();
                        }
                    }
                }
                if (result == null) {
                    result = uri.getPath();
                    if(result != null) {
                        int cut = result.lastIndexOf('/');
                        if (cut != -1) {
                            result = result.substring(cut + 1);
                        }
                    }
                }
            }
        } catch (Exception ex) {
            result = url;
        }

        return result;
    }

    // https://community.khronos.org/t/get-maximum-texture-size/67795
    static int getMaximumTextureSize() {
        EGL10 egl = (EGL10) EGLContext.getEGL();
        EGLDisplay display = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);

        // Initialise
        int[] version = new int[2];
        egl.eglInitialize(display, version);

        // Query total number of configurations
        int[] totalConfigurations = new int[1];
        egl.eglGetConfigs(display, null, 0, totalConfigurations);

        // Query actual list configurations
        EGLConfig[] configurationsList = new EGLConfig[totalConfigurations[0]];
        egl.eglGetConfigs(display, configurationsList, totalConfigurations[0], totalConfigurations);

        int[] textureSize = new int[1];
        int maximumTextureSize = 0;

        // Iterate through all the configurations to located the maximum texture size
        for (int i = 0; i < totalConfigurations[0]; i++)
        {
            // Only need to check for width since opengl textures are always squared
            egl.eglGetConfigAttrib(display, configurationsList[i], EGL10.EGL_MAX_PBUFFER_WIDTH, textureSize);

            // Keep track of the maximum texture size
            if (maximumTextureSize < textureSize[0])
            {
                maximumTextureSize = textureSize[0];
            }

            Log.i("GLHelper", Integer.toString(textureSize[0]));
        }

        // Release
        egl.eglTerminate(display);
        Log.i("GLHelper", "Maximum GL texture size: " + Integer.toString(maximumTextureSize));

        return maximumTextureSize;
    }
}
