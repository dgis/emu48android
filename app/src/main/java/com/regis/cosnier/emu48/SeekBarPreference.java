package com.regis.cosnier.emu48;

import android.content.Context;
import android.content.DialogInterface;
import android.preference.DialogPreference;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;

public class SeekBarPreference extends DialogPreference implements SeekBar.OnSeekBarChangeListener
{
	private static final String androidns="http://schemas.android.com/apk/res/android";
	private static final String appdns="http://schemas.android.com/apk/res-auto";

	private SeekBar mSeekBar;
	private TextView mSplashText,mValueText;
	private Context mContext;

	private String mDialogMessage, mSuffix, mSuffixes;
	private int mDefault, mMin, mMax, mValue = 0;
	private float mDisplayScale = 1.0f;
	private String mDisplayFormat;

	public SeekBarPreference(Context context, AttributeSet attrs) { 
		super(context,attrs);
		mContext = context;

		mDialogMessage = getAttributeValue(attrs, androidns, "dialogMessage");
		mValue = mDefault = attrs.getAttributeIntValue(androidns, "defaultValue", 0);
		mDisplayScale = attrs.getAttributeFloatValue(appdns, "displayScale", 1.0f);
		mSuffix = getAttributeValue(attrs, appdns, "suffix");
		mSuffixes = getAttributeValue(attrs, appdns, "suffixes");
		mDisplayFormat = attrs.getAttributeValue(appdns, "displayFormat");
		mMin = attrs.getAttributeIntValue(appdns, "min", 0);
		mMax = attrs.getAttributeIntValue(androidns, "max", 100);
	}
	
	private String getAttributeValue(AttributeSet attrs, String namespace, String name) {
		String attributeValue = attrs.getAttributeValue(namespace, name);
		String result = attributeValue;
		if(attributeValue != null && attributeValue.length() > 1 && attributeValue.charAt(0) == '@') {
			try {
				int id = Integer.parseInt(attributeValue.substring(1));
				result = mContext.getString(id);
			} catch (NumberFormatException e) {}
		}
		return result;
	}
	
	@Override 
	protected View onCreateDialogView() {
		LinearLayout layout = new LinearLayout(mContext);
		layout.setOrientation(LinearLayout.VERTICAL);
		layout.setPadding(6, 6, 6, 6);

		mSplashText = new TextView(mContext);
		if (mDialogMessage != null)
			mSplashText.setText(mDialogMessage);
		layout.addView(mSplashText);

		mValueText = new TextView(mContext);
		mValueText.setGravity(Gravity.CENTER_HORIZONTAL);
		mValueText.setTextSize(32);
		layout.addView(mValueText, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));

		mSeekBar = new SeekBar(mContext);
		mSeekBar.setOnSeekBarChangeListener(this);
		layout.addView(mSeekBar, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));

		if (shouldPersist())
			mValue = getPersistedInt(mDefault);

		mSeekBar.setMax(mMax - mMin);
		mSeekBar.setProgress(mValue - mMin);
		return layout;
	}
	@Override 
	protected void onBindDialogView(View v) {
		super.onBindDialogView(v);
		mSeekBar.setMax(mMax - mMin);
		mSeekBar.setProgress(mValue - mMin);
	}
	@Override
	protected void onSetInitialValue(boolean restore, Object defaultValue)  
	{
		super.onSetInitialValue(restore, defaultValue);
		if (restore) 
			mValue = shouldPersist() ? getPersistedInt(mDefault) : 0;
		else 
			mValue = (Integer)defaultValue;
	}

	public void setValueSummary()
	{
		setSummary(getValueSummary());
	}

	public String getValueSummary()
	{
		return getValueSummary(mValue);
	}

	public String getValueSummary(int value)
	{
		String textValue;
		if(mDisplayFormat != null)
			textValue = String.format(mDisplayFormat, value * mDisplayScale);
		else if(mDisplayScale != 1.0f)
			textValue = String.valueOf(value * mDisplayScale);
		else
			textValue = String.valueOf(value);

		if(value * mDisplayScale > 1.0f) {
			if(mSuffixes != null)
				textValue += " " + mSuffixes;
			else if(mSuffix != null)
				textValue += " " + mSuffix;
		} else if(mSuffix != null)
			textValue += " " + mSuffix;
		
		return textValue;
	}
	
	public void onProgressChanged(SeekBar seek, int value, boolean fromTouch)
	{
		int theValue = mMin + value;
		String textValue = getValueSummary(theValue);
		mValueText.setText(textValue);
		
		if (shouldPersist())
			persistInt(theValue);
		
		//mValue = theValue;
		//callChangeListener(Integer.valueOf(mMin + value));
	}
	public void onStartTrackingTouch(SeekBar seek) {}
	public void onStopTrackingTouch(SeekBar seek) {}

	@Override
	public void onClick(DialogInterface dialog, int which) {
	    switch (which) {
	    case DialogInterface.BUTTON_POSITIVE:
	        // save your new password here
	    	mValue = mMin + mSeekBar.getProgress(); 
			callChangeListener(Integer.valueOf(mValue));
	        break;
	    default:
	        // do something else...
	        break;
	    }
	}
	
	public void setMin(int min) { mMin = min; }
	public int getMin() { return mMin; }
	public void setMax(int max) { mMax = max; }
	public int getMax() { return mMax; }

	public void setProgress(int progress) { 
		mValue = progress;
		if (mSeekBar != null)
			mSeekBar.setProgress(progress - mMin); 
	}
	public int getProgress() { return mValue; }
}
