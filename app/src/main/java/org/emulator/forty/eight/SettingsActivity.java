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

package org.emulator.forty.eight;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.view.MenuItem;

import java.util.HashSet;

import androidx.annotation.Nullable;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.preference.Preference;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceManager;

import org.emulator.calculator.NativeLib;
import org.emulator.calculator.Utils;

public class SettingsActivity extends AppCompatActivity implements SharedPreferences.OnSharedPreferenceChangeListener {

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
        getSupportFragmentManager().beginTransaction().replace(android.R.id.content, generalPreferenceFragment).commit();
    }

    @Override
    protected void onDestroy() {
        sharedPreferences.unregisterOnSharedPreferenceChangeListener(this);
        super.onDestroy();
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int id = item.getItemId();
        if (id == android.R.id.home) {
            onBackPressed();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onBackPressed() {
        Intent resultIntent = new Intent();
        resultIntent.putExtra("changedKeys", settingsKeyChanged.toArray(new String[0]));
        setResult(Activity.RESULT_OK, resultIntent);
        finish();
        //super.onBackPressed();
    }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
        settingsKeyChanged.add(key);
    }

    /**
     * This fragment shows general preferences only. It is used when the
     * activity is showing a two-pane settings UI.
     */
    public static class GeneralPreferenceFragment extends PreferenceFragmentCompat {

        Preference preferencePort2load = null;

        @Override
        public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
            addPreferencesFromResource(R.xml.pref_general);
            setHasOptionsMenu(true);

            // Sound settings

//            Preference preferenceAllowSound = findPreference("settings_allow_sound");
//            if(preferenceAllowSound != null && !NativeLib.getSoundEnabled()) {
//                preferenceAllowSound.setSummary("Cannot initialize the sound engine.");
//                preferenceAllowSound.setEnabled(false);
//            }
            Preference preferenceSoundVolume = findPreference("settings_sound_volume");
            if(preferenceSoundVolume != null && !NativeLib.getSoundEnabled()) {
                preferenceSoundVolume.setSummary("Cannot initialize the sound engine.");
                preferenceSoundVolume.setEnabled(false);
            }

            // Background color settings

            Preference preferenceBackgroundFallbackColor = findPreference("settings_background_fallback_color");
//            final ColorPickerPreferenceCompat preferenceBackgroundCustomColor = (ColorPickerPreferenceCompat)findPreference("settings_background_custom_color");
            if(preferenceBackgroundFallbackColor != null /*&& preferenceBackgroundCustomColor != null*/) {
                final String[] stringArrayBackgroundFallbackColor = getResources().getStringArray(R.array.settings_background_fallback_color_item);
                Preference.OnPreferenceChangeListener onPreferenceChangeListenerBackgroundFallbackColor = new Preference.OnPreferenceChangeListener() {
                    @Override
                    public boolean onPreferenceChange(Preference preference, Object value) {
                        if(value != null) {
                            String stringValue = value.toString();
                            int backgroundFallbackColor = -1;
                            try {
                                backgroundFallbackColor = Integer.parseInt(stringValue);
                            } catch (NumberFormatException ignored) {}
                            if(backgroundFallbackColor >= 0 && backgroundFallbackColor < stringArrayBackgroundFallbackColor.length)
                                preference.setSummary(stringArrayBackgroundFallbackColor[backgroundFallbackColor]);
//                            preferenceBackgroundCustomColor.setEnabled(backgroundFallbackColor == 2);
                        }
                        return true;
                    }
                };
                preferenceBackgroundFallbackColor.setOnPreferenceChangeListener(onPreferenceChangeListenerBackgroundFallbackColor);
                onPreferenceChangeListenerBackgroundFallbackColor.onPreferenceChange(preferenceBackgroundFallbackColor,
                        sharedPreferences.getString(preferenceBackgroundFallbackColor.getKey(), "0"));


                //preferenceBackgroundCustomColor.setColorValue(customColor);

//                Preference.OnPreferenceChangeListener onPreferenceChangeListenerBackgroundCustomColor = new Preference.OnPreferenceChangeListener() {
//                    @Override
//                    public boolean onPreferenceChange(Preference preference, Object value) {
//                        if(value != null) {
//                            int customColor = (Integer)value;
//                        }
//                        return true;
//                    }
//                };
//                preferenceBackgroundCustomColor.setOnPreferenceChangeListener(onPreferenceChangeListenerBackgroundCustomColor);
//                onPreferenceChangeListenerBackgroundCustomColor.onPreferenceChange(preferenceBackgroundCustomColor, sharedPreferences.getBoolean(preferenceBackgroundCustomColor.getKey(), false));
            }

            // Macro

            Preference preferenceMacroRealSpeed = findPreference("settings_macro_real_speed");
            final Preference preferenceMacroManualSpeed = findPreference("settings_macro_manual_speed");
            if(preferenceMacroRealSpeed != null && preferenceMacroManualSpeed != null) {
                Preference.OnPreferenceChangeListener onPreferenceChangeListenerMacroRealSpeed = new Preference.OnPreferenceChangeListener() {
                    @Override
                    public boolean onPreferenceChange(Preference preference, Object value) {
                        if(value != null)
                            preferenceMacroManualSpeed.setEnabled(!(Boolean) value);
                        return true;
                    }
                };
                preferenceMacroRealSpeed.setOnPreferenceChangeListener(onPreferenceChangeListenerMacroRealSpeed);
                onPreferenceChangeListenerMacroRealSpeed.onPreferenceChange(preferenceMacroRealSpeed, sharedPreferences.getBoolean(preferenceMacroRealSpeed.getKey(), true));
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
                    preferencePort1en.setEnabled(enablePortPreferences);
                    preferencePort1wr.setEnabled(enablePortPreferences);
                    return true;
                }
            };
            preferencePort1en.setOnPreferenceChangeListener(onPreferenceChangeListenerPort1en);
            onPreferenceChangeListenerPort1en.onPreferenceChange(preferencePort1en, sharedPreferences.getBoolean(preferencePort1en.getKey(), false));

            Preference.OnPreferenceChangeListener onPreferenceChangeListenerPort2en = new Preference.OnPreferenceChangeListener() {
                @Override
                public boolean onPreferenceChange(Preference preference, Object value) {
                    preferencePort2en.setEnabled(enablePortPreferences);
                    preferencePort2wr.setEnabled(enablePortPreferences);
                    preferencePort2load.setEnabled(enablePortPreferences);
                    return true;
                }
            };
            preferencePort2en.setOnPreferenceChangeListener(onPreferenceChangeListenerPort2en);
            onPreferenceChangeListenerPort2en.onPreferenceChange(preferencePort2en, sharedPreferences.getBoolean(preferencePort2en.getKey(), false));

            updatePort2LoadFilename(sharedPreferences.getString(preferencePort2load.getKey(), ""));
            preferencePort2load.setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
                @Override
                public boolean onPreferenceClick(Preference preference) {
                    Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                    intent.addCategory(Intent.CATEGORY_OPENABLE);
                    intent.setType("*/*");
                    intent.putExtra(Intent.EXTRA_TITLE, "shared.bin");
                    Activity activity = getActivity();
                    if(activity != null)
                        activity.startActivityForResult(intent, MainActivity.INTENT_PORT2LOAD);
                    return true;
                }
            });
        }

        void updatePort2LoadFilename(String port2Filename) {
            if(preferencePort2load != null) {
                String displayName = port2Filename;
                try {
                    displayName = Utils.getFileName(getActivity(), port2Filename);
                } catch (Exception e) {
                    // Do nothing
                }
                preferencePort2load.setSummary(displayName);
            }
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        if(resultCode == Activity.RESULT_OK && data != null) {
            if(requestCode == MainActivity.INTENT_PORT2LOAD) {
                Uri uri = data.getData();
                //Log.d(TAG, "onActivityResult INTENT_PORT2LOAD " + uri.toString());
                String url;
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
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT)
            getContentResolver().takePersistableUriPermission(uri, takeFlags);
    }
}
