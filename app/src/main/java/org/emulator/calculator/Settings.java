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
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;

public class Settings extends PreferenceDataStore {

	private static final String TAG = "Settings";
	protected final boolean debug = true;

	private final SharedPreferences defaultSettings;
	private final HashMap<String, Object> embeddedStateSettings = new HashMap<>();
	private SharedPreferences.OnSharedPreferenceChangeListener sharedPreferenceChangeListener;
	private static String magic = "MYHP";


	private boolean isDefaultSettings = true;


	public Settings(@NonNull SharedPreferences sharedPreferences) {
		if(debug) Log.d(TAG, "Settings()");
		this.defaultSettings = sharedPreferences;
	}

	public void registerOnSharedPreferenceChangeListener(SharedPreferences.OnSharedPreferenceChangeListener listener) {
		if(debug) Log.d(TAG, "registerOnSharedPreferenceChangeListener()");
		sharedPreferenceChangeListener = listener;
	}

	public void unregisterOnSharedPreferenceChangeListener(SharedPreferences.OnSharedPreferenceChangeListener listener) {
		if(debug) Log.d(TAG, "unregisterOnSharedPreferenceChangeListener()");
		sharedPreferenceChangeListener = null;
	}

	@Override
	public void putString(String key, @Nullable String value) {
		putString(key, value, false);
	}
	public void putString(String key, @Nullable String value, boolean forceDefault) {
		if(debug) Log.d(TAG, (isDefaultSettings ? "DEFAULT" : "LOCAL") + " putString(key: '" + key + "' value: '" + value + "')");
		if(isDefaultSettings || forceDefault) {
			defaultSettings.edit().putString(key, value).apply();
			if(sharedPreferenceChangeListener != null)
				sharedPreferenceChangeListener.onSharedPreferenceChanged(defaultSettings, key);
		} else {
			embeddedStateSettings.put(key, value);
			if(sharedPreferenceChangeListener != null)
				sharedPreferenceChangeListener.onSharedPreferenceChanged(null, key);
		}
	}

	@Override
	public void putStringSet(String key, @Nullable Set<String> value) {
		putStringSet(key, value, false);
	}
	public void putStringSet(String key, @Nullable Set<String> value, boolean forceDefault) {
		if(debug) Log.d(TAG, (isDefaultSettings ? "DEFAULT" : "LOCAL") + " putStringSet(key: '" + key + "' value: '" + "" + "')");
		if(isDefaultSettings || forceDefault) {
			defaultSettings.edit().putStringSet(key, value).apply();
			if(sharedPreferenceChangeListener != null)
				sharedPreferenceChangeListener.onSharedPreferenceChanged(defaultSettings, key);
		} else {
			embeddedStateSettings.put(key, value);
			if(sharedPreferenceChangeListener != null)
				sharedPreferenceChangeListener.onSharedPreferenceChanged(null, key);
		}
	}

	@Override
	public void putInt(String key, int value) {
		putInt(key, value, false);
	}
	public void putInt(String key, int value, boolean forceDefault) {
		if(debug) Log.d(TAG, (isDefaultSettings ? "DEFAULT" : "LOCAL") + " putInt(key: '" + key + "' value: '" + value + "')");
		if(isDefaultSettings || forceDefault) {
			defaultSettings.edit().putInt(key, value).apply();
			if(sharedPreferenceChangeListener != null)
				sharedPreferenceChangeListener.onSharedPreferenceChanged(defaultSettings, key);
		} else {
			embeddedStateSettings.put(key, value);
			if(sharedPreferenceChangeListener != null)
				sharedPreferenceChangeListener.onSharedPreferenceChanged(null, key);
		}
	}

	@Override
	public void putLong(String key, long value){
		putLong(key,value,false);
	}
	public void putLong(String key, long value, boolean forceDefault) {
		if(debug) Log.d(TAG, (isDefaultSettings ? "DEFAULT" : "LOCAL") + " putLong(key: '" + key + "' value: '" + value + "')");
		if(isDefaultSettings || forceDefault) {
			defaultSettings.edit().putLong(key, value).apply();
			if(sharedPreferenceChangeListener != null)
				sharedPreferenceChangeListener.onSharedPreferenceChanged(defaultSettings, key);
		} else {
			embeddedStateSettings.put(key, value);
			if(sharedPreferenceChangeListener != null)
				sharedPreferenceChangeListener.onSharedPreferenceChanged(null, key);
		}
	}

