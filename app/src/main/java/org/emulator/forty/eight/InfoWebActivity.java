package org.emulator.forty.eight;

import android.app.Activity;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.webkit.WebView;

import org.emulator.forty.eight.R;

public class InfoWebActivity extends Activity {

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_web_info);
		// Show the Up button in the action bar.
		//getActionBar().setDisplayHomeAsUpEnabled(true);

		WebView webView = (WebView)findViewById(R.id.webViewInfo);
//		webView.setScrollBarStyle(View.SCROLLBARS_INSIDE_OVERLAY);
//		webView.setBackgroundColor(Color.LTGRAY);
		String url = getString(R.string.help_url);
		webView.loadUrl(url);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		//getMenuInflater().inflate(R.menu.activity_info, menu);
		//return true;
		return false;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId()) {
			case android.R.id.home:
				// This ID represents the Home or Up button. In the case of this
				// activity, the Up button is shown. Use NavUtils to allow users
				// to navigate up one level in the application structure. For
				// more details, see the Navigation pattern on Android Design:
				//
				// http://developer.android.com/design/patterns/navigation.html#up-vs-back
				//
				//NavUtils.navigateUpFromSameTask(this);
				finish();
				return true;
		}
		return super.onOptionsItemSelected(item);
	}

}
