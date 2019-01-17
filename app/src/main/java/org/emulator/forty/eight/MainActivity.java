package org.emulator.forty.eight;

import android.Manifest;
import android.app.Activity;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.SubMenu;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.google.android.material.navigation.NavigationView;
import com.google.android.material.snackbar.Snackbar;
import org.emulator.forty.eight.R;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.Date;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.ActionBarDrawerToggle;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.app.ActivityCompat;
import androidx.core.content.FileProvider;
import androidx.core.view.GravityCompat;
import androidx.drawerlayout.widget.DrawerLayout;

public class MainActivity extends AppCompatActivity implements NavigationView.OnNavigationItemSelectedListener, SharedPreferences.OnSharedPreferenceChangeListener {

    public static final int INTENT_GETOPENFILENAME = 1;
    public static final int INTENT_GETSAVEFILENAME = 2;
    public static final int INTENT_OBJECT_LOAD = 3;
    public static final int INTENT_OBJECT_SAVE = 4;
    public static final int INTENT_SETTINGS = 5;
    public static final int INTENT_PORT2LOAD = 6;

    public static MainActivity mainActivity;

    private static final String TAG = "MainActivity";
    private SharedPreferences sharedPreferences;
    private NavigationView navigationView;
    private DrawerLayout drawer;
    private MainScreenView mainScreenView;

    private final static int PERMISSIONS_REQUEST_WRITE_EXTERNAL_STORAGE = 2;

