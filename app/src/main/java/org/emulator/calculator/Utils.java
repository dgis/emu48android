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

import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.provider.OpenableColumns;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.Toast;

import androidx.fragment.app.Fragment;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;

public class Utils {

    public static void showAlert(Context context, String text) {
        Toast toast = Toast.makeText(context, text, Toast.LENGTH_SHORT);
        //View view = toast.getView();
        //view.setBackgroundColor(0x80000000);
        toast.show();
    }

    static int resId(Context context, String resourcename, String variableName)
    {
        try {
            return context.getResources().getIdentifier(variableName, resourcename, context.getApplicationContext().getPackageName());
        } catch (Exception e) {
            e.printStackTrace();
            return -1;
        }
    }

    static int resId(Fragment fragment, String resourcename, String variableName)
    {
        try {
            Context context = fragment.getContext();
            if(context != null)
                return fragment.getResources().getIdentifier(variableName, resourcename, context.getApplicationContext().getPackageName());
        } catch (Exception e) {
            e.printStackTrace();
        }
        return -1;
    }

    public static String getFileName(Context context, String url) {
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
        Log.i("GLHelper", "Maximum GL texture size: " + maximumTextureSize);

        return maximumTextureSize;
    }

    public static void setListViewHeightBasedOnChildren(ListView listView) {
        ListAdapter listAdapter = listView.getAdapter();
        if (listAdapter == null) {
            // pre-condition
            return;
        }

        int totalHeight = 0;
        for (int i = 0; i < listAdapter.getCount(); i++) {
            View listItem = listAdapter.getView(i, null, listView);
            listItem.measure(0, 0);
            totalHeight += listItem.getMeasuredHeight();
        }

        ViewGroup.LayoutParams params = listView.getLayoutParams();
        params.height = totalHeight + (listView.getDividerHeight() * (listAdapter.getCount() - 1));
        listView.setLayoutParams(params);
        listView.requestLayout();
    }
}
