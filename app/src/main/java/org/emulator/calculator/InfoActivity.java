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

import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.text.util.Linkify;
import android.view.MenuItem;
import android.widget.TextView;

import java.io.IOException;
import java.io.InputStream;

import androidx.appcompat.app.AppCompatActivity;

public class InfoActivity extends AppCompatActivity {

	private int homeId;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		setContentView(Utils.resId(this, "layout", "activity_info"));
		String filepath = getString(Utils.resId(this, "string", "info_readme"));
		homeId = Utils.resId(this, "id", "home");

		// Programmatically load text from an asset and place it into the
        // text view.  Note that the text we are loading is ASCII, so we
        // need to convert it to UTF-16.
        try {
            InputStream is = getAssets().open(filepath);
            
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
			TextView textViewInfo = findViewById(Utils.resId(this, "id", "textViewInfo"));
            textViewInfo.setMovementMethod(new ScrollingMovementMethod());
            textViewInfo.setText(text);
            Linkify.addLinks(textViewInfo, Linkify.ALL);
        } catch (IOException e) {
            // Should never happen!
            //throw new RuntimeException(e);
        }
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		if(item.getItemId() == homeId) {
			finish();
			return true;
		}
		return super.onOptionsItemSelected(item);
	}

}