    private int MRU_ID_START = 10000;
    private int MAX_MRU = 5;
    private LinkedHashMap<String, String> mruLinkedHashMap = new LinkedHashMap<String, String>(5, 1.0f, true) {
        @Override
        protected boolean removeEldestEntry(Map.Entry eldest) {
            return size() > MAX_MRU;
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        final Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

//        FloatingActionButton fab = findViewById(R.id.fab);
//        fab.setOnClickListener(new View.OnClickListener() {
//            @Override
//            public void onClick(View view) {
//                Snackbar.make(view, "Replace with your own action", Snackbar.LENGTH_LONG)
//                        .setAction("Action", null).show();
//            }
//        });

        drawer = findViewById(R.id.drawer_layout);
        ActionBarDrawerToggle toggle = new ActionBarDrawerToggle(this, drawer, toolbar, R.string.navigation_drawer_open, R.string.navigation_drawer_close);
        drawer.addDrawerListener(toggle);
        toggle.syncState();

        navigationView = findViewById(R.id.nav_view);
        navigationView.setNavigationItemSelectedListener(this);





        sharedPreferences = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
        mainActivity = this;


        ViewGroup mainScreenContainer = findViewById(R.id.main_screen_container);
        mainScreenView = new MainScreenView(this);
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
        mainScreenView.setFillScreen(sharedPreferences.getBoolean("settings_fill_screen", false));
        settingUpdateAllowRotation();

        AssetManager assetManager = getResources().getAssets();
        NativeLib.start(assetManager, mainScreenView.getBitmapMainScreen(), this, mainScreenView);

        // By default Port1 is set
        setPort1Settings(true, true);

        Set<String> savedMRU = sharedPreferences.getStringSet("MRU", null);
        if(savedMRU != null) {
            for (String url : savedMRU) {
                mruLinkedHashMap.put(url, null);
            }
        }
        updateMRU();
        updateFromPreferences(null, false);
        sharedPreferences.registerOnSharedPreferenceChangeListener(this);

        updateNavigationDrawerItems();


        String documentToOpenUrl = sharedPreferences.getString("lastDocument", "");
        Uri documentToOpenUri = null;
        boolean isFileAndNeedPermission = false;
        Intent intent = getIntent();
        if(intent != null) {
            String action = intent.getAction();
            if(action != null) {
                if (action.equals(Intent.ACTION_VIEW)) {
                    documentToOpenUri = intent.getData();
                    if (documentToOpenUri != null) {
                        String scheme = documentToOpenUri.getScheme();
                        if(scheme != null && scheme.compareTo("file") == 0) {
                            documentToOpenUrl = documentToOpenUri.getPath();
                            isFileAndNeedPermission = true;
                        } else
                            documentToOpenUrl = documentToOpenUri.toString();
                    }
                } else if (action.equals(Intent.ACTION_SEND)) {
                    documentToOpenUri = intent.getParcelableExtra(Intent.EXTRA_STREAM);
                    if (documentToOpenUri != null) {
                        documentToOpenUrl = documentToOpenUri.toString();
                    }
                }
            }
        }

        //https://developer.android.com/guide/topics/providers/document-provider#permissions
        if(documentToOpenUrl != null && documentToOpenUrl.length() > 0)
            try {
                if(isFileAndNeedPermission
                        && ActivityCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
                    ActivityCompat.requestPermissions(this, new String[]{ Manifest.permission.WRITE_EXTERNAL_STORAGE }, PERMISSIONS_REQUEST_WRITE_EXTERNAL_STORAGE);
                    //return;
                } else {
                    if(onFileOpen(documentToOpenUrl) != 0) {
                        saveLastDocument(documentToOpenUrl);
                        if(intent != null && documentToOpenUri != null && !isFileAndNeedPermission)
                            makeUriPersistable(intent, documentToOpenUri);
                    }

                }
            } catch (Exception e) {
                Log.e(TAG, e.getMessage());
            }
        else if(drawer != null)
            drawer.openDrawer(GravityCompat.START);
    }

    private void updateMRU() {
        Menu menu = navigationView.getMenu();
        MenuItem recentsMenuItem = menu.findItem(R.id.nav_item_recents);
        SubMenu recentsSubMenu = null;
        if(recentsMenuItem != null) {
            recentsSubMenu = recentsMenuItem.getSubMenu();
            if (recentsSubMenu != null)
                recentsSubMenu.clear();
        }
        if (recentsSubMenu != null) {
            Set<String> mruLinkedHashMapKeySet = mruLinkedHashMap.keySet();
            String[] mrus = mruLinkedHashMapKeySet.toArray(new String[mruLinkedHashMapKeySet.size()]);
            for (int i = mrus.length - 1; i >= 0; i--) {
                String displayName = getFilenameFromURL(mrus[i]);
                recentsSubMenu.add(Menu.NONE, MRU_ID_START + i, Menu.NONE, displayName);
            }
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    protected void onStop() {
        //TODO We cannot make the difference between going to the settings or loading/saving a file and a real app stop/kill!
        // -> Maybe by settings some flags when loading/saving
        if(NativeLib.isDocumentAvailable() && sharedPreferences.getBoolean("settings_autosave", true)) {
            String currentFilename = NativeLib.getCurrentFilename();
            if (currentFilename != null && currentFilename.length() > 0) {
                if(NativeLib.onFileSave() == 1)
                    showAlert("State saved");
            }
        }

        SharedPreferences.Editor editor = sharedPreferences.edit();
        editor.putStringSet("MRU", mruLinkedHashMap.keySet());
        editor.apply();

        super.onStop();
    }

    @Override
    protected void onDestroy() {
        //onDestroy is never called!
        NativeLib.stop();
        sharedPreferences.unregisterOnSharedPreferenceChangeListener(this);
        super.onDestroy();
    }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
        updateFromPreferences(key, true);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String permissions[], @NonNull int[] grantResults) {
        switch (requestCode) {
            case PERMISSIONS_REQUEST_WRITE_EXTERNAL_STORAGE: {
                // If request is cancelled, the result arrays are empty.
//				if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
//
//				} else {
//					//Toast.makeText(this, R.string.toast_access_location_denied, Toast.LENGTH_SHORT).show();
//				}
                String lastDocumentUrl = sharedPreferences.getString("lastDocument", "");
                if(lastDocumentUrl.length() > 0) {
                    if(onFileOpen(lastDocumentUrl) != 0)
                        try {
                            makeUriPersistable(getIntent(), Uri.parse(lastDocumentUrl));
                        } catch (Exception e) {
                        // Do nothing
                        }
                }
//				return;
            }
//			default:
//				super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        }
    }

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
        } else if(id >= MRU_ID_START && id < MRU_ID_START + MAX_MRU) {

            Set<String> mruLinkedHashMapKeySet = mruLinkedHashMap.keySet();
            int mruLength = mruLinkedHashMapKeySet.size();
            String[] mrus = mruLinkedHashMapKeySet.toArray(new String[mruLength]);

            int mruClickedIndex = id - MRU_ID_START;
            final String url = mrus[mruClickedIndex];
            mruLinkedHashMap.get(url);

            ensureDocumentSaved(new Runnable() {
                @Override
                public void run() {
                    if(onFileOpen(url) != 0) {
                        saveLastDocument(url);
                    }
                }
            });

        }

        DrawerLayout drawer = (DrawerLayout) findViewById(R.id.drawer_layout);
        drawer.closeDrawer(GravityCompat.START);
        return true;
    }

    private void updateNavigationDrawerItems() {
        Menu menu = navigationView.getMenu();
        boolean isDocumentAvailable = NativeLib.isDocumentAvailable();
        boolean isBackup = NativeLib.isBackup();
        menu.findItem(R.id.nav_save).setEnabled(isDocumentAvailable);
        menu.findItem(R.id.nav_save_as).setEnabled(isDocumentAvailable);
        menu.findItem(R.id.nav_close).setEnabled(isDocumentAvailable);
        menu.findItem(R.id.nav_load_object).setEnabled(isDocumentAvailable);
        menu.findItem(R.id.nav_save_object).setEnabled(isDocumentAvailable);
        menu.findItem(R.id.nav_copy_screen).setEnabled(isDocumentAvailable);
        menu.findItem(R.id.nav_copy_stack).setEnabled(isDocumentAvailable);
        menu.findItem(R.id.nav_paste_stack).setEnabled(isDocumentAvailable);
        menu.findItem(R.id.nav_reset_calculator).setEnabled(isDocumentAvailable);
        menu.findItem(R.id.nav_backup_save).setEnabled(isDocumentAvailable);
        menu.findItem(R.id.nav_backup_restore).setEnabled(isDocumentAvailable && isBackup);
        menu.findItem(R.id.nav_backup_delete).setEnabled(isDocumentAvailable && isBackup);
        menu.findItem(R.id.nav_change_kml_script).setEnabled(isDocumentAvailable);
    }

    class KMLScriptItem {
        public String filename;
        public String title;
        public String model;
    }
    ArrayList<KMLScriptItem> kmlScripts;
    private void extractKMLScripts() {
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
    }

    private Runnable fileSaveAsCallback = null;
    private void ensureDocumentSaved(final Runnable continueCallback) {
        if(NativeLib.isDocumentAvailable()) {
            final String currentFilename = NativeLib.getCurrentFilename();
            final boolean hasFilename = (currentFilename != null && currentFilename.length() > 0);

            DialogInterface.OnClickListener onClickListener = new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    if (which == DialogInterface.BUTTON_POSITIVE) {
                        if (hasFilename) {
                            if(NativeLib.onFileSave() == 1)
                                showAlert("State saved");
                            if (continueCallback != null)
                                continueCallback.run();
                        } else {
                            //TODO SaveAs...
                            fileSaveAsCallback = continueCallback;
                            OnFileSaveAs();
                        }
                    } else if (which == DialogInterface.BUTTON_NEGATIVE) {
                        if(continueCallback != null)
                            continueCallback.run();
                    }
                }
            };

            if(hasFilename && sharedPreferences.getBoolean("settings_autosave", true)) {
                onClickListener.onClick(null, DialogInterface.BUTTON_POSITIVE);
            } else {
                new AlertDialog.Builder(this)
                        .setMessage("Do you want to save changes?\n(BACK to cancel)")
                        .setPositiveButton("Yes", onClickListener)
                        .setNegativeButton("No", onClickListener)
                        .show();
            }
        } else if(continueCallback != null)
            continueCallback.run();
    }

    private void OnFileNew() {
        // By default Port1 is set
        setPort1Settings(true, true);

        extractKMLScripts();

        ensureDocumentSaved(new Runnable() {
            @Override
            public void run() {
                final String[] kmlScriptTitles = new String[kmlScripts.size()];
                for (int i = 0; i < kmlScripts.size(); i++)
                    kmlScriptTitles[i] = kmlScripts.get(i).title;
                new AlertDialog.Builder(MainActivity.this)
                        .setTitle("Pick a calculator")
                        .setItems(kmlScriptTitles, new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                String kmlScriptFilename = kmlScripts.get(which).filename;
                                NativeLib.onFileNew(kmlScriptFilename);
                                displayFilename("");
                                showKMLLog();
                                updateNavigationDrawerItems();
                            }
                        }).show();
            }
        });
    }

    private void OnFileOpen() {
        ensureDocumentSaved(new Runnable() {
            @Override
            public void run() {
                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                intent.addCategory(Intent.CATEGORY_OPENABLE);
                //intent.setType("YOUR FILETYPE"); //not needed, but maybe usefull
                intent.setType("*/*");
                intent.putExtra(Intent.EXTRA_TITLE, "emu48-state.e48");
                startActivityForResult(intent, INTENT_GETOPENFILENAME);
            }
        });
    }
    private void OnFileSave() {
        ensureDocumentSaved(null);
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
        ensureDocumentSaved(new Runnable() {
            @Override
            public void run() {
                NativeLib.onFileClose();
                saveLastDocument("");
                updateNavigationDrawerItems();
                displayFilename("");
                if(drawer != null) {
                    new android.os.Handler().postDelayed(new Runnable() {
                        public void run() {
                            drawer.openDrawer(GravityCompat.START);
                        }
                    }, 300);
                }
            }
        });
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
        int width = NativeLib.getScreenWidth();
        int height = NativeLib.getScreenHeight();
        Bitmap bitmapScreen = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        bitmapScreen.eraseColor(Color.BLACK);

        NativeLib.onViewCopy(bitmapScreen);

        String imageFilename = "Emu48-" + new SimpleDateFormat("yyyy-MM-dd_HH-mm-ss", Locale.US).format(new Date());
        try {
            File storagePath = new File(this.getExternalCacheDir(), "");
            File imageFile = File.createTempFile(imageFilename, ".png", storagePath);
            FileOutputStream fileOutputStream = new FileOutputStream(imageFile);
            bitmapScreen.compress(Bitmap.CompressFormat.PNG, 90, fileOutputStream);
            fileOutputStream.close();
            String mimeType = "application/png";
            Intent intent = new Intent(android.content.Intent.ACTION_SEND);
            intent.setType(mimeType);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            intent.putExtra(Intent.EXTRA_SUBJECT, "Emu48 screenshot");
            intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            intent.putExtra(Intent.EXTRA_STREAM, FileProvider.getUriForFile(this,this.getPackageName() + ".provider", imageFile));
            startActivity(Intent.createChooser(intent, "Share Emu48 screenshot"));
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    private void OnStackCopy() {
        NativeLib.onStackCopy();
    }
    private void OnStackPaste() {
        NativeLib.onStackPaste();
    }
    private void OnViewReset() {
        DialogInterface.OnClickListener onClickListener = new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                if (which == DialogInterface.BUTTON_POSITIVE) {
                    NativeLib.onViewReset();
                }
            }
        };
        new AlertDialog.Builder(this)
                .setMessage("Are you sure you want to press the Reset Button?")
                .setPositiveButton("Yes", onClickListener)
                .setNegativeButton("No", onClickListener)
                .show();
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
        extractKMLScripts();

        if (NativeLib.getState() != 0 /*SM_RUN*/)
        {
            showAlert("You cannot change the KML script when Emu48 is not running.\n"
                    + "Use the File,New menu item to create a new calculator.");
            return;
        }
        final ArrayList<KMLScriptItem> kmlScriptsForCurrentModel = new ArrayList<>();
        char m = (char)NativeLib.getCurrentModel();
        for (int i = 0; i < kmlScripts.size(); i++) {
            KMLScriptItem kmlScriptItem = kmlScripts.get(i);
            if (kmlScriptItem.model.charAt(0) == m)
                kmlScriptsForCurrentModel.add(kmlScriptItem);
        }

        final String[] kmlScriptTitles = new String[kmlScriptsForCurrentModel.size()];
        for (int i = 0; i < kmlScriptsForCurrentModel.size(); i++)
            kmlScriptTitles[i] = kmlScriptsForCurrentModel.get(i).title;
        new AlertDialog.Builder(MainActivity.this)
                .setTitle("Pick a calculator")
                .setItems(kmlScriptTitles, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        String kmlScriptFilename = kmlScriptsForCurrentModel.get(which).filename;
                        NativeLib.onViewScript(kmlScriptFilename);
                        displayKMLTitle();
                        showKMLLog();
                        updateNavigationDrawerItems();
                    }
                }).show();
    }
    private void OnTopics() {
        startActivity(new Intent(this, InfoWebActivity.class));
    }
    private void OnAbout() {
        startActivity(new Intent(this, InfoActivity.class));
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
                    saveLastDocument(url);
                    makeUriPersistable(data, uri);
                }
            } else if(requestCode == INTENT_GETSAVEFILENAME) {
                Uri uri = data.getData();

                //just as an example, I am writing a String to the Uri I received from the user:
                Log.d(TAG, "onActivityResult INTENT_GETSAVEFILENAME " + uri.toString());

                String url = uri.toString();
                if(NativeLib.onFileSaveAs(url) != 0) {
                    showAlert("State saved");
                    saveLastDocument(url);
                    makeUriPersistable(data, uri);
                    displayFilename(url);
                    if(fileSaveAsCallback != null)
                        fileSaveAsCallback.run();
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
        fileSaveAsCallback = null;
        super.onActivityResult(requestCode, resultCode, data);
    }

    private void saveLastDocument(String url) {
        SharedPreferences.Editor editor = sharedPreferences.edit();
        editor.putString("lastDocument", url);
        editor.apply();

        mruLinkedHashMap.put(url, null);
        updateMRU();
    }

    private void makeUriPersistable(Intent data, Uri uri) {
        //grantUriPermission(getPackageName(), uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
        final int takeFlags = data.getFlags() & (Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        getContentResolver().takePersistableUriPermission(uri, takeFlags);
    }

    private int onFileOpen(String url) {
        int result = NativeLib.onFileOpen(url);
        setPort1Settings(NativeLib.getPort1Plugged(), NativeLib.getPort1Writable());
        displayFilename(url);
        showKMLLog();
        updateNavigationDrawerItems();
        return result;
    }

    private void displayFilename(String url) {
        String displayName = getFilenameFromURL(url);
        View header = displayKMLTitle();
        TextView textViewSubtitle = header.findViewById(R.id.nav_header_subtitle);
        if(textViewSubtitle != null)
            textViewSubtitle.setText(displayName);
    }

    private String getFilenameFromURL(String url) {
        String displayName = "";
        try {
            displayName = SettingsActivity.getFileName(this, url);
        } catch(Exception e) {
            // Do nothing
        }
        return displayName;
    }

    private View displayKMLTitle() {
        NavigationView navigationView = findViewById(R.id.nav_view);
        View header = navigationView.getHeaderView(0);
        TextView textViewTitle = header.findViewById(R.id.nav_header_title);
        if(textViewTitle != null)
            textViewTitle.setText(NativeLib.getKMLTitle());
        return header;
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
    Map<Integer, ParcelFileDescriptor> parcelFileDescriptorPerFd = null;
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
        if(parcelFileDescriptorPerFd == null) {
            parcelFileDescriptorPerFd = new HashMap<>();
        }
        parcelFileDescriptorPerFd.put(fd, filePfd);
        return fd;
    }
    int closeFileFromContentResolver(int fd) {
        if(parcelFileDescriptorPerFd != null) {
            ParcelFileDescriptor filePfd = parcelFileDescriptorPerFd.get(fd);
            if(filePfd != null) {
                try {
                    filePfd.close();
                    parcelFileDescriptorPerFd.remove(fd);
                    return 0;
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        return -1;
    }

    void showAlert(String text) {
        Snackbar.make(findViewById(R.id.main_coordinator), text, Snackbar.LENGTH_SHORT).setAction("Action", null).show();
    }

    void sendMenuItemCommand(int menuItem) {
        switch (menuItem) {
            case 1: // FILE_NEW
                OnFileNew();
                break;
            case 2: // FILE_OPEN
                OnFileOpen();
                break;
            case 3: // FILE_SAVE
                OnFileSave();
                break;
            case 4: // FILE_SAVEAS
                OnFileSaveAs();
                break;
            case 5: // FILE_EXIT
                break;
            case 6: // EDIT_COPY_SCREEN
                OnViewCopy();
                break;
            case 7: // FILE_SETTINGS
                OnSettings();
                break;
            case 8: // EDIT_RESET
                OnViewReset();
                break;
            case 9: // EDIT_LOAD_OBJECT
                OnObjectLoad();
                break;
            case 10: // EDIT_SAVE_OBJECT
                OnObjectSave();
                break;
            case 11: // HELP_ABOUT
                OnAbout();
                break;
            case 12: // HELP_TOPICS
                OnTopics();
                break;
            case 13: // FILE_CLOSE
                OnFileClose();
                break;
            case 14: // EDIT_BACKUP_SAVE
                OnBackupSave();
                break;
            case 15: // EDIT_BACKUP_RESTORE
                OnBackupRestore();
                break;
            case 16: // EDIT_BACKUP_DELETE
                OnBackupDelete();
                break;
            case 17: // VIEW_SCRIPT
                OnViewScript();
                break;
            case 18: // EDIT_PORT_CONFIGURATION
                break;
            case 19: // EDIT_COPY_STRING
                OnStackCopy();
                break;
            case 20: // EDIT_PASTE_STRING
                OnStackPaste();
                break;
            case 21: // TOOL_DISASM
                break;
            case 22: // TOOL_DEBUG
                break;
            case 23: // TOOL_MACRO_RECORD
                break;
            case 24: // TOOL_MACRO_PLAY
                break;
            case 25: // TOOL_MACRO_STOP
                break;
            case 26: // TOOL_MACRO_SETTINGS
                break;
            default:
                break;
        }
    }

    String getFirstKMLFilenameForType(char chipsetType) {
        extractKMLScripts();

        for (int i = 0; i < kmlScripts.size(); i++) {
            KMLScriptItem kmlScriptItem = kmlScripts.get(i);
            if (kmlScriptItem.model.charAt(0) == chipsetType) {
                return kmlScriptItem.filename;
            }
        }
        return null;
    }

    void clipboardCopyText(String text) {
        // Gets a handle to the clipboard service.
        ClipboardManager clipboard = (ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
        if(clipboard != null) {
            ClipData clip = ClipData.newPlainText("simple text", text);
            // Set the clipboard's primary clip.
            clipboard.setPrimaryClip(clip);
        }
    }
    String clipboardPasteText() {
        ClipboardManager clipboard = (ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
        if (clipboard != null && clipboard.hasPrimaryClip()) {
            ClipData clipData = clipboard.getPrimaryClip();
            if(clipData != null) {
                ClipData.Item item = clipData.getItemAt(0);
                // Gets the clipboard as text.
                CharSequence pasteData = item.getText();
                if (pasteData != null)
                    return pasteData.toString();
            }
        }
        return "";
    }

    private void setPort1Settings(boolean port1Plugged, boolean port1Writable) {
        SharedPreferences.Editor editor = sharedPreferences.edit();
        editor.putBoolean("settings_port1en", port1Plugged);
        editor.putBoolean("settings_port1wr", port1Writable);
        editor.apply();
    }

    private void updateFromPreferences(String key, boolean isDynamic) {
        int isDynamicValue = isDynamic ? 1 : 0;
        if(key == null) {
            String[] settingKeys = { "settings_realspeed", "settings_grayscale", "settings_port1", "settings_port2" };
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

                case "settings_allow_rotation":
                    //setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LOCKED);
                    settingUpdateAllowRotation();
                    break;
                case "settings_fill_screen":
                    mainScreenView.setFillScreen(sharedPreferences.getBoolean("settings_fill_screen", false));
                    break;
            }
        }
    }

    private void settingUpdateAllowRotation() {
        if(sharedPreferences.getBoolean("settings_allow_rotation", false))
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
        else
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
    }
}
