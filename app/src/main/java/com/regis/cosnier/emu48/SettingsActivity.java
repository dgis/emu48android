package com.regis.cosnier.emu48;

import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceFragment;
import android.preference.PreferenceManager;
import android.provider.OpenableColumns;
import android.view.MenuItem;

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
public class SettingsActivity extends AppCompatPreferenceActivity {

    private static final String TAG = "SettingsActivity";
    private static SharedPreferences sharedPreferences;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        sharedPreferences = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());

        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            // Show the Up button in the action bar.
            actionBar.setDisplayHomeAsUpEnabled(true);
        }

        getFragmentManager().beginTransaction().replace(android.R.id.content, new GeneralPreferenceFragment()).commit();

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

    /**
     * A preference value change listener that updates the preference's summary
     * to reflect its new value.
     */
    private static Preference.OnPreferenceChangeListener sBindPreferenceSummaryToValueListener = new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange(Preference preference, Object value) {
            String stringValue = value.toString();

            if (preference instanceof ListPreference) {
                // For list preferences, look up the correct display value in
                // the preference's 'entries' list.
                ListPreference listPreference = (ListPreference) preference;
                int index = listPreference.findIndexOfValue(stringValue);

                // Set the summary to reflect the new value.
                preference.setSummary(
                        index >= 0
                                ? listPreference.getEntries()[index]
                                : null);

            } else {
                // For all other preferences, set the summary to the value's
                // simple string representation.
                preference.setSummary(stringValue);
            }
            return true;
        }
    };

    /**
     * Helper method to determine if the device has an extra-large screen. For
     * example, 10" tablets are extra-large.
     */
    private static boolean isXLargeTablet(Context context) {
        return (context.getResources().getConfiguration().screenLayout
                & Configuration.SCREENLAYOUT_SIZE_MASK) >= Configuration.SCREENLAYOUT_SIZE_XLARGE;
    }

//    /**
//     * Binds a preference's summary to its value. More specifically, when the
//     * preference's value is changed, its summary (line of text below the
//     * preference title) is updated to reflect the value. The summary is also
//     * immediately updated upon calling this method. The exact display format is
//     * dependent on the type of preference.
//     *
//     * @see #sBindPreferenceSummaryToValueListener
//     */
//    private static void bindPreferenceSummaryToStringValue(Preference preference) {
//        preference.setOnPreferenceChangeListener(sBindPreferenceSummaryToValueListener);
//        sBindPreferenceSummaryToValueListener.onPreferenceChange(preference, sharedPreferences.getString(preference.getKey(), ""));
//    }
//
//    /**
//     * Binds a preference's summary to its value. More specifically, when the
//     * preference's value is changed, its summary (line of text below the
//     * preference title) is updated to reflect the value. The summary is also
//     * immediately updated upon calling this method. The exact display format is
//     * dependent on the type of preference.
//     *
//     * @see #sBindPreferenceSummaryToValueListener
//     */
//    private static void bindPreferenceSummaryToIntValue(Preference preference) {
//        preference.setOnPreferenceChangeListener(sBindPreferenceSummaryToValueListener);
//        sBindPreferenceSummaryToValueListener.onPreferenceChange(preference, sharedPreferences.getInt(preference.getKey(), 0));
//    }
//
//    /**
//     * Binds a preference's summary to its value. More specifically, when the
//     * preference's value is changed, its summary (line of text below the
//     * preference title) is updated to reflect the value. The summary is also
//     * immediately updated upon calling this method. The exact display format is
//     * dependent on the type of preference.
//     *
//     * @see #sBindPreferenceSummaryToValueListener
//     */
//    private static void bindPreferenceSummaryToBoolValue(Preference preference) {
//        // Set the listener to watch for value changes.
//        preference.setOnPreferenceChangeListener(sBindPreferenceSummaryToValueListener);
//        sBindPreferenceSummaryToValueListener.onPreferenceChange(preference, sharedPreferences.getBoolean(preference.getKey(), false));
//    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean onIsMultiPane() {
        return isXLargeTablet(this);
    }

