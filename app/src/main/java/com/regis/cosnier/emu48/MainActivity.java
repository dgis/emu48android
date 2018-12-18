package com.regis.cosnier.emu48;

import android.Manifest;
import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.MotionEvent;
import android.view.ViewGroup;
import com.google.android.material.floatingactionbutton.FloatingActionButton;
import com.google.android.material.navigation.NavigationView;
import com.google.android.material.snackbar.Snackbar;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.ActionBarDrawerToggle;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.app.ActivityCompat;
import androidx.core.view.GravityCompat;
import androidx.drawerlayout.widget.DrawerLayout;

import android.view.View;
import android.view.Menu;
import android.view.MenuItem;

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class MainActivity extends AppCompatActivity
        implements NavigationView.OnNavigationItemSelectedListener {

    public static final int INTENT_GETOPENFILENAME = 1;
    public static final int INTENT_GETSAVEFILENAME = 2;
    public static final int INTENT_OBJECT_LOAD = 3;
    public static final int INTENT_OBJECT_SAVE = 4;
    public static final int INTENT_SETTINGS = 5;
    public static final int INTENT_PORT2LOAD = 6;

    public static MainActivity mainActivity;

    private static final String TAG = "MainActivity";
    private MainScreenView mainScreenView;
    SharedPreferences sharedPreferences;
    SharedPreferences.OnSharedPreferenceChangeListener sharedPreferenceChangeListener;

    private final static int PERMISSIONS_REQUEST_WRITE_EXTERNAL_STORAGE = 2;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        final Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        FloatingActionButton fab = (FloatingActionButton) findViewById(R.id.fab);
        fab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Snackbar.make(view, "Replace with your own action", Snackbar.LENGTH_LONG)
                        .setAction("Action", null).show();
            }
        });

        DrawerLayout drawer = (DrawerLayout) findViewById(R.id.drawer_layout);
        ActionBarDrawerToggle toggle = new ActionBarDrawerToggle(
                this, drawer, toolbar, R.string.navigation_drawer_open, R.string.navigation_drawer_close);
        drawer.addDrawerListener(toggle);
        toggle.syncState();

        NavigationView navigationView = (NavigationView) findViewById(R.id.nav_view);
        navigationView.setNavigationItemSelectedListener(this);



        mainActivity = this;


        ViewGroup mainScreenContainer = (ViewGroup)findViewById(R.id.main_screen_container);
        mainScreenView = new MainScreenView(this); //, currentProject);
//        mainScreenView.setOnTouchListener(new View.OnTouchListener() {
//            @Override
//            public boolean onTouch(View view, MotionEvent motionEvent) {
//                if (motionEvent.getAction() == MotionEvent.ACTION_DOWN){
//                    if(motionEvent.getY() < 0.3f * mainScreenView.getHeight()) {
//                        if(toolbar.getVisibility() == View.GONE)
//                            toolbar.setVisibility(View.VISIBLE);
//                        else
//                            toolbar.setVisibility(View.GONE);
//                        return true;
//                    }
//                }
//                return false;
//            }
//        });
        toolbar.setVisibility(View.GONE);
        mainScreenContainer.addView(mainScreenView, 0);

        sharedPreferences = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
        updateFromPreferences(null, false);
        sharedPreferenceChangeListener = new SharedPreferences.OnSharedPreferenceChangeListener() {
            @Override
            public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
                updateFromPreferences(key, true);
            }
        };
        sharedPreferences.registerOnSharedPreferenceChangeListener(sharedPreferenceChangeListener);


        AssetManager assetManager = getResources().getAssets();
        NativeLib.start(assetManager, mainScreenView.getBitmapMainScreen(), this, mainScreenView);

        //https://developer.android.com/guide/topics/providers/document-provider#permissions
        String lastDocumentUrl = sharedPreferences.getString("lastDocument", "");
        if(lastDocumentUrl.length() > 0)
