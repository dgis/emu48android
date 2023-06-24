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

import android.app.Dialog;
import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.text.util.Linkify;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatDialogFragment;
import androidx.appcompat.widget.Toolbar;

import java.io.IOException;
import java.io.InputStream;

public class InfoFragment extends AppCompatDialogFragment {

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

		String title = getString(Utils.resId(this, "string", "title_fragment_info"));
		Dialog dialog = getDialog();
		if(dialog != null)
			dialog.setTitle(title);

		View view = inflater.inflate(Utils.resId(this, "layout", "fragment_info"), container, false);

		// Toolbar

		Toolbar toolbar = view.findViewById(Utils.resId(this, "id", "my_toolbar"));
		toolbar.setTitle(title);
		Utils.colorizeDrawableWithColor(requireContext(), toolbar.getNavigationIcon(), android.R.attr.colorForeground);
		toolbar.setNavigationOnClickListener(v -> dismiss());

		// Programmatically load text from an asset and place it into the
        // text view.  Note that the text we are loading is ASCII, so we
        // need to convert it to UTF-16.
        try {
            InputStream is = requireContext().getAssets().open(getString(Utils.resId(this, "string", "info_readme")));
            
            // We guarantee that the available method returns the total
            // size of the asset...  of course, this does mean that a single
            // asset can't be more than 2 gigs.
            int size = is.available();
            
            // Read the entire asset into a local byte buffer.
            byte[] buffer = new byte[size];
			//noinspection ResultOfMethodCallIgnored
			is.read(buffer);
            is.close();
            
            // Convert the buffer into a string.
            String text = new String(buffer);
            
            // Finally stick the string into the text view.
	        TextView textViewInfo = view.findViewById(Utils.resId(this, "id", "textViewInfo"));
            textViewInfo.setMovementMethod(new ScrollingMovementMethod());
            textViewInfo.setText(text);
            Linkify.addLinks(textViewInfo, Linkify.WEB_URLS);
        } catch (IOException ignored) { }
        return view;
	}
}
