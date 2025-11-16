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
import android.app.Dialog;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Vibrator;
import android.os.VibratorManager;
import android.text.InputType;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.EditText;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatDialog;
import androidx.appcompat.app.AppCompatDialogFragment;
import androidx.appcompat.widget.Toolbar;
import androidx.fragment.app.Fragment;
import androidx.preference.Preference;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.SeekBarPreference;

import org.emulator.calculator.EmuApplication;
import org.emulator.calculator.NativeLib;
import org.emulator.calculator.Settings;
import org.emulator.calculator.Utils;
import org.emulator.calculator.usbserial.DevicesDialogFragment;
import org.emulator.calculator.usbserial.DevicesFragment;

import java.util.HashSet;
import java.util.Locale;

public class SettingsFragment extends AppCompatDialogFragment {

	private static final String TAG = "SettingsFragment";
	protected final boolean debug = false;

	private static Settings settings;
	private final HashSet<String> settingsKeyChanged = new HashSet<>();
	private final Settings.OnOneKeyChangedListener sharedPreferenceChangeListener = settingsKeyChanged::add;
	private GeneralPreferenceFragment generalPreferenceFragment;

	public interface OnSettingsKeyChangedListener {
		void onSettingsKeyChanged(HashSet<String> settingsKeyChanged);
	}

	private OnSettingsKeyChangedListener onSettingsKeyChangedListener;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		settings = EmuApplication.getSettings();
		settings.registerOnOneKeyChangeListener(sharedPreferenceChangeListener);