	@Override
	public void putFloat(String key, float value){
		putFloat(key,value,false);
	}
	public void putFloat(String key, float value, boolean forceDefault) {
		if(debug) Log.d(TAG, (isDefaultSettings ? "DEFAULT" : "LOCAL") + " putFloat(key: '" + key + "' value: '" + value + "')");
		if(isDefaultSettings || forceDefault) {
			defaultSettings.edit().putFloat(key, value).apply();
			if(sharedPreferenceChangeListener != null)
				sharedPreferenceChangeListener.onSharedPreferenceChanged(defaultSettings, key);
		} else {
			embeddedStateSettings.put(key, value);
			if(sharedPreferenceChangeListener != null)
				sharedPreferenceChangeListener.onSharedPreferenceChanged(null, key);
		}
	}

	@Override
	public void putBoolean(String key, boolean value){
		putBoolean(key,value,false);
	}
	public void putBoolean(String key, boolean value, boolean forceDefault) {
		if(debug) Log.d(TAG, (isDefaultSettings ? "DEFAULT" : "LOCAL") + " putBoolean(key: '" + key + "' value: '" + value + "')");
		if(isDefaultSettings || forceDefault) {
			defaultSettings.edit().putBoolean(key, value).apply();
			if(sharedPreferenceChangeListener != null)
				sharedPreferenceChangeListener.onSharedPreferenceChanged(defaultSettings, key);
		} else {
			embeddedStateSettings.put(key, value);
			if(sharedPreferenceChangeListener != null)
				sharedPreferenceChangeListener.onSharedPreferenceChanged(null, key);
		}
	}

	@Nullable
	@Override
	public String getString(String key, @Nullable String defValue) {
		if(debug) Log.d(TAG, "getString(key: '" + key + "')");
		if(!isDefaultSettings) {
			Object result =  embeddedStateSettings.get(key);
			if(result instanceof String)
				return (String) result;
		}
		return defaultSettings.getString(key, defValue);
	}

	@SuppressWarnings("unchecked")
	@Nullable
	@Override
	public Set<String> getStringSet(String key, @Nullable Set<String> defValues) {
		if(debug) Log.d(TAG, "getStringSet(key: '" + key + "')");
		if(!isDefaultSettings) {
			Object result =  embeddedStateSettings.get(key);
			if(result instanceof Set<?>)
				try {
					return (Set<String>) result;
				} catch (Exception ignored) {}
		}
		return defaultSettings.getStringSet(key, defValues);
	}

	@Override
	public int getInt(String key, int defValue) {
		if(debug) Log.d(TAG, "getInt(key: '" + key + "')");
		if(!isDefaultSettings) {
			Object result =  embeddedStateSettings.get(key);
			if(result != null)
				try {
					return (Integer) result;
				} catch (Exception ignored) {}
		}
		return defaultSettings.getInt(key, defValue);
	}

	@Override
	public long getLong(String key, long defValue) {
		if(debug) Log.d(TAG, "getLong(key: '" + key + "')");
		if(!isDefaultSettings) {
			Object result =  embeddedStateSettings.get(key);
			if(result != null)
				try {
					return (Long) result;
				} catch (Exception ignored) {}
		}
		return defaultSettings.getLong(key, defValue);
	}

	@Override
	public float getFloat(String key, float defValue) {
		if(debug) Log.d(TAG, "getFloat(key: '" + key + "')");
		if(!isDefaultSettings) {
			Object result =  embeddedStateSettings.get(key);
			if(result != null)
				try {
					return (Float) result;
				} catch (Exception ignored) {}
		}
		return defaultSettings.getFloat(key, defValue);
	}

	@Override
	public boolean getBoolean(String key, boolean defValue) {
		if(debug) Log.d(TAG, "getBoolean(key: '" + key + "')");
		if(!isDefaultSettings) {
			Object result =  embeddedStateSettings.get(key);
			if(result != null)
				try {
					return (Boolean) result;
				} catch (Exception ignored) {}
		}
		return defaultSettings.getBoolean(key, defValue);
	}


	public void setDefaultSettings(boolean defaultSettings) {
		isDefaultSettings = defaultSettings;
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
		Uri uri = Uri.parse(url);
		try {
			ParcelFileDescriptor pfd = context.getContentResolver().openFileDescriptor(uri, "wa");
			if(pfd != null) {
				FileOutputStream fileOutputStream = new FileOutputStream(pfd.getFileDescriptor());
				String json = toJSON(embeddedStateSettings);
				byte[] jsonBytes = json.getBytes(StandardCharsets.UTF_8);
				int jsonLength = jsonBytes.length;
				fileOutputStream.write(jsonBytes);
				// The JSON text should not be more than 64KB
				fileOutputStream.write((jsonLength >> 8) & 0xFF);
				fileOutputStream.write(jsonLength & 0xFF);
				for (int i = 0; i < magic.length(); i++) {
					fileOutputStream.write(magic.charAt(i));
				}
				fileOutputStream.flush();
				fileOutputStream.close();
			}
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	public void clearEmbeddedStateSettings() {
		embeddedStateSettings.clear();
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
					fileInputStream.read(lastChunk, lastChunkOffset, lastChunk.length);
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
}