//    /**
//     * {@inheritDoc}
//     */
//    @Override
//    @TargetApi(Build.VERSION_CODES.HONEYCOMB)
//    public void onBuildHeaders(List<Header> target) {
//        loadHeadersFromResource(R.xml.pref_headers, target);
//    }

    /**
     * This method stops fragment injection in malicious applications.
     * Make sure to deny any unknown fragments here.
     */
    protected boolean isValidFragment(String fragmentName) {
        return PreferenceFragment.class.getName().equals(fragmentName)
                || GeneralPreferenceFragment.class.getName().equals(fragmentName)
//                || DataSyncPreferenceFragment.class.getName().equals(fragmentName)
//                || NotificationPreferenceFragment.class.getName().equals(fragmentName)
                ;
    }

    /**
     * This fragment shows general preferences only. It is used when the
     * activity is showing a two-pane settings UI.
     */
    @TargetApi(Build.VERSION_CODES.HONEYCOMB)
    public static class GeneralPreferenceFragment extends PreferenceFragment {
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

//            Preference preferenceAutosaveonexit = findPreference("settings_autosaveonexit");
//            Preference.OnPreferenceChangeListener onPreferenceChangeListenerAutosaveonexit = new Preference.OnPreferenceChangeListener() {
//                @Override
//                public boolean onPreferenceChange(Preference preference, Object value) {
//                    String stringValue = value.toString();
//                    preference.setSummary(stringValue);
//                    return true;
//                }
//            };
//            preferenceAutosaveonexit.setOnPreferenceChangeListener(onPreferenceChangeListenerAutosaveonexit);
//            onPreferenceChangeListenerAutosaveonexit.onPreferenceChange(preferenceAutosaveonexit, sharedPreferences.getBoolean(preferenceAutosaveonexit.getKey(), false));

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

            // Ports 1 & 2 settings

            final Preference preferencePort1en = findPreference("settings_port1en");
            final Preference preferencePort1wr = findPreference("settings_port1wr");
            final Preference preferencePort2en = findPreference("settings_port2en");
            final Preference preferencePort2wr = findPreference("settings_port2wr");
            final Preference preferencePort2load = findPreference("settings_port2load");

            final boolean enablePortPreferences = NativeLib.isPortExtensionPossible();

            Preference.OnPreferenceChangeListener onPreferenceChangeListenerPort1en = new Preference.OnPreferenceChangeListener() {
                @Override
                public boolean onPreferenceChange(Preference preference, Object value) {
                    Boolean booleanValue = (Boolean)value;
                    String stringValue = value.toString();
                    preference.setSummary(stringValue);
                    preferencePort1en.setEnabled(enablePortPreferences);
                    preferencePort1wr.setEnabled(enablePortPreferences /*&& booleanValue.booleanValue()*/);
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
                    Boolean booleanValue = (Boolean)value;
                    String stringValue = value.toString();
                    preference.setSummary(stringValue);
                    preferencePort2en.setEnabled(enablePortPreferences);
                    preferencePort2wr.setEnabled(enablePortPreferences /*&& booleanValue.booleanValue()*/);
                    preferencePort2load.setEnabled(enablePortPreferences /*&& booleanValue.booleanValue()*/);
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

            Preference.OnPreferenceChangeListener onPreferenceChangeListenerPort2load = new Preference.OnPreferenceChangeListener() {
                @Override
                public boolean onPreferenceChange(Preference preference, Object value) {
                    String stringValue = value.toString();
                    String displayName = stringValue;
                    try {
                        displayName = getFileName(getActivity(), stringValue);
                    } catch(Exception e) {
                    }
                    preference.setSummary(displayName);
                    return true;
                }
            };
            preferencePort2load.setOnPreferenceChangeListener(onPreferenceChangeListenerPort2load);
            onPreferenceChangeListenerPort2load.onPreferenceChange(preferencePort2load, sharedPreferences.getString(preferencePort2load.getKey(), ""));
            preferencePort2load.setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
                @Override
                public boolean onPreferenceClick(Preference preference) {
                    Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                    intent.addCategory(Intent.CATEGORY_OPENABLE);
                    //intent.setType("YOUR FILETYPE"); //not needed, but maybe usefull
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
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        if(resultCode == Activity.RESULT_OK) {
            if(requestCode == MainActivity.INTENT_PORT2LOAD) {
                Uri uri = data.getData();
                //Log.d(TAG, "onActivityResult INTENT_PORT2LOAD " + uri.toString());
                String url = uri.toString();
                SharedPreferences.Editor editor = sharedPreferences.edit();
                editor.putString("settings_port2load", url);
                editor.commit();
                makeUriPersistable(data, uri);
            }
        }
        super.onActivityResult(requestCode, resultCode, data);
    }

    private void makeUriPersistable(Intent data, Uri uri) {
        //grantUriPermission(getPackageName(), uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
        final int takeFlags = data.getFlags() & (Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        getContentResolver().takePersistableUriPermission(uri, takeFlags);
    }

    public static String getFileName(Context context, String url) {
        Uri uri = Uri.parse(url);
        String result = null;
        if (uri.getScheme().equals("content")) {
            Cursor cursor = context.getContentResolver().query(uri, null, null, null, null);
            try {
                if (cursor != null && cursor.moveToFirst()) {
                    result = cursor.getString(cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME));
                }
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
}
