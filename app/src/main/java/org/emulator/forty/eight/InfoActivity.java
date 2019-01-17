package org.emulator.forty.eight;

import android.app.ActionBar;
import android.app.Activity;
import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.text.util.Linkify;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.TextView;

import org.emulator.forty.eight.R;

import java.io.IOException;
import java.io.InputStream;

public class InfoActivity extends Activity {

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_info);

		// Show the Up button in the action bar.
		ActionBar actionBar = getActionBar();
		if(actionBar != null) {
			actionBar.setDisplayShowHomeEnabled(true);
			actionBar.setDisplayShowTitleEnabled(true);
			actionBar.setDisplayHomeAsUpEnabled(true);
		}
		
		String filepath = getString(R.string.info_readme);

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
            TextView textViewInfo = (TextView)findViewById(R.id.textViewInfo);
            textViewInfo.setMovementMethod(new ScrollingMovementMethod());
            textViewInfo.setText(text);
            Linkify.addLinks(textViewInfo, Linkify.ALL);
        } catch (IOException e) {
            // Should never happen!
            //throw new RuntimeException(e);
        }
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		//getMenuInflater().inflate(R.menu.activity_info, menu);
		return false; //true;
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
