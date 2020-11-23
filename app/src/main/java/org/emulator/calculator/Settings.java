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

package org.emulator.calculator;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.PreferenceDataStore;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONTokener;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class Settings extends PreferenceDataStore {

	private static final String TAG = "Settings";
	protected final boolean debug = false;

	private final SharedPreferences androidSettings;

	// Defined the setting keys which are only defined at the application level.
	private List<String> applicationSettingKeys = Arrays.asList("settings_kml_default", "settings_kml_folder", "lastDocument", "MRU");

	// The settings only defined at the application level.
	private final HashMap<String, Object> applicationSettings = new HashMap<>();

	// The commonSettings are used when no calculator is loaded, and are the default settings of the embeddedStateSettings.
	private final HashMap<String, Object> commonSettings = new HashMap<>();

	// The embeddedStateSettings are saved in the calculator state file.
	private final HashMap<String, Object> embeddedStateSettings = new HashMap<>();

	private boolean isCommonSettings = true;

	public interface OnOneKeyChangedListener {
		void onOneKeyChanged(String keyChanged);
	}
	private OnOneKeyChangedListener oneKeyChangedListener;
	private static String magic = "MYHP";



	public Settings(@NonNull SharedPreferences sharedPreferences) {
		if(debug) Log.d(TAG, "Settings()");
		androidSettings = sharedPreferences;

		loadApplicationSettings();
	}

	public void registerOnOneKeyChangeListener(OnOneKeyChangedListener listener) {
		if(debug) Log.d(TAG, "registerOnOneKeyChangeListener()");
		oneKeyChangedListener = listener;
	}

	public void unregisterOnOneKeyChangeListener() {
		if(debug) Log.d(TAG, "unregisterOnOneKeyChangeListener()");
		oneKeyChangedListener = null;
	}

	private void putValue(String key, @Nullable Object value) {
		if(applicationSettingKeys.contains(key))
			applicationSettings.put(key, value);
		else if(isCommonSettings)
			commonSettings.put(key, value);
		else
			embeddedStateSettings.put(key, value);
		if(oneKeyChangedListener != null)
			oneKeyChangedListener.onOneKeyChanged(key);
	}

	private Object getValue(String key) {
		Object value = null;
		if(!isCommonSettings)
			value = embeddedStateSettings.get(key);
		if(value == null) {
			if(applicationSettingKeys.contains(key))
				value = applicationSettings.get(key);
			else
				value = commonSettings.get(key);
		}
		return value;
	}

	public void removeValue(String key) {
		if(applicationSettingKeys.contains(key))
			applicationSettings.remove(key);
		else if(isCommonSettings)
			commonSettings.remove(key);
		else
			embeddedStateSettings.remove(key);
		if(oneKeyChangedListener != null)
			oneKeyChangedListener.onOneKeyChanged(key);
	}

	public boolean hasValue(String key) {
		if(debug) Log.d(TAG, "has(key: '" + key + "')");
		if(applicationSettingKeys.contains(key))
			return applicationSettings.containsKey(key);
		else if(isCommonSettings)
			return commonSettings.containsKey(key);
		else
			return embeddedStateSettings.containsKey(key);
	}

	public boolean getIsDefaultSettings() {
		return isCommonSettings;
	}

	public void setIsDefaultSettings(boolean isDefaultSettings) {
		this.isCommonSettings = isDefaultSettings;
	}

	private static String toJSON(Collection<String> array) {
		StringBuilder json = new StringBuilder();
		json.append("[");
		boolean first = true;
		for (String string : array) {
			if (first)
				first = false;
			else
				json.append(",");
			json.append("\"").append(string).append("\":");
		}
		json.append("]");
		return json.toString();
	}

	@SuppressWarnings("unchecked")
	private static String toJSON(Map<String, Object> map) {
		StringBuilder json = new StringBuilder();
		json.append("{");
		boolean first = true;
		for (String key : map.keySet()) {
			if (first)
				first = false;
			else
				json.append(",");
			json.append("\"").append(key).append("\":");
			Object value = map.get(key);
			if (value instanceof Collection<?>)
				json.append(toJSON((Collection<String>) value));
			else if (value instanceof String)
				json.append("\"").append(value).append("\"");
			else
				json.append(value);
		}
		json.append("}");
		return json.toString();
	}

	private static HashMap<String, Object> fromJSON(String json) {
		try {
			return fromJSON((JSONObject)new JSONTokener(json).nextValue());
		} catch (JSONException e) {
			e.printStackTrace();
		}
		return new HashMap<>();
	}

	private static HashMap<String, Object> fromJSON(JSONObject jsonObject) {
		HashMap<String, Object> result = new HashMap<>();
		Iterator<String> keys = jsonObject.keys();
		while(keys.hasNext()) {
			String key = keys.next();
			try {
				Object value = jsonObject.get(key);
				if (value instanceof Integer || value instanceof Long)
					result.put(key, ((Number)value).longValue());
				else if (value instanceof Boolean)
					result.put(key, (Boolean) value);
				else if (value instanceof Float || value instanceof Double)
					result.put(key, ((Number)value).doubleValue());
				else if (value instanceof JSONObject)
					result.put(key, fromJSON((JSONObject)value));
				else if (value instanceof JSONArray) {
					JSONArray jsonArray = (JSONArray) value;
					int length = jsonArray.length();
					Set<String> valueArray = new HashSet<>();
					for(int i = 0; i < length; i++) {
						try {
							valueArray.add(jsonArray.get(i).toString());
						} catch (JSONException e1) { /* No need */ }
					}
					result.put(key, valueArray);
				} else if (JSONObject.NULL.equals(value))
					result.put(key, null);
				else
					result.put(key, jsonObject.getString(key));
			} catch (JSONException e) {
				e.printStackTrace();
			}
		}
		return result;
	}

	public void saveInStateFile(Context context, String url) {
		if(debug) Log.d(TAG, "saveInStateFile(url: '" + url + "')");

		// Consolidate common and embedded settings but without the app only settings
		Map<String, Object> stateSettings = new HashMap<>();
		stateSettings.putAll(commonSettings);
		stateSettings.putAll(embeddedStateSettings);

		String json = toJSON(stateSettings);
		byte[] jsonBytes = json.getBytes(StandardCharsets.UTF_8);
		int jsonLength = jsonBytes.length;

		if(jsonLength < 65536) {
			Uri uri = Uri.parse(url);
			try {
				ParcelFileDescriptor pfd = context.getContentResolver().openFileDescriptor(uri, "wa");
				if (pfd != null) {
					FileOutputStream fileOutputStream = new FileOutputStream(pfd.getFileDescriptor());

					fileOutputStream.write(jsonBytes);
					// The JSON text should not be more than 64KB
					fileOutputStream.write((jsonLength >> 8) & 0xFF);
					fileOutputStream.write(jsonLength & 0xFF);
					for (int i = 0; i < magic.length(); i++) {
						fileOutputStream.write(magic.charAt(i));
					}
					fileOutputStream.close();
				}
			} catch (IOException e) {
				if(debug) Log.d(TAG, "saveInStateFile() Error");
				e.printStackTrace();
			}
		}
	}

	public void clearEmbeddedStateSettings() {
		embeddedStateSettings.clear();
	}

	@SuppressLint("ApplySharedPref")
	public void clearCommonDefaultSettings() {
		embeddedStateSettings.clear();
		commonSettings.clear();
	}

	@SuppressWarnings("ResultOfMethodCallIgnored")
	public void loadFromStateFile(Context context, String url) {
		if(debug) Log.d(TAG, "loadFromStateFile(url: '" + url + "')");
		Uri uri = Uri.parse(url);
		try {
			ParcelFileDescriptor pfd = context.getContentResolver().openFileDescriptor(uri, "r");
			if(pfd != null) {
				byte[] lastChunk = new byte[64 * 1024];
				long fileSize = pfd.getStatSize();
				FileInputStream fileInputStream = new FileInputStream(pfd.getFileDescriptor());
				if(fileSize > lastChunk.length) {
					long byteToSkip = fileSize - lastChunk.length;
					fileInputStream.skip(byteToSkip);
					fileInputStream.read(lastChunk, 0, lastChunk.length);
				} else {
					int lastChunkOffset = lastChunk.length - (int)fileSize;
					fileInputStream.read(lastChunk, lastChunkOffset, (int)fileSize);
				}
				fileInputStream.close();

				boolean magicFound = true;
				int magicLength = magic.length();
				for (int i = 0; i < magicLength; i++) {
					if(lastChunk[lastChunk.length - magicLength + i] != magic.charAt(i)) {
						magicFound = false;
						break;
					}
				}
				if(!magicFound)
					return;

				// The JSON text should not be more than 64KB
				int jsonLength = ((lastChunk[lastChunk.length - magicLength - 2] << 8) & 0xFF00) | (lastChunk[lastChunk.length - magicLength - 1] & 0xFF);
				String json = new String(lastChunk, lastChunk.length - jsonLength - magicLength - 2, jsonLength, StandardCharsets.US_ASCII);
				HashMap<String, Object> settings = fromJSON(json);
				embeddedStateSettings.putAll(settings);
			}
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	private void loadApplicationSettings() {
		commonSettings.clear();
		Map<String, ?> keyValuePairs = androidSettings.getAll();
		for (String key : keyValuePairs.keySet()) {
			if (applicationSettingKeys.indexOf(key) != -1)
				applicationSettings.put(key, keyValuePairs.get(key));
			else
				commonSettings.put(key, keyValuePairs.get(key));
		}
	}

	public void saveApplicationSettings() {
		if(debug) Log.d(TAG, "saveApplicationSettings()");

		SharedPreferences.Editor settingsEditor = androidSettings.edit();
		for (String key : commonSettings.keySet()) {
			Object value = commonSettings.get(key);
			putKeyValueInEditor(settingsEditor, key, value);
		}
		for (String key : applicationSettingKeys) {
			Object value = applicationSettings.get(key);
			putKeyValueInEditor(settingsEditor, key, value);
		}

		settingsEditor.apply();
	}

	private void putKeyValueInEditor(SharedPreferences.Editor settingsEditor, String key, Object value) {
		if (value instanceof Integer)
			settingsEditor.putInt(key, ((Number)value).intValue());
		else if (value instanceof Long)
			settingsEditor.putLong(key, ((Number)value).longValue());
		else if (value instanceof Boolean)
			settingsEditor.putBoolean(key, (Boolean) value);
		else if (value instanceof Float || value instanceof Double)
			settingsEditor.putFloat(key, ((Number)value).floatValue());
		else if (value instanceof String)
			settingsEditor.putString(key, (String) value);
		else if (value instanceof Set<?>)
			settingsEditor.putStringSet(key, (Set<String>) value);
		else
			settingsEditor.putString(key, null);
	}


	// PreferenceDataStore

	@Override
	public void putString(String key, @Nullable String value) {
		if(debug) Log.d(TAG, (isCommonSettings ? "DEFAULT" : "LOCAL") + " putString(key: '" + key + "' value: '" + value + "')");
		putValue(key, value);
	}

	@Override
	public void putStringSet(String key, @Nullable Set<String> value) {
		if(debug) Log.d(TAG, (isCommonSettings ? "DEFAULT" : "LOCAL") + " putStringSet(key: '" + key + "' value: '" + "" + "')");
		putValue(key, value);
	}

	@Override
	public void putInt(String key, int value) {
		if(debug) Log.d(TAG, (isCommonSettings ? "DEFAULT" : "LOCAL") + " putInt(key: '" + key + "' value: '" + value + "')");
		putValue(key, value);
	}

	@Override
	public void putLong(String key, long value){
		if(debug) Log.d(TAG, (isCommonSettings ? "DEFAULT" : "LOCAL") + " putLong(key: '" + key + "' value: '" + value + "')");
		putValue(key, value);
	}

	@Override
	public void putFloat(String key, float value){
		if(debug) Log.d(TAG, (isCommonSettings ? "DEFAULT" : "LOCAL") + " putFloat(key: '" + key + "' value: '" + value + "')");
		putValue(key, value);
	}

	@Override
	public void putBoolean(String key, boolean value){
		if(debug) Log.d(TAG, (isCommonSettings ? "DEFAULT" : "LOCAL") + " putBoolean(key: '" + key + "' value: '" + value + "')");
		putValue(key, value);
	}

	@Nullable
	@Override
	public String getString(String key, @Nullable String defValue) {
		Object result = getValue(key);
		if(debug) Log.d(TAG, "getString(key: '" + key + "') -> " + result);
		if(result instanceof String)
			return (String) result;
		return defValue;
	}

	@SuppressWarnings("unchecked")
	@Nullable
	@Override
	public Set<String> getStringSet(String key, @Nullable Set<String> defValues) {
		if(debug) Log.d(TAG, "getStringSet(key: '" + key + "')");
		Object result = getValue(key);
		if(result instanceof Set<?>)
			try {
				return (Set<String>) result;
			} catch (Exception ignored) {}
		return defValues;
	}

	@Override
	public int getInt(String key, int defValue) {
		Object result = getValue(key);
		if(debug) Log.d(TAG, "getInt(key: '" + key + "') -> " + result);
		if(result != null)
			try {
				return ((Number) result).intValue();
			} catch (Exception ignored) {}
		return defValue;
	}

	@Override
	public long getLong(String key, long defValue) {
		Object result = getValue(key);
		if(debug) Log.d(TAG, "getLong(key: '" + key + "') -> " + result);
		if(result != null)
			try {
				return ((Number) result).longValue();
			} catch (Exception ignored) {}
		return defValue;
	}

	@Override
	public float getFloat(String key, float defValue) {
		Object result = getValue(key);
		if(debug) Log.d(TAG, "getFloat(key: '" + key + "') -> " + result);
		if(result != null)
			try {
				return ((Number) result).floatValue();
			} catch (Exception ignored) {}
		return defValue;
	}

	@Override
	public boolean getBoolean(String key, boolean defValue) {
		Object result = getValue(key);
		if(debug) Log.d(TAG, "getBoolean(key: '" + key + "') -> " + result);
		if(result != null)
			try {
				return (Boolean) result;
			} catch (Exception ignored) {}
		return defValue;
	}
}
