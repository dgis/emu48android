package org.emulator.forty.eight;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.HapticFeedbackConstants;
import android.view.Menu;
import android.view.MenuItem;
import android.view.SubMenu;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ImageButton;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.ActionBarDrawerToggle;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.content.FileProvider;
import androidx.core.view.GravityCompat;
import androidx.documentfile.provider.DocumentFile;
import androidx.drawerlayout.widget.DrawerLayout;
import com.google.android.material.navigation.NavigationView;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class MainActivity extends AppCompatActivity implements NavigationView.OnNavigationItemSelectedListener {

    private static final String TAG = "MainActivity";
    public static MainActivity mainActivity;
    private SharedPreferences sharedPreferences;
    private NavigationView navigationView;
    private DrawerLayout drawer;
    private MainScreenView mainScreenView;

    public static final int INTENT_GETOPENFILENAME = 1;
    public static final int INTENT_GETSAVEFILENAME = 2;
    public static final int INTENT_OBJECT_LOAD = 3;
    public static final int INTENT_OBJECT_SAVE = 4;
    public static final int INTENT_SETTINGS = 5;
    public static final int INTENT_PORT2LOAD = 6;
    public static final int INTENT_PICK_KML_FOLDER_FOR_NEW_FILE = 7;
    public static final int INTENT_PICK_KML_FOLDER_FOR_CHANGING = 8;
    public static final int INTENT_PICK_KML_FOLDER_FOR_SETTINGS = 9;
    public static final int INTENT_CREATE_RAM_CARD = 10;

    private String kmlMimeType = "application/vnd.google-earth.kml+xml";
    private boolean kmlFolderUseDefault = true;
    private String kmlFolderURL = "";
    private boolean kmFolderChange = true;

    private int selectedRAMSize = -1;

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
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            mainScreenView.setStatusBarColor(getWindow().getStatusBarColor());
        }

        toolbar.setVisibility(View.GONE);
        mainScreenContainer.addView(mainScreenView, 0);

        AssetManager assetManager = getResources().getAssets();
        NativeLib.start(assetManager, mainScreenView.getBitmapMainScreen(), this, mainScreenView);

        // By default Port1 is set
        setPort1Settings(true, true);

        Set<String> savedMRU = sharedPreferences.getStringSet("MRU", null);
        if(savedMRU != null) {
            for (String url : savedMRU) {
                if(url != null & !url.isEmpty())
                    mruLinkedHashMap.put(url, null);
            }
        }
        updateMRU();
        updateFromPreferences(null, false);

        updateNavigationDrawerItems();




        //android.os.Debug.waitForDebugger();


        String documentToOpenUrl = sharedPreferences.getString("lastDocument", "");
        Uri documentToOpenUri = null;
        Intent intent = getIntent();
        if(intent != null) {
            String action = intent.getAction();
            if(action != null) {
                if (action.equals(Intent.ACTION_VIEW)) {
                    documentToOpenUri = intent.getData();
                    if (documentToOpenUri != null) {
                        String scheme = documentToOpenUri.getScheme();
                        if(scheme != null && scheme.compareTo("file") == 0) {
                            documentToOpenUrl = null;
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

        if(documentToOpenUrl != null && documentToOpenUrl.length() > 0)
            try {
                if(onFileOpen(documentToOpenUrl) != 0) {
                    saveLastDocument(documentToOpenUrl);
                    if(intent != null && documentToOpenUri != null)
                        makeUriPersistable(intent, documentToOpenUri);
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
            String[] mrus = mruLinkedHashMapKeySet.toArray(new String[0]);
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
                    showAlert(getString(R.string.message_state_saved));
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
        super.onDestroy();
    }

    @Override
    public void onBackPressed() {
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
        } else if (id == R.id.nav_create_ram_card) {
            OnCreateRAMCard();
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
        if(kmlScripts == null || kmFolderChange) {
            kmFolderChange = false;

            kmlScripts = new ArrayList<>();

            if(kmlFolderUseDefault) {
                AssetManager assetManager = getAssets();
                String[] calculatorsAssetFilenames = new String[0];
                try {
                    calculatorsAssetFilenames = assetManager.list("calculators");
                } catch (IOException e) {
                    e.printStackTrace();
                }
                kmlScripts.clear();
                if(calculatorsAssetFilenames != null) {
                    Pattern patternGlobalTitle = Pattern.compile("\\s*Title\\s+\"(.*)\"");
                    Pattern patternGlobalModel = Pattern.compile("\\s*Model\\s+\"(.*)\"");
                    Matcher m;
                    for (String calculatorFilename : calculatorsAssetFilenames) {
                        if (calculatorFilename.toLowerCase().lastIndexOf(".kml") != -1) {
                            BufferedReader reader = null;
                            try {
                                reader = new BufferedReader(new InputStreamReader(assetManager.open("calculators/" + calculatorFilename), "UTF-8"));
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
                                            newKMLScriptItem.filename = calculatorFilename;
                                            newKMLScriptItem.title = title;
                                            newKMLScriptItem.model = model;
                                            kmlScripts.add(newKMLScriptItem);
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
                }
            } else {
                Uri kmlFolderUri = Uri.parse(kmlFolderURL);
                List<String> calculatorsAssetFilenames = new LinkedList<>();

                DocumentFile kmlFolderDocumentFile = DocumentFile.fromTreeUri(this, kmlFolderUri);
                for (DocumentFile file : kmlFolderDocumentFile.listFiles()) {
                    final String url = file.getUri().toString();
                    final String name = file.getName();
                    final String mime = file.getType();
                    Log.d(TAG, "url: " + url + ", name: " + name + ", mime: " + mime);
                    if(kmlMimeType.equals(mime)) {
                        calculatorsAssetFilenames.add(url);
                    }
                }
                kmlScripts.clear();
                Pattern patternGlobalTitle = Pattern.compile("\\s*Title\\s+\"(.*)\"");
                Pattern patternGlobalModel = Pattern.compile("\\s*Model\\s+\"(.*)\"");
                Matcher m;
                for (String calculatorFilename : calculatorsAssetFilenames) {
                    if (calculatorFilename.toLowerCase().lastIndexOf(".kml") != -1) {
                        BufferedReader reader = null;
                        try {
                            Uri calculatorsAssetFilenameUri = Uri.parse(calculatorFilename);
                            DocumentFile documentFile = DocumentFile.fromSingleUri(this, calculatorsAssetFilenameUri);
                            Uri fileUri = documentFile.getUri();
                            InputStream inputStream = getContentResolver().openInputStream(fileUri);
                            reader = new BufferedReader(new InputStreamReader(inputStream, "UTF-8"));
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
                                        newKMLScriptItem.filename = kmlFolderUseDefault ? calculatorFilename : "document:" + kmlFolderURL + "|" + calculatorFilename;
                                        newKMLScriptItem.title = title;
                                        newKMLScriptItem.model = model;
                                        kmlScripts.add(newKMLScriptItem);
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

            }

            Collections.sort(kmlScripts, new Comparator<KMLScriptItem>() {
                @Override
                public int compare(KMLScriptItem lhs, KMLScriptItem rhs) {
                    return lhs.title.compareTo(rhs.title);
                }
            });
        }
    }

    private void ensureDocumentSaved(final Runnable continueCallback) {
        ensureDocumentSaved(continueCallback, false);
    }

    private Runnable fileSaveAsCallback = null;
    private void ensureDocumentSaved(final Runnable continueCallback, boolean forceRequest) {
        if(NativeLib.isDocumentAvailable()) {
            final String currentFilename = NativeLib.getCurrentFilename();
            final boolean hasFilename = (currentFilename != null && currentFilename.length() > 0);

            DialogInterface.OnClickListener onClickListener = new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    if (which == DialogInterface.BUTTON_POSITIVE) {
                        if (hasFilename) {
                            if(NativeLib.onFileSave() == 1)
                                showAlert(getString(R.string.message_state_saved));
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

            if(!forceRequest && hasFilename && sharedPreferences.getBoolean("settings_autosave", true)) {
                onClickListener.onClick(null, DialogInterface.BUTTON_POSITIVE);
            } else {
                new AlertDialog.Builder(this)
                        .setMessage(getString(R.string.message_do_you_want_to_save))
                        .setPositiveButton(getString(R.string.message_yes), onClickListener)
                        .setNegativeButton(getString(R.string.message_no), onClickListener)
                        .show();
            }
        } else if(continueCallback != null)
            continueCallback.run();
    }

    private void OnFileNew() {
        // By default Port1 is set
        setPort1Settings(true, true);

        ensureDocumentSaved(new Runnable() {
            @Override
            public void run() {
                showKMLPicker(false);
            }
        });
    }

    private void newFileFromKML(String kmlScriptFilename) {
        int result = NativeLib.onFileNew(kmlScriptFilename);
        if(result > 0) {
            displayFilename("");
            showKMLLog();
        } else
            showKMLLogForce();
        updateNavigationDrawerItems();
    }

    private void OnFileOpen() {
        ensureDocumentSaved(new Runnable() {
            @Override
            public void run() {
                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                intent.addCategory(Intent.CATEGORY_OPENABLE);
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
        String extension = "e48";
        switch (model) {
            case 'S': //HP48SX
            case 'G': //HP48GX
                extension = "e48";
                break;
            case '6': //HP38G 64K RAM
            case 'A': //HP38G 32K RAM
                extension = "e38";
                break;
            case 'E': // HP39G/(HP39G+/HP39GS)/HP40G/HP40GS
                extension = "e39";
                break;
            case 'P': // HP39G+/HP39GS
                extension = "e39";
                break;
            case '2': // HP48GII
            case 'Q': // HP49G+/HP50G
            case 'X': // HP49G
                extension = "e49";
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
        }, true);
    }
    private void OnSettings() {
        startActivityForResult(new Intent(this, SettingsActivity.class), INTENT_SETTINGS);
    }

    private void openDocument() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
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
                    .setMessage(getString(R.string.message_object_load))
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
            intent.putExtra(Intent.EXTRA_SUBJECT, R.string.message_screenshot);
            intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            intent.putExtra(Intent.EXTRA_STREAM, FileProvider.getUriForFile(this,this.getPackageName() + ".provider", imageFile));
            startActivity(Intent.createChooser(intent, getString(R.string.message_share_screenshot)));
        } catch (Exception e) {
            e.printStackTrace();
            showAlert(e.getMessage());
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
                .setMessage(getString(R.string.message_press_reset))
                .setPositiveButton(getString(R.string.message_yes), onClickListener)
                .setNegativeButton(getString(R.string.message_no), onClickListener)
                .show();
    }
    private void OnBackupSave() {
        NativeLib.onBackupSave();
        updateNavigationDrawerItems();
    }
    private void OnBackupRestore() {
        NativeLib.onBackupRestore();
        updateNavigationDrawerItems();
    }
    private void OnBackupDelete() {
        NativeLib.onBackupDelete();
        updateNavigationDrawerItems();
    }
    private void OnViewScript() {
        if (NativeLib.getState() != 0 /*SM_RUN*/) {
            showAlert(getString(R.string.message_change_kml));
            return;
        }

        showKMLPicker(true);
    }

    private void showKMLPicker(final boolean changeKML) {
        extractKMLScripts();

        final ArrayList<KMLScriptItem> kmlScriptsForCurrentModel;
        if(changeKML) {
            kmlScriptsForCurrentModel = new ArrayList<KMLScriptItem>();
            char m = (char) NativeLib.getCurrentModel();
            for (int i = 0; i < kmlScripts.size(); i++) {
                KMLScriptItem kmlScriptItem = kmlScripts.get(i);
                if (kmlScriptItem.model.charAt(0) == m)
                    kmlScriptsForCurrentModel.add(kmlScriptItem);
            }
        } else
            kmlScriptsForCurrentModel = kmlScripts;

        final int lastIndex = kmlScriptsForCurrentModel.size();
        final String[] kmlScriptTitles = new String[lastIndex + (lastIndex > 0 ? 2 : 1)];
        for (int i = 0; i < kmlScriptsForCurrentModel.size(); i++)
            kmlScriptTitles[i] = kmlScriptsForCurrentModel.get(i).title;
        kmlScriptTitles[lastIndex] = getResources().getString(R.string.load_custom_kml);
        if(lastIndex > 0)
            kmlScriptTitles[lastIndex + 1] = getResources().getString(R.string.load_default_kml);
        new AlertDialog.Builder(MainActivity.this)
                .setTitle(getResources().getString(R.string.pick_calculator))
                .setItems(kmlScriptTitles, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        if(which == lastIndex) {
                            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
                            startActivityForResult(intent, changeKML ? INTENT_PICK_KML_FOLDER_FOR_CHANGING : INTENT_PICK_KML_FOLDER_FOR_NEW_FILE);
                        } else if(which == lastIndex + 1) {
                            // Reset to default KML folder
                            SharedPreferences.Editor editor = sharedPreferences.edit();
                            editor.putBoolean("settings_kml_default", true);
                            editor.apply();
                            updateFromPreferences("settings_kml", true);
                            if(changeKML)
                                OnViewScript();
                            else
                                OnFileNew();
                        } else {
                            String kmlScriptFilename = kmlScriptsForCurrentModel.get(which).filename;
                            if(changeKML) {
                                int result = NativeLib.onViewScript(kmlScriptFilename);
                                if(result > 0) {
                                    displayKMLTitle();
                                    showKMLLog();
                                } else
                                    showKMLLogForce();
                                updateNavigationDrawerItems();
                            } else
                                newFileFromKML(kmlScriptFilename);
                        }
                    }
                }).show();
    }

    private void OnCreateRAMCard() {
        String[] stringArrayRAMCards = getResources().getStringArray(R.array.ram_cards);
        new AlertDialog.Builder(MainActivity.this)
                .setTitle(getResources().getString(R.string.create_ram_card_title))
                .setItems(stringArrayRAMCards, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
                        intent.addCategory(Intent.CATEGORY_OPENABLE);
                        intent.setType("*/*");
                        String sizeTitle = "2mb";
                        selectedRAMSize = -1;
                        switch (which) {
                            case 0: // 32kb (1 port: 2)
                                sizeTitle = "32kb";
                                selectedRAMSize = 32;
                                break;
                            case 1: // 128kb (1 port: 2)
                                sizeTitle = "128kb";
                                selectedRAMSize = 128;
                                break;
                            case 2: // 256kb (2 ports: 2,3)
                                sizeTitle = "256kb";
                                selectedRAMSize = 256;
                                break;
                            case 3: // 512kb (4 ports: 2 through 5)
                                sizeTitle = "512kb";
                                selectedRAMSize = 512;
                                break;
                            case 4: // 1mb (8 ports: 2 through 9)
                                sizeTitle = "1mb";
                                selectedRAMSize = 1024;
                                break;
                            case 5: // 2mb (16 ports: 2 through 17)
                                sizeTitle = "2mb";
                                selectedRAMSize = 2048;
                                break;
                            case 6: // 4mb (32 ports: 2 through 33)
                                sizeTitle = "4mb";
                                selectedRAMSize = 4096;
                                break;
                        }
                        intent.putExtra(Intent.EXTRA_TITLE, "shared-" + sizeTitle + ".bin");
                        startActivityForResult(intent, INTENT_CREATE_RAM_CARD);
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
        if(resultCode == Activity.RESULT_OK && data != null) {
            if(requestCode == INTENT_SETTINGS) {
                String[] changedKeys = data.getStringArrayExtra("changedKeys");
                if(changedKeys != null) {
                    HashSet<String> changedKeysCleaned = new HashSet<>();
                    for (String key : changedKeys) {
                        //Log.d(TAG, "ChangedKey): " + key);
                        switch (key) {
                            case "settings_port1en":
                            case "settings_port1wr":
                                changedKeysCleaned.add("settings_port1");
                                break;
                            case "settings_port2en":
                            case "settings_port2wr":
                            case "settings_port2load":
                                changedKeysCleaned.add("settings_port2");
                                break;
                            default:
                                changedKeysCleaned.add(key);
                                break;
                        }
                    }
                    for (String key : changedKeysCleaned) {
                        updateFromPreferences(key, true);
                    }
                }
            } else {
                Uri uri = data.getData();
                String url = null;
                if (uri != null)
                    url = uri.toString();
                if (url != null) {
                    switch (requestCode) {
                        case INTENT_GETOPENFILENAME: {
                            //Log.d(TAG, "onActivityResult INTENT_GETOPENFILENAME " + url);
                            if (onFileOpen(url) != 0) {
                                saveLastDocument(url);
                                makeUriPersistable(data, uri);
                            }
                            break;
                        }
                        case INTENT_GETSAVEFILENAME: {
                            //Log.d(TAG, "onActivityResult INTENT_GETSAVEFILENAME " + url);
                            if (NativeLib.onFileSaveAs(url) != 0) {
                                showAlert(getString(R.string.message_state_saved));
                                saveLastDocument(url);
                                makeUriPersistable(data, uri);
                                displayFilename(url);
                                if (fileSaveAsCallback != null)
                                    fileSaveAsCallback.run();
                            }
                            break;
                        }
                        case INTENT_OBJECT_LOAD: {
                            //Log.d(TAG, "onActivityResult INTENT_OBJECT_LOAD " + url);
                            NativeLib.onObjectLoad(url);
                            break;
                        }
                        case INTENT_OBJECT_SAVE: {
                            //Log.d(TAG, "onActivityResult INTENT_OBJECT_SAVE " + url);
                            NativeLib.onObjectSave(url);
                            break;
                        }
                        case INTENT_PICK_KML_FOLDER_FOR_NEW_FILE:
                        case INTENT_PICK_KML_FOLDER_FOR_CHANGING:
                        case INTENT_PICK_KML_FOLDER_FOR_SETTINGS: {
                            //Log.d(TAG, "onActivityResult INTENT_PICK_KML_FOLDER " + url);
                            SharedPreferences.Editor editor = sharedPreferences.edit();
                            editor.putBoolean("settings_kml_default", false);
                            editor.putString("settings_kml_folder", url);
                            editor.apply();
                            updateFromPreferences("settings_kml", true);
                            makeUriPersistableReadOnly(data, uri);

                            switch (requestCode) {
                                case INTENT_PICK_KML_FOLDER_FOR_NEW_FILE:
                                    OnFileNew();
                                    break;
                                case INTENT_PICK_KML_FOLDER_FOR_CHANGING:
                                    OnViewScript();
                                    break;
                                case INTENT_PICK_KML_FOLDER_FOR_SETTINGS:

                                    break;
                            }
                            break;
                        }
                        case INTENT_CREATE_RAM_CARD: {
                            //Log.d(TAG, "onActivityResult INTENT_CREATE_RAM_CARD " + url);
                            if(selectedRAMSize > 0) {
                                int size = 2 * selectedRAMSize;
                                FileOutputStream fileOutputStream = null;
                                try {
                                    ParcelFileDescriptor pfd = getContentResolver().openFileDescriptor(uri, "w");
                                    fileOutputStream = new FileOutputStream(pfd.getFileDescriptor());
                                    byte[] zero = new byte[1024];
                                    Arrays.fill(zero, (byte) 0);
                                    for (int i = 0; i < size; i++)
                                        fileOutputStream.write(zero);
                                    fileOutputStream.flush();
                                    fileOutputStream.close();
                                } catch (FileNotFoundException e) {
                                    e.printStackTrace();
                                } catch (IOException e) {
                                    e.printStackTrace();
                                }
                                selectedRAMSize = -1;
                            }

                            break;
                        }
                        default:
                            break;
                    }
                }
            }
        }
        fileSaveAsCallback = null;
        super.onActivityResult(requestCode, resultCode, data);
    }

    private void saveLastDocument(String url) {
        SharedPreferences.Editor editor = sharedPreferences.edit();
        editor.putString("lastDocument", url);
        editor.apply();

        if(url != null && !url.isEmpty())
            mruLinkedHashMap.put(url, null);
        navigationView.post(new Runnable() {
            public void run() {
                updateMRU();
            }
        });
    }

    private void makeUriPersistable(Intent data, Uri uri) {
        final int takeFlags = data.getFlags() & (Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT)
            getContentResolver().takePersistableUriPermission(uri, takeFlags);
    }
    private void makeUriPersistableReadOnly(Intent data, Uri uri) {
        final int takeFlags = data.getFlags() & (Intent.FLAG_GRANT_READ_URI_PERMISSION);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT)
            getContentResolver().takePersistableUriPermission(uri, takeFlags);
    }


    private int onFileOpen(String url) {
        int result = NativeLib.onFileOpen(url);
        if(result > 0) {
            setPort1Settings(NativeLib.getPort1Plugged(), NativeLib.getPort1Writable());
            displayFilename(url);
            showKMLLog();
        } else
            showKMLLogForce();
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
            displayName = Utils.getFileName(this, url);
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
            showKMLLogForce();
        }
    }

    private void showKMLLogForce() {
        String kmlLog = NativeLib.getKMLLog();
        new AlertDialog.Builder(this)
                .setTitle(getString(R.string.message_kml_script_compilation_result))
                .setMessage(kmlLog)
                .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                    }
                }).show();
    }

    final int GENERIC_READ   = 1;
    final int GENERIC_WRITE  = 2;
    Map<Integer, ParcelFileDescriptor> parcelFileDescriptorPerFd = null;
    int openFileFromContentResolver(String fileURL, int writeAccess) {
        //https://stackoverflow.com/a/31677287
        Uri uri = Uri.parse(fileURL);
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
    int openFileInFolderFromContentResolver(String filename, String folderURL, int writeAccess) {
        Uri folderURI = Uri.parse(folderURL);
        DocumentFile folderDocumentFile = DocumentFile.fromTreeUri(this, folderURI);
        for (DocumentFile file : folderDocumentFile.listFiles()) {
            final String url = file.getUri().toString();
            final String name = file.getName();
            //Log.d(TAG, "url: " + url + ", name: " + name);
            if(filename.equals(name)) {
                return openFileFromContentResolver(url, writeAccess);
            }
        }
        return -1;
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
        Toast toast = Toast.makeText(this, text, Toast.LENGTH_SHORT);
        //View view = toast.getView();
        //view.setBackgroundColor(0x80000000);
        toast.show();
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

        // Not found, so we search in the default KML asset folder
        kmlFolderUseDefault = true;
        kmFolderChange = true;

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

    void performHapticFeedback() {
        if(sharedPreferences.getBoolean("settings_haptic_feedback", true))
            mainScreenView.performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY, HapticFeedbackConstants.FLAG_IGNORE_GLOBAL_SETTING);
    }

    private void setPort1Settings(boolean port1Plugged, boolean port1Writable) {
        SharedPreferences.Editor editor = sharedPreferences.edit();
        editor.putBoolean("settings_port1en", port1Plugged);
        editor.putBoolean("settings_port1wr", port1Writable);
        editor.apply();
        updateFromPreferences("settings_port1", true);
    }

    private void updateFromPreferences(String key, boolean isDynamic) {
        int isDynamicValue = isDynamic ? 1 : 0;
        if(key == null) {
            String[] settingKeys = {
                    "settings_realspeed", "settings_grayscale", "settings_allow_rotation", "settings_fill_screen",
                    "settings_hide_bar", "settings_scale", "settings_allow_sound", "settings_haptic_feedback",
                    "settings_background_kml_color", "settings_background_fallback_color",
                    "settings_kml", "settings_port1", "settings_port2" };
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

                case "settings_allow_rotation":
                    if(sharedPreferences.getBoolean("settings_allow_rotation", false))
                        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
                    else
                        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
                    break;
                case "settings_fill_screen":
                    mainScreenView.setFillScreen(sharedPreferences.getBoolean("settings_fill_screen", false));
                    break;
                case "settings_hide_bar":
                case "settings_hide_bar_status":
                case "settings_hide_bar_nav":
                    if(sharedPreferences.getBoolean("settings_hide_bar_status", false)
                    || sharedPreferences.getBoolean("settings_hide_bar_nav", false))
                        hideSystemUI();
                    else
                        showSystemUI();
                    break;
                case "settings_scale":
                    //mainScreenView.setScale(1.0f); //sharedPreferences.getFloat("settings_scale", 0.0f));
                    break;

                case "settings_allow_sound":
                    NativeLib.setConfiguration("settings_sound_volume", isDynamicValue,
                            sharedPreferences.getBoolean("settings_allow_sound", true) ? 64 : 0, 0, null);
                    break;

                case "settings_haptic_feedback":
                    // Nothing to do
                    break;

                case "settings_background_kml_color":
                    mainScreenView.setBackgroundKmlColor(sharedPreferences.getBoolean("settings_background_kml_color", false));
                    break;
                case "settings_background_fallback_color":
                    String fallbackColor = sharedPreferences.getString("settings_background_fallback_color", "0");
                    try {
                        mainScreenView.setBackgroundFallbackColor(Integer.parseInt(fallbackColor));
                    } catch (NumberFormatException ex) {}
                    break;

                case "settings_kml":
                case "settings_kml_default":
                case "settings_kml_folder":
                    kmlFolderUseDefault = sharedPreferences.getBoolean("settings_kml_default", true);
                    if(!kmlFolderUseDefault) {
                        kmlFolderURL = sharedPreferences.getString("settings_kml_folder", "");
                        // https://github.com/googlesamples/android-DirectorySelection
                        // https://stackoverflow.com/questions/44185477/intent-action-open-document-tree-doesnt-seem-to-return-a-real-path-to-drive/44185706
                        // https://stackoverflow.com/questions/26744842/how-to-use-the-new-sd-card-access-api-presented-for-android-5-0-lollipop
                    }
                    kmFolderChange = true;
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

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus && (
            sharedPreferences.getBoolean("settings_hide_bar_status", false)
            || sharedPreferences.getBoolean("settings_hide_bar_nav", false)
        )) {
            hideSystemUI();
        }
    }

    private void hideSystemUI() {
        int flags = View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
        if(sharedPreferences.getBoolean("settings_hide_bar_status", false))
            flags |= View.SYSTEM_UI_FLAG_FULLSCREEN;
        if(sharedPreferences.getBoolean("settings_hide_bar_nav", false))
            flags |= View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;

        getWindow().getDecorView().setSystemUiVisibility(flags);
    }

    private void showSystemUI() {
        getWindow().getDecorView().setSystemUiVisibility(0);
    }
}
