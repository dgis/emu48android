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
import android.os.Bundle;
import android.view.MenuItem;
import android.webkit.WebView;
import android.webkit.WebViewClient;

import androidx.appcompat.app.AppCompatActivity;

public class InfoWebActivity extends AppCompatActivity {

	private int homeId;

	@SuppressLint("SetJavaScriptEnabled")
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		setContentView(Utils.resId(this, "layout", "activity_web_info"));
		homeId = Utils.resId(this, "id", "home");

		WebView webView = findViewById(Utils.resId(this, "id", "webViewInfo"));
		webView.getSettings().setJavaScriptEnabled(true);
		webView.setWebViewClient(new WebViewClient() {
			@Override
			public void onPageFinished(WebView view, String url) {
				super.onPageFinished(view, url);

				if(url != null)
					// Inject a CSS style to wrap the table of content if needed
					view.evaluateJavascript("javascript:(function(){var css=document.createElement(\"style\");css.type=\"text/css\";css.innerHTML=\".nav1{overflow-wrap:break-word;}\";document.head.appendChild(css);})();", null);
			}
		});
		webView.loadUrl(getString(Utils.resId(this, "string", "help_url")));
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
