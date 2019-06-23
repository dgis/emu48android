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
import android.view.MenuItem;
import android.webkit.WebView;

import androidx.appcompat.app.AppCompatActivity;

public class InfoWebActivity extends AppCompatActivity {

	private int homeId;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		setContentView(Utils.resId(this, "layout", "activity_web_info"));
		homeId = Utils.resId(this, "id", "home");
		WebView webView = findViewById(Utils.resId(this, "id", "webViewInfo"));
		String url = getString(Utils.resId(this, "string", "help_url"));
		webView.loadUrl(url);
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
