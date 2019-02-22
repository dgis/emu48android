package org.emulator.forty.eight;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceFragment;
import android.preference.PreferenceManager;
import android.util.ArraySet;
import android.view.MenuItem;

import com.google.android.material.navigation.NavigationView;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

import androidx.annotation.Nullable;
import androidx.appcompat.app.ActionBar;
import androidx.core.app.NavUtils;

/**
 * A {@link PreferenceActivity} that presents a set of application settings. On
 * handset devices, settings are presented as a single list. On tablets,
 * settings are split by category, with category headers shown to the left of
 * the list of settings.
 * <p>
 * See <a href="http://developer.android.com/design/patterns/settings.html">
 * Android Design: Settings</a> for design guidelines and the <a
 * href="http://developer.android.com/guide/topics/ui/settings.html">Settings
 * API Guide</a> for more information on developing a Settings UI.
 */
public class SettingsActivity extends AppCompatPreferenceActivity implements SharedPreferences.OnSharedPreferenceChangeListener {

    //private static final String TAG = "SettingsActivity";
    private static SharedPreferences sharedPreferences;
    private HashSet<String> settingsKeyChanged = new HashSet<>();

    private GeneralPreferenceFragment generalPreferenceFragment;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        sharedPreferences = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
        sharedPreferences.registerOnSharedPreferenceChangeListener(this);

        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            // Show the Up button in the action bar.
            actionBar.setDisplayHomeAsUpEnabled(true);
        }

        generalPreferenceFragment = new GeneralPreferenceFragment();
        getFragmentManager().beginTransaction().replace(android.R.id.content, generalPreferenceFragment).commit();
    }

    @Override
    protected void onDestroy() {
        sharedPreferences.unregisterOnSharedPreferenceChangeListener(this);
        super.onDestroy();
    }

    @Override
    public boolean onMenuItemSelected(int featureId, MenuItem item) {
        int id = item.getItemId();
        if (id == android.R.id.home) {
            if (!super.onMenuItemSelected(featureId, item)) {
                NavUtils.navigateUpFromSameTask(this);
            }
            return true;
        }
        return super.onMenuItemSelected(featureId, item);
    }

    @Override
    public void onBackPressed() {
        Intent resultIntent = new Intent();
        resultIntent.putExtra("changedKeys", settingsKeyChanged.toArray(new String[0]));
        setResult(Activity.RESULT_OK, resultIntent);
        finish();
        super.onBackPressed();
    }

    /**
     * Helper method to determine if the device has an extra-large screen. For
     * example, 10" tablets are extra-large.
     */
    private static boolean isXLargeTablet(Context context) {
        return (context.getResources().getConfiguration().screenLayout
                & Configuration.SCREENLAYOUT_SIZE_MASK) >= Configuration.SCREENLAYOUT_SIZE_XLARGE;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean onIsMultiPane() {
        return isXLargeTablet(this);
    }

    /**
     * This method stops fragment injection in malicious applications.
     * Make sure to deny any unknown fragments here.
     */
    protected boolean isValidFragment(String fragmentName) {
        return PreferenceFragment.class.getName().equals(fragmentName)
                || GeneralPreferenceFragment.class.getName().equals(fragmentName)
                ;
    }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
        settingsKeyChanged.add(key);
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        if(resultCode == Activity.RESULT_OK && data != null) {
            if(requestCode == MainActivity.INTENT_PORT2LOAD) {
                Uri uri = data.getData();
                //Log.d(TAG, "onActivityResult INTENT_PORT2LOAD " + uri.toString());
                String url = null;
                if (uri != null) {
                    url = uri.toString();
                    SharedPreferences.Editor editor = sharedPreferences.edit();
                    editor.putString("settings_port2load", url);
                    editor.apply();
                    makeUriPersistable(data, uri);
                    if(generalPreferenceFragment != null)
                        generalPreferenceFragment.updatePort2LoadFilename(url);
                }
            }
        }
        super.onActivityResult(requestCode, resultCode, data);
    }

    private void makeUriPersistable(Intent data, Uri uri) {
        final int takeFlags = data.getFlags() & (Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        getContentResolver().takePersistableUriPermission(uri, takeFlags);
    }

    /**
     * This fragment shows general preferences only. It is used when the
     * activity is showing a two-pane settings UI.
     */
    public static class GeneralPreferenceFragment extends PreferenceFragment {

        Preference preferencePort2load = null;

        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            addPreferencesFromResource(R.xml.pref_general);
            setHasOptionsMenu(true);


            // General settings

            Preference preferenceRealspeed = findPreference("settings_realspeed");
            Preference.OnPreferenceChangeListener onPreferenceChangeListenerRealspeed = new Preference.OnPreferenceChangeListener() {
                @Override
                public boolean onPreferenceChange(Preference preference, Object value) {
                    String stringValue = value.toString();
                    preference.setSummary(stringValue);
                    return true;
                }
            };
            preferenceRealspeed.setOnPreferenceChangeListener(onPreferenceChangeListenerRealspeed);
            onPreferenceChangeListenerRealspeed.onPreferenceChange(preferenceRealspeed, sharedPreferences.getBoolean(preferenceRealspeed.getKey(), false));

            Preference preferenceGrayscale = findPreference("settings_grayscale");
            Preference.OnPreferenceChangeListener onPreferenceChangeListenerGrayscale = new Preference.OnPreferenceChangeListener() {
                @Override
                public boolean onPreferenceChange(Preference preference, Object value) {
                    String stringValue = value.toString();
                    preference.setSummary(stringValue);
                    return true;
                }
            };
            preferenceGrayscale.setOnPreferenceChangeListener(onPreferenceChangeListenerGrayscale);
            onPreferenceChangeListenerGrayscale.onPreferenceChange(preferenceGrayscale, sharedPreferences.getBoolean(preferenceGrayscale.getKey(), false));

            Preference preferenceAutosave = findPreference("settings_autosave");
            Preference.OnPreferenceChangeListener onPreferenceChangeListenerAutosave = new Preference.OnPreferenceChangeListener() {
                @Override
                public boolean onPreferenceChange(Preference preference, Object value) {
                    String stringValue = value.toString();
                    preference.setSummary(stringValue);
                    return true;
                }
            };
            preferenceAutosave.setOnPreferenceChangeListener(onPreferenceChangeListenerAutosave);
            onPreferenceChangeListenerAutosave.onPreferenceChange(preferenceAutosave, sharedPreferences.getBoolean(preferenceAutosave.getKey(), false));

            Preference preferenceObjectloadwarning = findPreference("settings_objectloadwarning");
            Preference.OnPreferenceChangeListener onPreferenceChangeListenerObjectloadwarning = new Preference.OnPreferenceChangeListener() {
                @Override
                public boolean onPreferenceChange(Preference preference, Object value) {
                    String stringValue = value.toString();
                    preference.setSummary(stringValue);
                    return true;
                }
            };
            preferenceObjectloadwarning.setOnPreferenceChangeListener(onPreferenceChangeListenerObjectloadwarning);
            onPreferenceChangeListenerObjectloadwarning.onPreferenceChange(preferenceObjectloadwarning, sharedPreferences.getBoolean(preferenceObjectloadwarning.getKey(), false));

            Preference preferenceAlwaysdisplog = findPreference("settings_alwaysdisplog");
            Preference.OnPreferenceChangeListener onPreferenceChangeListenerAlwaysdisplog = new Preference.OnPreferenceChangeListener() {
                @Override
                public boolean onPreferenceChange(Preference preference, Object value) {
                    String stringValue = value.toString();
                    preference.setSummary(stringValue);
                    return true;
                }
            };
            preferenceAlwaysdisplog.setOnPreferenceChangeListener(onPreferenceChangeListenerAlwaysdisplog);
            onPreferenceChangeListenerAlwaysdisplog.onPreferenceChange(preferenceAlwaysdisplog, sharedPreferences.getBoolean(preferenceAlwaysdisplog.getKey(), false));

            Preference preferenceAllowSound = findPreference("settings_allow_sound");
            if(preferenceAllowSound != null && !NativeLib.getSoundEnabled()) {
                preferenceAllowSound.setSummary("Cannot initialize the sound engine.");
                preferenceAllowSound.setEnabled(false);
            }

            // Ports 1 & 2 settings

            final Preference preferencePort1en = findPreference("settings_port1en");
            final Preference preferencePort1wr = findPreference("settings_port1wr");
            final Preference preferencePort2en = findPreference("settings_port2en");
            final Preference preferencePort2wr = findPreference("settings_port2wr");
            preferencePort2load = findPreference("settings_port2load");

            final boolean enablePortPreferences = NativeLib.isPortExtensionPossible();

            Preference.OnPreferenceChangeListener onPreferenceChangeListenerPort1en = new Preference.OnPreferenceChangeListener() {
                @Override
                public boolean onPreferenceChange(Preference preference, Object value) {
                    String stringValue = value.toString();
                    preference.setSummary(stringValue);
                    preferencePort1en.setEnabled(enablePortPreferences);
                    preferencePort1wr.setEnabled(enablePortPreferences);
                    return true;
                }
            };
            preferencePort1en.setOnPreferenceChangeListener(onPreferenceChangeListenerPort1en);
            onPreferenceChangeListenerPort1en.onPreferenceChange(preferencePort1en, sharedPreferences.getBoolean(preferencePort1en.getKey(), false));

            Preference.OnPreferenceChangeListener onPreferenceChangeListenerPort1wr = new Preference.OnPreferenceChangeListener() {
                @Override
                public boolean onPreferenceChange(Preference preference, Object value) {
                    String stringValue = value.toString();
                    preference.setSummary(stringValue);
                    return true;
                }
            };
            preferencePort1wr.setOnPreferenceChangeListener(onPreferenceChangeListenerPort1wr);
            onPreferenceChangeListenerPort1wr.onPreferenceChange(preferencePort1wr, sharedPreferences.getBoolean(preferencePort1wr.getKey(), false));

            Preference.OnPreferenceChangeListener onPreferenceChangeListenerPort2en = new Preference.OnPreferenceChangeListener() {
                @Override
                public boolean onPreferenceChange(Preference preference, Object value) {
                    String stringValue = value.toString();
                    preference.setSummary(stringValue);
                    preferencePort2en.setEnabled(enablePortPreferences);
                    preferencePort2wr.setEnabled(enablePortPreferences);
                    preferencePort2load.setEnabled(enablePortPreferences);
                    return true;
                }
            };
            preferencePort2en.setOnPreferenceChangeListener(onPreferenceChangeListenerPort2en);
            onPreferenceChangeListenerPort2en.onPreferenceChange(preferencePort2en, sharedPreferences.getBoolean(preferencePort2en.getKey(), false));

            Preference.OnPreferenceChangeListener onPreferenceChangeListenerPort2wr = new Preference.OnPreferenceChangeListener() {
                @Override
                public boolean onPreferenceChange(Preference preference, Object value) {
                    String stringValue = value.toString();
                    preference.setSummary(stringValue);
                    return true;
                }
            };
            preferencePort2wr.setOnPreferenceChangeListener(onPreferenceChangeListenerPort2wr);
            onPreferenceChangeListenerPort2wr.onPreferenceChange(preferencePort2wr, sharedPreferences.getBoolean(preferencePort2wr.getKey(), false));

            updatePort2LoadFilename(sharedPreferences.getString(preferencePort2load.getKey(), ""));
            preferencePort2load.setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
                @Override
                public boolean onPreferenceClick(Preference preference) {
                    Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                    intent.addCategory(Intent.CATEGORY_OPENABLE);
                    intent.setType("*/*");
                    intent.putExtra(Intent.EXTRA_TITLE, "shared.bin");
                    getActivity().startActivityForResult(intent, MainActivity.INTENT_PORT2LOAD);
                    return true;
                }
            });
        }

        @Override
        public boolean onOptionsItemSelected(MenuItem item) {
            int id = item.getItemId();
            if (id == android.R.id.home) {
                startActivity(new Intent(getActivity(), SettingsActivity.class));
                return true;
            }
            return super.onOptionsItemSelected(item);
        }

        public void updatePort2LoadFilename(String port2Filename) {
            if(preferencePort2load != null) {
                String displayName = port2Filename;
                try {
                    displayName = Utils.getFileName(getActivity(), port2Filename);
                } catch (Exception e) {
                }
                preferencePort2load.setSummary(displayName);
            }
        }
    }
}