//            try {
//                if(ActivityCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
//                    ActivityCompat.requestPermissions(this, new String[]{ Manifest.permission.WRITE_EXTERNAL_STORAGE }, PERMISSIONS_REQUEST_WRITE_EXTERNAL_STORAGE);
//                    //return;
//                } else {
                    onFileOpen(lastDocumentUrl);
//                }
//            } catch (Exception e) {
//                Log.e(TAG, e.getMessage());
//            }
    }

//    @Override
//    public void onRequestPermissionsResult(int requestCode, @NonNull String permissions[], @NonNull int[] grantResults) {
//        switch (requestCode) {
//            case PERMISSIONS_REQUEST_WRITE_EXTERNAL_STORAGE: {
//                // If request is cancelled, the result arrays are empty.
////				if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
////
////				} else {
////					//Toast.makeText(this, R.string.toast_access_location_denied, Toast.LENGTH_SHORT).show();
////				}
//                String lastDocumentUrl = sharedPreferences.getString("lastDocument", "");
//                if(lastDocumentUrl.length() > 0)
//                    onFileOpen(lastDocumentUrl);
////				return;
//            }
////			default:
////				super.onRequestPermissionsResult(requestCode, permissions, grantResults);
//        }
//    }

    @Override
    public void onBackPressed() {
        DrawerLayout drawer = (DrawerLayout) findViewById(R.id.drawer_layout);
        if (drawer.isDrawerOpen(GravityCompat.START)) {
            drawer.closeDrawer(GravityCompat.START);
        } else {
            super.onBackPressed();
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            OnSettings();
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    @SuppressWarnings("StatementWithEmptyBody")
    @Override
    public boolean onNavigationItemSelected(MenuItem item) {
        // Handle navigation view item clicks here.
        int id = item.getItemId();

        if (id == R.id.nav_new) {
            OnFileNew();
        } else if (id == R.id.nav_open) {
            OnFileOpen();
        } else if (id == R.id.nav_save) {
            OnFileSave();
        } else if (id == R.id.nav_save_as) {
            OnFileSaveAs();
        } else if (id == R.id.nav_close) {
            OnFileClose();
        } else if (id == R.id.nav_settings) {
            OnSettings();
        } else if (id == R.id.nav_load_object) {
            OnObjectLoad();
        } else if (id == R.id.nav_save_object) {
            OnObjectSave();
        } else if (id == R.id.nav_copy_screen) {
            OnViewCopy();
        } else if (id == R.id.nav_copy_stack) {
            OnStackCopy();
        } else if (id == R.id.nav_paste_stack) {
            OnStackPaste();
        } else if (id == R.id.nav_reset_calculator) {
            OnViewReset();
        } else if (id == R.id.nav_backup_save) {
            OnBackupSave();
        } else if (id == R.id.nav_backup_restore) {
            OnBackupRestore();
        } else if (id == R.id.nav_backup_delete) {
            OnBackupDelete();
        } else if (id == R.id.nav_change_kml_script) {
            OnViewScript();
        } else if (id == R.id.nav_help) {
            OnTopics();
        } else if (id == R.id.nav_about) {
            OnAbout();
        }

        DrawerLayout drawer = (DrawerLayout) findViewById(R.id.drawer_layout);
        drawer.closeDrawer(GravityCompat.START);
        return true;
    }

    class KMLScriptItem {
        public String filename;
        public String title;
        public String model;
    }
    ArrayList<KMLScriptItem> kmlScripts;

    private void OnFileNew() {
        if(kmlScripts == null) {
            kmlScripts = new ArrayList<>();
            AssetManager assetManager = getAssets();
            String[] calculatorsAssetFilenames = new String[0];
            try {
                calculatorsAssetFilenames = assetManager.list("calculators");
            } catch (IOException e) {
                e.printStackTrace();
            }
            String cKmlType = null; //"S";
            kmlScripts.clear();
            Pattern patternGlobalTitle = Pattern.compile("\\s*Title\\s+\"(.*)\"");
            Pattern patternGlobalModel = Pattern.compile("\\s*Model\\s+\"(.*)\"");
            Matcher m;
            for (String calculatorsAssetFilename : calculatorsAssetFilenames) {
                if (calculatorsAssetFilename.toLowerCase().lastIndexOf(".kml") != -1) {
                    BufferedReader reader = null;
                    try {
                        reader = new BufferedReader(new InputStreamReader(assetManager.open("calculators/" + calculatorsAssetFilename), "UTF-8"));
                        // do reading, usually loop until end of file reading
                        String mLine;
                        boolean inGlobal = false;
                        String title = null;
                        String model = null;
                        while ((mLine = reader.readLine()) != null) {
                            //process line
                            if (mLine.indexOf("Global") == 0) {
                                inGlobal = true;
                                title = null;
                                model = null;
                                continue;
                            }
                            if (inGlobal) {
                                if (mLine.indexOf("End") == 0) {
                                    KMLScriptItem newKMLScriptItem = new KMLScriptItem();
                                    newKMLScriptItem.filename = calculatorsAssetFilename;
                                    newKMLScriptItem.title = title;
                                    newKMLScriptItem.model = model;
                                    kmlScripts.add(newKMLScriptItem);
                                    title = null;
                                    model = null;
                                    break;
                                }

                                m = patternGlobalTitle.matcher(mLine);
                                if (m.find()) {
                                    title = m.group(1);
                                }
                                m = patternGlobalModel.matcher(mLine);
                                if (m.find()) {
                                    model = m.group(1);
                                }
                            }
                        }
                    } catch (IOException e) {
                        //log the exception
                        e.printStackTrace();
                    } finally {
                        if (reader != null) {
                            try {
                                reader.close();
                            } catch (IOException e) {
                                //log the exception
                            }
                        }
                    }
                }
            }
            Collections.sort(kmlScripts, new Comparator<KMLScriptItem>() {
                @Override
                public int compare(KMLScriptItem lhs, KMLScriptItem rhs) {
                    return lhs.title.compareTo(rhs.title);
                }
            });
        }


        final String[] kmlScriptTitles = new String[kmlScripts.size()];
        for (int i = 0; i < kmlScripts.size(); i++)
            kmlScriptTitles[i] = kmlScripts.get(i).title;
        new AlertDialog.Builder(this)
            .setTitle("Pick a calculator")
            .setItems(kmlScriptTitles, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    String kmlScriptFilename = kmlScripts.get(which).filename;
                    NativeLib.onFileNew(kmlScriptFilename);
                    showKMLLog();
                }
            }).show();
    }

    private void OnFileOpen() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        //intent.setType("YOUR FILETYPE"); //not needed, but maybe usefull
        intent.setType("*/*");
        intent.putExtra(Intent.EXTRA_TITLE, "emu48-state.e48");
        startActivityForResult(intent, INTENT_GETOPENFILENAME);
    }
    private void OnFileSave() {
        NativeLib.onFileSave();
    }
    private void OnFileSaveAs() {
        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        int model = NativeLib.getCurrentModel();
        String extension = "e48"; // HP48SX/GX
        switch (model) {
            case '6':
            case 'A':
                extension = "e38"; // HP38G
                break;
            case 'E':
                extension = "e39"; // HP39/40G
                break;
            case 'X':
                extension = "e49"; // HP49G
                break;
        }
        intent.putExtra(Intent.EXTRA_TITLE, "emu48-state." + extension);
        startActivityForResult(intent, INTENT_GETSAVEFILENAME);
    }
    private void OnFileClose() {
        NativeLib.onFileClose();
        SharedPreferences.Editor editor = sharedPreferences.edit();
        editor.putString("lastDocument", "");
        editor.commit();
    }
    private void OnSettings() {
        startActivityForResult(new Intent(this, SettingsActivity.class), INTENT_SETTINGS);
    }

    private void openDocument() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        // //Intent.setType("application/*|text/*");
        // String[] mimeTypes = {
        //     "text/plain",
        //     "application/pdf",
        //     "application/zip"
        // };
        // intent.putExtra(Intent.EXTRA_MIME_TYPES, mimeTypes);
        intent.putExtra(Intent.EXTRA_TITLE, "emu48-object.hp");
        startActivityForResult(intent, INTENT_OBJECT_LOAD);
    }

    private void OnObjectLoad() {
        if(sharedPreferences.getBoolean("settings_objectloadwarning", false)) {
            DialogInterface.OnClickListener onClickListener = new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    if (which == DialogInterface.BUTTON_POSITIVE) {
                        openDocument();
                    }
                }
            };
            new AlertDialog.Builder(this)
                    .setMessage("Warning: Trying to load an object while the emulator is busy\n" +
                            "will certainly result in a memory lost. Before loading an object\n" +
                            "you should be sure that the calculator is in idle state.\n" +
                            "Do you want to see this warning next time you try to load an object?")
                    .setPositiveButton(android.R.string.yes, onClickListener)
                    .setNegativeButton(android.R.string.no, onClickListener)
                    .show();
        } else
            openDocument();
    }
    private void OnObjectSave() {
        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        intent.putExtra(Intent.EXTRA_TITLE, "emu48-object.hp");
        startActivityForResult(intent, INTENT_OBJECT_SAVE);
    }
    private void OnViewCopy() {
        NativeLib.onViewCopy();

    }
    private void OnStackCopy() {
        //https://developer.android.com/guide/topics/text/copy-paste
        NativeLib.onStackCopy();

    }
    private void OnStackPaste() {
        NativeLib.onStackPaste();

    }
    private void OnViewReset() {
        NativeLib.onViewReset();

    }
    private void OnBackupSave() {
        NativeLib.onBackupSave();

    }
    private void OnBackupRestore() {
        NativeLib.onBackupRestore();

    }
    private void OnBackupDelete() {
        NativeLib.onBackupDelete();

    }
    private void OnViewScript() {
        //NativeLib.onViewScript();
    }
    private void OnTopics() {
    }
    private void OnAbout() {
    }

    @Override
    protected void onDestroy() {

        NativeLib.stop();

        super.onDestroy();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        if(resultCode == Activity.RESULT_OK) {

            if(requestCode == INTENT_GETOPENFILENAME) {
                Uri uri = data.getData();

                //just as an example, I am writing a String to the Uri I received from the user:
                Log.d(TAG, "onActivityResult INTENT_GETOPENFILENAME " + uri.toString());

                String url = uri.toString();
                if(onFileOpen(url) != 0) {
                    SharedPreferences.Editor editor = sharedPreferences.edit();
                    editor.putString("lastDocument", url);
                    editor.commit();

                    makeUriPersistable(data, uri);
                }
            } else if(requestCode == INTENT_GETSAVEFILENAME) {
                Uri uri = data.getData();

                //just as an example, I am writing a String to the Uri I received from the user:
                Log.d(TAG, "onActivityResult INTENT_GETSAVEFILENAME " + uri.toString());

                String url = uri.toString();
                if(NativeLib.onFileSaveAs(url) != 0) {
                    SharedPreferences.Editor editor = sharedPreferences.edit();
                    editor.putString("lastDocument", url);
                    editor.commit();

                    makeUriPersistable(data, uri);
                }
            } else if(requestCode == INTENT_OBJECT_LOAD) {
                Uri uri = data.getData();

                //just as an example, I am writing a String to the Uri I received from the user:
                Log.d(TAG, "onActivityResult INTENT_OBJECT_LOAD " + uri.toString());

                String url = uri.toString();
                NativeLib.onObjectLoad(url);
            } else if(requestCode == INTENT_OBJECT_SAVE) {
                Uri uri = data.getData();

                //just as an example, I am writing a String to the Uri I received from the user:
                Log.d(TAG, "onActivityResult INTENT_OBJECT_SAVE " + uri.toString());

                String url = uri.toString();
                NativeLib.onObjectSave(url);
            } else if(requestCode == INTENT_SETTINGS) {

            }
        }
        super.onActivityResult(requestCode, resultCode, data);
    }

    private void makeUriPersistable(Intent data, Uri uri) {
        //grantUriPermission(getPackageName(), uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
        final int takeFlags = data.getFlags() & (Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        getContentResolver().takePersistableUriPermission(uri, takeFlags);
    }

    private int onFileOpen(String url) {
        int result = NativeLib.onFileOpen(url);
        showKMLLog();
        return result;
    }

    private void showKMLLog() {
        if(sharedPreferences.getBoolean("settings_alwaysdisplog", true)) {
            String kmlLog = NativeLib.getKMLLog();
            new AlertDialog.Builder(this)
                    .setTitle("KML Script Compilation Result")
                    .setMessage(kmlLog)
                    .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                        }
                    }).show();
        }
    }

    final int GENERIC_READ   = 1;
    final int GENERIC_WRITE  = 2;
    int openFileFromContentResolver(String url, int writeAccess) {
        //https://stackoverflow.com/a/31677287
        Uri uri = Uri.parse(url);
        ParcelFileDescriptor filePfd;
        try {
            String mode = "";
            if((writeAccess & GENERIC_READ) == GENERIC_READ)
                mode += "r";
            if((writeAccess & GENERIC_WRITE) == GENERIC_WRITE)
                mode += "w";
            filePfd = getContentResolver().openFileDescriptor(uri, mode);
        } catch (Exception e) {
            e.printStackTrace();
            return -1;
        }
        int fd = filePfd != null ? filePfd.getFd() : 0;
        return fd;
    }

    private void updateFromPreferences(String key, boolean isDynamic) {
        int isDynamicValue = isDynamic ? 1 : 0;
        if(key == null) {
//        boolean settingsAutosave = sharedPreferences.getBoolean("settings_autosave", false);
//        boolean settingsAutosaveonexit = sharedPreferences.getBoolean("settings_autosaveonexit", false);
            String[] settingKeys = { "settings_realspeed", "settings_grayscale", "settings_alwaysdisplog", "settings_port1", "settings_port2" };
            for (String settingKey : settingKeys) {
                updateFromPreferences(settingKey, false);
            }
        } else {
            switch (key) {
                case "settings_realspeed":
                    NativeLib.setConfiguration(key, isDynamicValue, sharedPreferences.getBoolean(key, false) ? 1 : 0, 0, null);
                    break;
                case "settings_grayscale":
                    NativeLib.setConfiguration(key, isDynamicValue, sharedPreferences.getBoolean(key, false) ? 1 : 0, 0, null);
                    break;
                case "settings_alwaysdisplog":
                    NativeLib.setConfiguration(key, isDynamicValue, sharedPreferences.getBoolean(key, true) ? 1 : 0, 0, null);
                    break;
                case "settings_port1":
                case "settings_port1en":
                case "settings_port1wr":
                    NativeLib.setConfiguration("settings_port1", isDynamicValue,
                            sharedPreferences.getBoolean("settings_port1en", false) ? 1 : 0,
                            sharedPreferences.getBoolean("settings_port1wr", false) ? 1 : 0,
                            null);
                    break;
                case "settings_port2":
                case "settings_port2en":
                case "settings_port2wr":
                case "settings_port2load":
                    NativeLib.setConfiguration("settings_port2", isDynamicValue,
                            sharedPreferences.getBoolean("settings_port2en", false) ? 1 : 0,
                            sharedPreferences.getBoolean("settings_port2wr", false) ? 1 : 0,
                            sharedPreferences.getString("settings_port2load", ""));
                    break;
            }
        }
    }
}
