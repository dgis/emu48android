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
import android.app.Dialog;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.webkit.WebView;
import android.webkit.WebViewClient;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatDialogFragment;
import androidx.appcompat.widget.Toolbar;

import java.util.Locale;

public class InfoWebFragment extends AppCompatDialogFragment {

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

	@SuppressLint("SetJavaScriptEnabled")
	@Override
	public View onCreateView(@NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {

		String title = getString(Utils.resId(this, "string", "title_web_fragment_info"));
		Dialog dialog = getDialog();
		if(dialog != null)
			dialog.setTitle(title);

		View view = inflater.inflate(Utils.resId(this, "layout", "fragment_web_info"), container, false);

		// Toolbar

		Toolbar toolbar = view.findViewById(Utils.resId(this, "id", "my_toolbar"));
		toolbar.setTitle(title);
		Utils.colorizeDrawableWithColor(requireContext(), toolbar.getNavigationIcon(), android.R.attr.colorForeground);
		toolbar.setNavigationOnClickListener(v -> dismiss());

		WebView webView = view.findViewById(Utils.resId(this, "id", "webViewInfo"));
		webView.getSettings().setJavaScriptEnabled(true);
		webView.setWebViewClient(new WebViewClient() {
			@Override
			public void onPageFinished(WebView view, String url) {
				super.onPageFinished(view, url);

				if(url != null)
					// Inject a CSS style to wrap the table of content if needed
					view.evaluateJavascript("javascript:(function(){var css=document.createElement(\"style\");css.type=\"text/css\";css.innerHTML=\".nav1{overflow-wrap:break-word;}\";document.head.appendChild(css);})();", null);
			}

			@Override
			public boolean shouldOverrideUrlLoading(WebView view, String url) {
				if(url != null && url.toLowerCase(Locale.ENGLISH).matches("^https?://.*")) {
					// External pages should be loaded in external web browser.
					startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(url)));
					return true;
				}
				return super.shouldOverrideUrlLoading(view, url);
			}
		});
		webView.loadUrl(getString(Utils.resId(this, "string", "help_url")));

		return view;
	}
}