		setStyle(AppCompatDialogFragment.STYLE_NO_FRAME, R.style.AppTheme);
	}

	@NonNull
	@Override
	public Dialog onCreateDialog(Bundle savedInstanceState) {
		Dialog dialog = new AppCompatDialog(requireContext(), getTheme()) {
			@Override
			public void onBackPressed() {
				super.onBackPressed();
				dialogResult();
				dismiss();
			}
		};
		dialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
		return dialog;
	}

	@Override
	public View onCreateView(@NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {

		String title = getString(settings.getIsDefaultSettings() ? R.string.dialog_common_settings_title : R.string.dialog_state_settings_title);
		Dialog dialog = getDialog();
		if(dialog != null)
			dialog.setTitle(title);

		View view = inflater.inflate(R.layout.fragment_settings, container, false);

		// Configure the toolbar

		Toolbar toolbar = view.findViewById(R.id.my_toolbar);
		toolbar.setTitle(title);
		Utils.colorizeDrawableWithColor(requireContext(), toolbar.getNavigationIcon(), android.R.attr.colorForeground);
		toolbar.setNavigationOnClickListener(v -> {
			dialogResult();
			dismiss();
		});
		toolbar.inflateMenu(R.menu.fragment_settings);
		Menu menu = toolbar.getMenu();
		menu.findItem(R.id.menu_settings_reset_to_default).setEnabled(settings.getIsDefaultSettings());
		menu.findItem(R.id.menu_settings_reset_to_common).setEnabled(!settings.getIsDefaultSettings());
		toolbar.setOnMenuItemClickListener(item -> {
			int id = item.getItemId();
			if(id == R.id.menu_settings_reset_to_default) {
				settings.clearCommonDefaultSettings();
				restartPreferenceFragment();
			} else if(id == R.id.menu_settings_reset_to_common) {
				settings.clearEmbeddedStateSettings();
				restartPreferenceFragment();
			}
			return true;
		});

		// Insert the Preference fragment
		restartPreferenceFragment();
		return view;
	}

	/**
	 * Start or restart the preference fragment to take into account the new settings.
	 */
	private void restartPreferenceFragment() {
		if(generalPreferenceFragment != null)
			getChildFragmentManager().beginTransaction().remove(generalPreferenceFragment).commit();
		generalPreferenceFragment = new GeneralPreferenceFragment();
		getChildFragmentManager().beginTransaction().replace(R.id.settingsContent, generalPreferenceFragment).commit();
	}

	/**
	 * Common method to handle the result of the dialog.
	 */
	private void dialogResult() {
		if (onSettingsKeyChangedListener != null)
			onSettingsKeyChangedListener.onSettingsKeyChanged(settingsKeyChanged);
	}

	@Override
	public void onDestroy() {
		settings.unregisterOnOneKeyChangeListener();
		super.onDestroy();
	}

	void registerOnSettingsKeyChangedListener(OnSettingsKeyChangedListener listener) {
		if(debug) Log.d(TAG, "registerOnSettingsKeyChangedListener()");
		onSettingsKeyChangedListener = listener;
	}

	void unregisterOnSettingsKeyChangedListener() {
		if(debug) Log.d(TAG, "unregisterOnSettingsKeyChangedListener()");
		onSettingsKeyChangedListener = null;
	}

	public static class GeneralPreferenceFragment extends PreferenceFragmentCompat {

		Preference preferencePort2load = null;

		@Override
		public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {

			// Register our own settings data store
			getPreferenceManager().setPreferenceDataStore(EmuApplication.getSettings());

			// Load the preferences from an XML resource
			setPreferencesFromResource(R.xml.pref_general, rootKey);


			// Sound settings

			SeekBarPreference preferenceSoundVolume = findPreference("settings_sound_volume");
			if(preferenceSoundVolume != null) {
				if(!NativeLib.getSoundEnabled()) {
					preferenceSoundVolume.setSummary("Cannot initialize the sound engine.");
					preferenceSoundVolume.setEnabled(false);
				} else {
					preferenceSoundVolume.setOnPreferenceClickListener(preference -> {
						AlertDialog.Builder alert = new AlertDialog.Builder(requireContext());
						alert.setTitle(R.string.settings_sound_volume_dialog_title);
						final EditText input = new EditText(getContext());
						input.setInputType(InputType.TYPE_CLASS_NUMBER);
						input.setRawInputType(Configuration.KEYBOARD_12KEY);
						input.setFocusable(true);
						input.setText(String.format(Locale.US,"%d", preferenceSoundVolume.getValue()));
						alert.setView(input);
						alert.setPositiveButton(R.string.message_ok, (dialog, whichButton) -> {
							String newValueText = input.getText().toString();
							try {
								int newValue = Integer.parseInt(newValueText);
								if(newValue >= preferenceSoundVolume.getMin() && newValue <= preferenceSoundVolume.getMax())
									preferenceSoundVolume.setValue(newValue);
							} catch (NumberFormatException ignored) {}
						});
						alert.setNegativeButton(R.string.message_cancel, (dialog, whichButton) -> {});
						alert.show();
						return true;
					});
				}
			}

			// Haptic feedback settings

			Vibrator vibrator;
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
				vibrator = ((VibratorManager)requireContext().getSystemService(Context.VIBRATOR_MANAGER_SERVICE)).getDefaultVibrator();
			} else {
				vibrator = (Vibrator) requireContext().getSystemService(Context.VIBRATOR_SERVICE);
			}

			SeekBarPreference preferenceHapticFeedbackDuration = findPreference("settings_haptic_feedback_duration");
			if(preferenceHapticFeedbackDuration != null) {
				preferenceHapticFeedbackDuration.setOnPreferenceChangeListener((preference, newValue) -> {
					if(newValue instanceof Integer)
						Utils.vibrate(vibrator, (int)newValue);
					return true;
				});
				preferenceHapticFeedbackDuration.setOnPreferenceClickListener(preference -> {
					AlertDialog.Builder alert = new AlertDialog.Builder(requireContext());
					alert.setTitle(R.string.settings_haptic_feedback_dialog_title);
					final EditText input = new EditText(getContext());
					input.setInputType(InputType.TYPE_CLASS_NUMBER);
					input.setRawInputType(Configuration.KEYBOARD_12KEY);
					input.setFocusable(true);
					input.setText(String.format(Locale.US,"%d", preferenceHapticFeedbackDuration.getValue()));
					alert.setView(input);
					alert.setPositiveButton(R.string.message_ok, (dialog, whichButton) -> {
						String newValueText = input.getText().toString();
						try {
							int newValue = Integer.parseInt(newValueText);
							if(newValue >= preferenceHapticFeedbackDuration.getMin() && newValue <= preferenceHapticFeedbackDuration.getMax()) {
								preferenceHapticFeedbackDuration.setValue(newValue);
								Utils.vibrate(vibrator, newValue);
							}
						} catch (NumberFormatException ignored) {}
					});
					alert.setNegativeButton(R.string.message_cancel, (dialog, whichButton) -> {});
					alert.show();
					return true;
				});
			}

			// Background color settings

			Preference preferenceBackgroundFallbackColor = findPreference("settings_background_fallback_color");
//            final ColorPickerPreferenceCompat preferenceBackgroundCustomColor = (ColorPickerPreferenceCompat)findPreference("settings_background_custom_color");
			if(preferenceBackgroundFallbackColor != null /*&& preferenceBackgroundCustomColor != null*/) {
				final String[] stringArrayBackgroundFallbackColor = getResources().getStringArray(R.array.settings_background_fallback_color_item);
				Preference.OnPreferenceChangeListener onPreferenceChangeListenerBackgroundFallbackColor = (preference, value) -> {
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
				};
				preferenceBackgroundFallbackColor.setOnPreferenceChangeListener(onPreferenceChangeListenerBackgroundFallbackColor);
				onPreferenceChangeListenerBackgroundFallbackColor.onPreferenceChange(preferenceBackgroundFallbackColor,
						settings.getString(preferenceBackgroundFallbackColor.getKey(), "0"));


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
			Preference preferenceMacroManualSpeed = findPreference("settings_macro_manual_speed");
			if(preferenceMacroRealSpeed != null && preferenceMacroManualSpeed != null) {
				Preference.OnPreferenceChangeListener onPreferenceChangeListenerMacroRealSpeed = (preference, value) -> {
					if(value != null)
						preferenceMacroManualSpeed.setEnabled(!(Boolean) value);
					return true;
				};
				preferenceMacroRealSpeed.setOnPreferenceChangeListener(onPreferenceChangeListenerMacroRealSpeed);
				onPreferenceChangeListenerMacroRealSpeed.onPreferenceChange(preferenceMacroRealSpeed, settings.getBoolean(preferenceMacroRealSpeed.getKey(), true));
			}

			// Ports 1 & 2 settings

			Preference preferencePort1en = findPreference("settings_port1en");
			Preference preferencePort1wr = findPreference("settings_port1wr");
			Preference preferencePort2en = findPreference("settings_port2en");
			Preference preferencePort2wr = findPreference("settings_port2wr");
			preferencePort2load = findPreference("settings_port2load");
			if(preferencePort1en != null && preferencePort1wr != null
					&& preferencePort2en != null && preferencePort2wr != null
					&& preferencePort2load != null) {
				boolean enablePortPreferences = NativeLib.isPortExtensionPossible();

				Preference.OnPreferenceChangeListener onPreferenceChangeListenerPort1en = (preference, value) -> {
					preferencePort1en.setEnabled(enablePortPreferences);
					preferencePort1wr.setEnabled(enablePortPreferences);
					return true;
				};
				preferencePort1en.setOnPreferenceChangeListener(onPreferenceChangeListenerPort1en);
				onPreferenceChangeListenerPort1en.onPreferenceChange(preferencePort1en, settings.getBoolean(preferencePort1en.getKey(), false));

				Preference.OnPreferenceChangeListener onPreferenceChangeListenerPort2en = (preference, value) -> {
					preferencePort2en.setEnabled(enablePortPreferences);
					preferencePort2wr.setEnabled(enablePortPreferences);
					preferencePort2load.setEnabled(enablePortPreferences);
					return true;
				};
				preferencePort2en.setOnPreferenceChangeListener(onPreferenceChangeListenerPort2en);
				onPreferenceChangeListenerPort2en.onPreferenceChange(preferencePort2en, settings.getBoolean(preferencePort2en.getKey(), false));

				updatePort2LoadFilename(settings.getString(preferencePort2load.getKey(), ""));
				preferencePort2load.setOnPreferenceClickListener(preference -> {
					Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
					intent.addCategory(Intent.CATEGORY_OPENABLE);
					intent.setType("*/*");
					intent.putExtra(Intent.EXTRA_TITLE, "shared.bin");
					Fragment parentFragment = getParentFragment();
					if (parentFragment != null)
						parentFragment.startActivityForResult(intent, MainActivity.INTENT_PORT2LOAD);
					return true;
				});
			}


			// Port 2 flash (HP49 / HP50)

			Preference preferenceFlashPort2 = findPreference("settings_flash_port2");
			if(preferenceFlashPort2 != null) {
				int cCurrentRomType = NativeLib.getCurrentModel();
				if(cCurrentRomType == 'X' || cCurrentRomType == 'Q') {
					String flashPort2Filename = settings.getString(preferenceFlashPort2.getKey(), "");
					String displayName = flashPort2Filename;
					try {
						displayName = Utils.getFileName(getActivity(), flashPort2Filename);
					} catch (Exception e) {
						// Do nothing
					}
					preferenceFlashPort2.setSummary(displayName);
					//TODO offer the possibility to manage the Flash from here too.
					preferenceFlashPort2.setOnPreferenceClickListener(preference -> {
						new AlertDialog.Builder(requireContext())
								.setTitle(getString(R.string.settings_flash_port2_title))
								.setMessage(getString(R.string.settings_flash_port2_dialog_description))
								.setPositiveButton(android.R.string.ok, (dialog1, which1) -> {
								}).show();
						return true;
					});
				} else
					preferenceFlashPort2.setEnabled(false);
			}


			// Serial ports

			Preference preferenceSerialPortsWire = findPreference("settings_serial_ports_wire");
			if(preferenceSerialPortsWire != null) {
				String serialPortsWire = settings.getString(preferenceSerialPortsWire.getKey(), "");
				serialPortsWire = DevicesFragment.SerialConnectParameters.fromSettingsString(serialPortsWire).toDisplayString(requireContext());
				preferenceSerialPortsWire.setSummary(serialPortsWire);
				preferenceSerialPortsWire.setOnPreferenceClickListener(preference -> {
					DevicesDialogFragment devicesDialogFragment = new DevicesDialogFragment();
					devicesDialogFragment.setOnSerialDeviceClickedListener((serialConnectParameters) -> {
						preferenceSerialPortsWire.setSummary(serialConnectParameters.toDisplayString(requireContext()));
						settings.putString(preferenceSerialPortsWire.getKey(), serialConnectParameters.toSettingsString());
					});
					devicesDialogFragment.show(getChildFragmentManager(), "DevicesDialogFragment");
					return true;
				});
			}

			Preference preferenceSerialPortsIr = findPreference("settings_serial_ports_ir");
			if(preferenceSerialPortsIr != null) {
				String serialPortsIr = settings.getString(preferenceSerialPortsIr.getKey(), "");
				serialPortsIr = DevicesFragment.SerialConnectParameters.fromSettingsString(serialPortsIr).toDisplayString(requireContext());
				preferenceSerialPortsIr.setSummary(serialPortsIr);
				preferenceSerialPortsIr.setOnPreferenceClickListener(preference -> {
					DevicesDialogFragment devicesDialogFragment = new DevicesDialogFragment();
					devicesDialogFragment.setOnSerialDeviceClickedListener((serialConnectParameters) -> {
						preferenceSerialPortsIr.setSummary(serialConnectParameters.toDisplayString(requireContext()));
						settings.putString(preferenceSerialPortsIr.getKey(), serialConnectParameters.toSettingsString());
					});
					devicesDialogFragment.show(getChildFragmentManager(), "DevicesDialogFragment");
					return true;
				});
			}
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
				String url;
				if (uri != null) {
					if(debug) Log.d(TAG, "onActivityResult INTENT_PORT2LOAD " + uri.toString());
					url = uri.toString();
					settings.putString("settings_port2load", url);
					Utils.makeUriPersistable(requireContext(), data, uri);
					if(generalPreferenceFragment != null)
						generalPreferenceFragment.updatePort2LoadFilename(url);
				}
			}
		}
		super.onActivityResult(requestCode, resultCode, data);
	}
}
