package org.emulator.forty.eight;

import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.provider.OpenableColumns;

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
}
