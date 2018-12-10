package com.regis.cosnier.emu48;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;
import android.view.ViewGroup;
import com.google.android.material.floatingactionbutton.FloatingActionButton;
import com.google.android.material.navigation.NavigationView;
import com.google.android.material.snackbar.Snackbar;

import androidx.annotation.Nullable;
import androidx.appcompat.app.ActionBarDrawerToggle;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.view.GravityCompat;
import androidx.drawerlayout.widget.DrawerLayout;

import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.Toast;

import java.io.IOException;
import java.io.OutputStream;

public class MainActivity extends AppCompatActivity
        implements NavigationView.OnNavigationItemSelectedListener {

    private static final int INTENT_SETTINGS = 1;
    private static final String TAG = "MainActivity";
    private MainScreenView mainScreenView;

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



        ViewGroup mainScreenContainer = (ViewGroup)findViewById(R.id.main_screen_container);
        mainScreenView = new MainScreenView(this); //, currentProject);
        mainScreenView.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View view, MotionEvent motionEvent) {
                if (motionEvent.getAction() == MotionEvent.ACTION_DOWN){
                    if(motionEvent.getY() < 0.3f * mainScreenView.getHeight()) {
                        if(toolbar.getVisibility() == View.GONE)
                            toolbar.setVisibility(View.VISIBLE);
                        else
                            toolbar.setVisibility(View.GONE);
                        return true;
                    }
                }
                return false;
            }
        });
        mainScreenContainer.addView(mainScreenView, 0);
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
            startActivityForResult(new Intent(this, SettingsActivity.class), INTENT_SETTINGS);
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

    private void OnFileNew() {
        NativeLib.onFileNew();

    }

    private void OnFileOpen() {
        NativeLib.onFileOpen();

    }
    private void OnFileSave() {
        NativeLib.onFileSave();
    }
    private void OnFileSaveAs() {
        NativeLib.onFileSaveAs();

    }
    private void OnFileClose() {
        NativeLib.onFileClose();

    }
    private void OnObjectLoad() {
        NativeLib.onObjectLoad();

    }
    private void OnObjectSave() {
        NativeLib.onObjectSave();

    }
    private void OnViewCopy() {
        NativeLib.onViewCopy();

    }
    private void OnStackCopy() {
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
        if(requestCode == MainScreenView.INTENT_GETSAVEFILENAME && resultCode == Activity.RESULT_OK) {
            Uri uri = data.getData();

            //just as an example, I am writing a String to the Uri I received from the user:
            Log.d(TAG, "onActivityResult INTENT_GETSAVEFILENAME " + uri.toString());

//            try {
//                OutputStream output = getContentResolver().openOutputStream(uri);
//
//                output.write(SOME_CONTENT.getBytes());
//                output.close();
//            }
//            catch(IOException e) {
//                Toast.makeText(this, "Error", Toast.LENGTH_SHORT).show();
//            }
        }
        super.onActivityResult(requestCode, resultCode, data);
    }
}
