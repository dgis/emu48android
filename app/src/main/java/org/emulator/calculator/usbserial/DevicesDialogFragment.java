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

package org.emulator.calculator.usbserial;

import android.app.Dialog;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatDialogFragment;
import androidx.appcompat.widget.Toolbar;

import org.emulator.calculator.Utils;

public class DevicesDialogFragment extends AppCompatDialogFragment {

	private DevicesFragment devicesFragment;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setStyle(AppCompatDialogFragment.STYLE_NO_FRAME, Utils.resId(this, "style", "AppTheme"));
	}

	@NonNull
	@Override
	public Dialog onCreateDialog(Bundle savedInstanceState) {
		Dialog dialog = super.onCreateDialog(savedInstanceState);
		dialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
		return dialog;
	}

	@Override
	public View onCreateView(@NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {

		String title = getString(Utils.resId(this, "string", "serial_fragment_title"));
		Dialog dialog = getDialog();
		if(dialog != null)
			dialog.setTitle(title);

		View view = inflater.inflate(Utils.resId(this, "layout", "fragment_serial"), container, false);

		// Toolbar

		Toolbar toolbar = view.findViewById(Utils.resId(this, "id", "toolbar"));
		toolbar.setTitle(title);
		Utils.colorizeDrawableWithColor(requireContext(), toolbar.getNavigationIcon(), android.R.attr.colorForeground);
		toolbar.setNavigationOnClickListener(v -> dismiss());

		devicesFragment = new DevicesFragment();
		devicesFragment.setOnSerialDeviceClickedListener((serialConnectParameters) -> {
			if(onSerialDeviceClickedListener != null)
				onSerialDeviceClickedListener.onSerialDeviceClicked(serialConnectParameters);
			dismiss();
		});
		getChildFragmentManager().beginTransaction().add(Utils.resId(this, "id", "serialport_devices_fragment"), devicesFragment, "devices").commit();


        return view;
	}

	private DevicesFragment.OnSerialDeviceClickedListener onSerialDeviceClickedListener;

	/**
	 * Register a callback to be invoked when a serial device has been chosen.
	 * @param onSerialDeviceClickedListener The callback that will run
	 */
	public void setOnSerialDeviceClickedListener(DevicesFragment.OnSerialDeviceClickedListener onSerialDeviceClickedListener) {
		this.onSerialDeviceClickedListener = onSerialDeviceClickedListener;
	}
}
