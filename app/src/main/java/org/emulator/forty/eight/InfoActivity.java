package org.emulator.forty.eight;

import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.text.util.Linkify;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.TextView;

import java.io.IOException;
import java.io.InputStream;

import androidx.appcompat.app.AppCompatActivity;

public class InfoActivity extends AppCompatActivity {

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_info);

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
            TextView textViewInfo = findViewById(R.id.textViewInfo);
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
			finish();
			return true;
		}
		return super.onOptionsItemSelected(item);
	}

}
