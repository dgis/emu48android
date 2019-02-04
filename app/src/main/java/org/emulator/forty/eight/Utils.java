package org.emulator.forty.eight;

import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.provider.DocumentsContract;
import android.provider.MediaStore;
import android.provider.OpenableColumns;

public class Utils {
    public static String getFileName(Context context, String url) {
        Uri uri = Uri.parse(url);
        String result = null;
        if (uri.getScheme().equals("content")) {
            Cursor cursor = context.getContentResolver().query(uri, null, null, null, null);
            try {
                if (cursor != null && cursor.moveToFirst())
                    result = cursor.getString(cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME));
            } finally {
                cursor.close();
            }
        }
        if (result == null) {
            result = uri.getPath();
            int cut = result.lastIndexOf('/');
            if (cut != -1) {
                result = result.substring(cut + 1);
            }
        }
        return result;
    }

    public static String getFilePath(Context context, String url) {
        Uri uri = Uri.parse(url);
        String result = null;
        if (uri.getScheme().equals("content")) {
//            //Cursor cursor = context.getContentResolver().query(uri, new String[] { android.provider.MediaStore.Images.ImageColumns.DATA }, null, null, null);
//            Cursor cursor = context.getContentResolver().query(uri, new String[] { android.provider.MediaStore.Files.FileColumns.DATA }, null, null, null);
//            try {
//                if (cursor != null && cursor.moveToFirst()) {
//                    result = cursor.getString(0);
//                    //result = cursor.getString(cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME));
//                }
//            } finally {
//                cursor.close();
//            }

//https://stackoverflow.com/questions/13209494/how-to-get-the-full-file-path-from-uri
//            String wholeID = DocumentsContract.getDocumentId(uri);
//            String id = wholeID.split(":")[1];
//            String[] column = { MediaStore.Images.Media.DATA };
//            //String[] column = { MediaStore.Files.FileColumns.DATA };
//            String sel = MediaStore.Images.Media._ID + "=?";
//            Cursor cursor = context.getContentResolver().query(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, column, sel, new String[]{ id }, null);
//            //Cursor cursor = context.getContentResolver().query(MediaStore.Files.get, column, sel, new String[]{ id }, null);
//            int columnIndex = cursor.getColumnIndex(column[0]);
//            if (cursor.moveToFirst()) {
//                result = cursor.getString(columnIndex);
//            }
//            cursor.close();


            Cursor cursor = context.getContentResolver().query(uri, null, null, null, null);
            try {
                if (cursor != null && cursor.moveToFirst()) {
                    result = cursor.getString(0);
                    //result = cursor.getString(cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME));
                }
            } finally {
                cursor.close();
            }
        }
        if (result == null)
            result = url;
        return result;
    }
}
