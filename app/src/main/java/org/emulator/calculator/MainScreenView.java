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
import android.app.Activity;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.SparseIntArray;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;

import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class MainScreenView extends PanAndScaleView {

    protected static final String TAG = "MainScreenView";
    protected final boolean debug = false;

    private Paint paintFullCalc = new Paint();
	private Paint paintLCD = new Paint();
    private Bitmap bitmapMainScreen;
	private Rect srcBitmapCopy = new Rect();
    private SparseIntArray vkmap;
    private HashMap<Character, Integer> charmap;
    private List<Integer> numpadKey;
    private int kmlBackgroundColor = Color.BLACK;
    private boolean useKmlBackgroundColor = false;
    private int fallbackBackgroundColorType = 0;
    private int statusBarColor = 0;
    private boolean viewSized = false;
    private int rotationMode = 0;
    private boolean autoRotation = false;
	private boolean autoZoom = false;
	private boolean usePixelBorders = false;

    public float defaultViewScaleFactorX = 1.0f;
    public float defaultViewScaleFactorY = 1.0f;
    public float defaultViewPanOffsetX = 0.0f;
    public float defaultViewPanOffsetY = 0.0f;

//	private Rect invalidateRectangle = new Rect();

    public MainScreenView(Context context) {
        super(context);

        setShowScaleThumbnail(true);
        setAllowDoubleTapZoom(false);

	    paintFullCalc.setFilterBitmap(true);
	    paintFullCalc.setAntiAlias(true);

	    paintLCD.setStyle(Paint.Style.STROKE);
	    paintLCD.setStrokeWidth(1.0f);
	    paintLCD.setAntiAlias(false);

        DisplayMetrics displayMetrics = new DisplayMetrics();
        ((Activity)context).getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        bitmapMainScreen = Bitmap.createBitmap(1, 1, Bitmap.Config.ARGB_8888);
        bitmapMainScreen.eraseColor(Color.BLACK);
        enableZoomKeyboard = false;

        charmap = new HashMap<>();
        charmap.put('+', 0x6B); // VK_ADD
        charmap.put('-', 0x6D); // VK_SUBTRACT
        charmap.put('*', 0x6A); // VK_MULTIPLY
        charmap.put('/', 0x6F); // VK_DIVIDE
        charmap.put('[', 0xDB); // VK_OEM_4
        charmap.put(']', 0xDD); // VK_OEM_6

        vkmap = new SparseIntArray();
        //vkmap.put(KeyEvent.KEYCODE_BACK, 0x08); // VK_BACK
        vkmap.put(KeyEvent.KEYCODE_TAB, 0x09); // VK_TAB
        vkmap.put(KeyEvent.KEYCODE_ENTER, 0x0D); // VK_RETURN
        vkmap.put(KeyEvent.KEYCODE_NUMPAD_ENTER, 0x0D); // VK_RETURN
        vkmap.put(KeyEvent.KEYCODE_DEL, 0x08); // VK_BACK
        vkmap.put(KeyEvent.KEYCODE_FORWARD_DEL, 0x2E); // VK_DELETE
        vkmap.put(KeyEvent.KEYCODE_INSERT, 0x2D); // VK_INSERT
        vkmap.put(KeyEvent.KEYCODE_SHIFT_LEFT, 0x10); // VK_SHIFT
//        vkmap.put(KeyEvent.KEYCODE_SHIFT_RIGHT, 0x10); // VK_SHIFT
        vkmap.put(KeyEvent.KEYCODE_CTRL_LEFT, 0x11); // VK_CONTROL
//        vkmap.put(KeyEvent.KEYCODE_CTRL_RIGHT, 0x11); // VK_CONTROL
        vkmap.put(KeyEvent.KEYCODE_ESCAPE, 0x1B); // VK_ESCAPE
        vkmap.put(KeyEvent.KEYCODE_SPACE, 0x20); // VK_SPACE
	    vkmap.put(KeyEvent.KEYCODE_PAGE_UP, 0x21);  // VK_PRIOR
	    vkmap.put(KeyEvent.KEYCODE_PAGE_DOWN, 0x22);  // VK_NEXT
	    vkmap.put(KeyEvent.KEYCODE_MOVE_END, 0x23);  // VK_END
	    vkmap.put(KeyEvent.KEYCODE_MOVE_HOME, 0x24);  // VK_HOME
	    vkmap.put(KeyEvent.KEYCODE_DPAD_LEFT, 0x25); // VK_LEFT
        vkmap.put(KeyEvent.KEYCODE_DPAD_UP, 0x26); // VK_UP
        vkmap.put(KeyEvent.KEYCODE_DPAD_RIGHT, 0x27); // VK_RIGHT
        vkmap.put(KeyEvent.KEYCODE_DPAD_DOWN, 0x28); // VK_DOWN
        vkmap.put(KeyEvent.KEYCODE_A, 0x41); // A
        vkmap.put(KeyEvent.KEYCODE_B, 0x42); // B
        vkmap.put(KeyEvent.KEYCODE_C, 0x43); // C
        vkmap.put(KeyEvent.KEYCODE_D, 0x44); // D
        vkmap.put(KeyEvent.KEYCODE_E, 0x45); // E
        vkmap.put(KeyEvent.KEYCODE_F, 0x46); // F
        vkmap.put(KeyEvent.KEYCODE_G, 0x47); // G
        vkmap.put(KeyEvent.KEYCODE_H, 0x48); // H
        vkmap.put(KeyEvent.KEYCODE_I, 0x49); // I
        vkmap.put(KeyEvent.KEYCODE_J, 0x4A); // J
        vkmap.put(KeyEvent.KEYCODE_K, 0x4B); // K
        vkmap.put(KeyEvent.KEYCODE_L, 0x4C); // L
        vkmap.put(KeyEvent.KEYCODE_M, 0x4D); // M
        vkmap.put(KeyEvent.KEYCODE_N, 0x4E); // N
        vkmap.put(KeyEvent.KEYCODE_O, 0x4F); // O
        vkmap.put(KeyEvent.KEYCODE_P, 0x50); // P
        vkmap.put(KeyEvent.KEYCODE_Q, 0x51); // Q
        vkmap.put(KeyEvent.KEYCODE_R, 0x52); // R
        vkmap.put(KeyEvent.KEYCODE_S, 0x53); // S
        vkmap.put(KeyEvent.KEYCODE_T, 0x54); // T
        vkmap.put(KeyEvent.KEYCODE_U, 0x55); // U
        vkmap.put(KeyEvent.KEYCODE_V, 0x56); // V
        vkmap.put(KeyEvent.KEYCODE_W, 0x57); // W
        vkmap.put(KeyEvent.KEYCODE_X, 0x58); // X
        vkmap.put(KeyEvent.KEYCODE_Y, 0x59); // Y
        vkmap.put(KeyEvent.KEYCODE_Z, 0x5A); // Z
        vkmap.put(KeyEvent.KEYCODE_0, 0x30); //VK_0   0x60); // VK_NUMPAD0
        vkmap.put(KeyEvent.KEYCODE_1, 0x31); //VK_1   0x61); // VK_NUMPAD1
        vkmap.put(KeyEvent.KEYCODE_2, 0x32); //VK_2   0x62); // VK_NUMPAD2
        vkmap.put(KeyEvent.KEYCODE_3, 0x33); //VK_3   0x63); // VK_NUMPAD3
        vkmap.put(KeyEvent.KEYCODE_4, 0x34); //VK_4   0x64); // VK_NUMPAD4
        vkmap.put(KeyEvent.KEYCODE_5, 0x35); //VK_5   0x65); // VK_NUMPAD5
        vkmap.put(KeyEvent.KEYCODE_6, 0x36); //VK_6   0x66); // VK_NUMPAD6
        vkmap.put(KeyEvent.KEYCODE_7, 0x37); //VK_7   0x67); // VK_NUMPAD7
        vkmap.put(KeyEvent.KEYCODE_8, 0x38); //VK_8   0x68); // VK_NUMPAD8
        vkmap.put(KeyEvent.KEYCODE_9, 0x39); //VK_9   0x69); // VK_NUMPAD9
        vkmap.put(KeyEvent.KEYCODE_NUMPAD_0, 0x60); // VK_NUMPAD0
        vkmap.put(KeyEvent.KEYCODE_NUMPAD_1, 0x61); // VK_NUMPAD1
        vkmap.put(KeyEvent.KEYCODE_NUMPAD_2, 0x62); // VK_NUMPAD2
        vkmap.put(KeyEvent.KEYCODE_NUMPAD_3, 0x63); // VK_NUMPAD3
        vkmap.put(KeyEvent.KEYCODE_NUMPAD_4, 0x64); // VK_NUMPAD4
        vkmap.put(KeyEvent.KEYCODE_NUMPAD_5, 0x65); // VK_NUMPAD5
        vkmap.put(KeyEvent.KEYCODE_NUMPAD_6, 0x66); // VK_NUMPAD6
        vkmap.put(KeyEvent.KEYCODE_NUMPAD_7, 0x67); // VK_NUMPAD7
        vkmap.put(KeyEvent.KEYCODE_NUMPAD_8, 0x68); // VK_NUMPAD8
        vkmap.put(KeyEvent.KEYCODE_NUMPAD_9, 0x69); // VK_NUMPAD9
        vkmap.put(KeyEvent.KEYCODE_NUMPAD_MULTIPLY, 0x6A); // VK_MULTIPLY
        vkmap.put(KeyEvent.KEYCODE_NUMPAD_ADD, 0x6B); // VK_ADD
        vkmap.put(KeyEvent.KEYCODE_EQUALS, 0x6B); // VK_ADD
        vkmap.put(KeyEvent.KEYCODE_NUMPAD_SUBTRACT, 0x6D); // VK_SUBTRACT
        vkmap.put(KeyEvent.KEYCODE_MINUS, 0x6D); // VK_SUBTRACT
        vkmap.put(KeyEvent.KEYCODE_NUMPAD_DOT, 0x6E); // VK_DECIMAL
        vkmap.put(KeyEvent.KEYCODE_SLASH, 0x6F); // VK_DIVIDE
        vkmap.put(KeyEvent.KEYCODE_NUMPAD_DIVIDE, 0x6F); // VK_DIVIDE
        vkmap.put(KeyEvent.KEYCODE_COMMA, 0xBC); // VK_OEM_COMMA
        vkmap.put(KeyEvent.KEYCODE_PERIOD, 0xBE); // VK_OEM_PERIOD
        vkmap.put(KeyEvent.KEYCODE_SEMICOLON, 0xBA); // VK_OEM_1 (:);)
        vkmap.put(KeyEvent.KEYCODE_APOSTROPHE, 0xDE);  // VK_OEM_7 (‘ »)
        vkmap.put(KeyEvent.KEYCODE_BACKSLASH, 0xDC);  // VK_OEM_5 (\|)

	    numpadKey = Arrays.asList(
	    		KeyEvent.KEYCODE_NUMPAD_0,
			    KeyEvent.KEYCODE_NUMPAD_1,
			    KeyEvent.KEYCODE_NUMPAD_2,
			    KeyEvent.KEYCODE_NUMPAD_3,
			    KeyEvent.KEYCODE_NUMPAD_4,
			    KeyEvent.KEYCODE_NUMPAD_5,
			    KeyEvent.KEYCODE_NUMPAD_6,
			    KeyEvent.KEYCODE_NUMPAD_7,
			    KeyEvent.KEYCODE_NUMPAD_8,
			    KeyEvent.KEYCODE_NUMPAD_9,
			    KeyEvent.KEYCODE_NUMPAD_DOT,
			    KeyEvent.KEYCODE_NUMPAD_COMMA
			    );

        this.setFocusable(true);
        this.setFocusableInTouchMode(true);
    }

	private boolean previousRightMouseButtonStateDown = false;

	// Prevent accidental scroll when taping a calc button
    protected Set<Integer> currentButtonTouched = new HashSet<>();
    @SuppressLint("ClickableViewAccessibility")
    public boolean onTouchEvent(MotionEvent event) {
	    if(event.getSource() == InputDevice.SOURCE_MOUSE) {
	    	// Support the right mouse button click effect with Android version >= 5.0
		    if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.LOLLIPOP) {
			    boolean rightMouseButtonStateDown = event.isButtonPressed(MotionEvent.BUTTON_SECONDARY);
			    if(rightMouseButtonStateDown != previousRightMouseButtonStateDown) {
				    // Right button pressed or released.
				    previousRightMouseButtonStateDown = rightMouseButtonStateDown;
				    if(!previousRightMouseButtonStateDown) {
				    	// Allows pressing a calculator button but prevents its release to allow the On+A+F key combination.
				        return true;
				    }
			    }
		    }
	    }

	    int actionIndex = event.getActionIndex();
        int action = event.getActionMasked();
        switch (action) {
            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_POINTER_DOWN:
                currentButtonTouched.remove(actionIndex);
                if(actionIndex == 0 && event.getPointerCount() == 1)
                    currentButtonTouched.clear();
                if (NativeLib.buttonDown((int) ((event.getX(actionIndex) - viewPanOffsetX) / viewScaleFactorX),
                        (int) ((event.getY(actionIndex) - viewPanOffsetY) / viewScaleFactorY))) {
                    currentButtonTouched.add(actionIndex);
                    preventToScroll = true;
                }
                if (debug) Log.d(TAG, "onTouchEvent() ACTION_DOWN false, actionIndex: " + actionIndex
                        + ", currentButtonTouched: " + currentButtonTouched.size()
                        + ", preventToScroll: " + preventToScroll + ", getPointerCount: " + event.getPointerCount());
                break;
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_POINTER_UP:
	            float pointerX = event.getX(actionIndex);
	            float pointerY = event.getY(actionIndex);
	            post(() -> {
		            NativeLib.buttonUp((int) ((pointerX - viewPanOffsetX) / viewScaleFactorX), (int) ((pointerY - viewPanOffsetY) / viewScaleFactorY));
	            });
                currentButtonTouched.remove(actionIndex);
                preventToScroll = currentButtonTouched.size() > 0;
                if (debug) Log.d(TAG, "onTouchEvent() ACTION_UP, actionIndex: " + actionIndex + ", currentButtonTouched: " + currentButtonTouched.size() + ", preventToScroll: " + preventToScroll);
                break;
            case MotionEvent.ACTION_CANCEL:
                currentButtonTouched.remove(actionIndex);
                preventToScroll = currentButtonTouched.size() > 0;
                if (debug) Log.d(TAG, "onTouchEvent() ACTION_CANCEL, actionIndex: " + actionIndex + ", currentButtonTouched: " + currentButtonTouched.size() + ", preventToScroll: " + preventToScroll);
                break;
            default:
                break;
        }
        return super.onTouchEvent(event);
    }

	@Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if((event.getFlags() & KeyEvent.FLAG_VIRTUAL_HARD_KEY) == 0
        && ((event.getSource() & InputDevice.SOURCE_KEYBOARD) == InputDevice.SOURCE_KEYBOARD) || event.getSource() == 0) {
        	if(!event.isNumLockOn() && numpadKey.indexOf(keyCode) != -1)
        		return false;
            char pressedKey = (char) event.getUnicodeChar();
            Integer windowsKeycode = charmap.get(pressedKey);
            if(windowsKeycode == null)
                windowsKeycode = vkmap.get(keyCode);
            if(debug) Log.d(TAG, String.format("onKeyDown keyCode: 0x%x (%d), unicodeChar: '%c' 0x%x (%d) -> windowsKeycode: 0x%x (%d)",
                    keyCode, keyCode, pressedKey, (int)pressedKey, (int)pressedKey, windowsKeycode, windowsKeycode));
            if (windowsKeycode != 0)
                NativeLib.keyDown(windowsKeycode);
            else if(debug) Log.e(TAG, String.format("Unknown keyCode: %d", keyCode));
            if(keyCode == KeyEvent.KEYCODE_ESCAPE /*KEYCODE_BACK*/ || keyCode == KeyEvent.KEYCODE_TAB)
                return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if((event.getFlags() & KeyEvent.FLAG_VIRTUAL_HARD_KEY) == 0
        && ((event.getSource() & InputDevice.SOURCE_KEYBOARD) == InputDevice.SOURCE_KEYBOARD) || event.getSource() == 0) {
	        if(!event.isNumLockOn() && numpadKey.indexOf(keyCode) != -1)
		        return false;
            char pressedKey = (char) event.getUnicodeChar();
            Integer windowsKeycode = charmap.get(pressedKey);
            if(windowsKeycode == null)
                windowsKeycode = vkmap.get(keyCode);
            if(debug) Log.d(TAG, String.format("onKeyUp keyCode: 0x%x (%d), unicodeChar: '%c' 0x%x (%d) -> windowsKeycode: 0x%x (%d)",
                    keyCode, keyCode, pressedKey, (int)pressedKey, (int)pressedKey, windowsKeycode, windowsKeycode));
            if (windowsKeycode != 0)
                NativeLib.keyUp(windowsKeycode);
            else if(debug) Log.e(TAG, String.format("Unknown keyCode: %d", keyCode));
            if(keyCode == KeyEvent.KEYCODE_ESCAPE /*KEYCODE_BACK*/ || keyCode == KeyEvent.KEYCODE_TAB)
                return true;
        }
        return super.onKeyUp(keyCode, event);
    }

    @Override
    protected void onSizeChanged(int viewWidth, int viewHeight, int oldViewWidth, int oldViewHeight) {
        viewSizeWidth = viewWidth;
        viewSizeHeight = viewHeight;

        viewSized = true;
        updateLayout();
    }

    @SuppressLint("SourceLockedOrientationActivity")
    protected void updateLayout() {
        if(bitmapMainScreen != null && virtualSizeHeight > 1) {
            if (virtualSizeWidth > 0.0f && viewSizeWidth > 0.0f) {
                float imageRatio = virtualSizeHeight / virtualSizeWidth;
                float viewRatio = viewSizeHeight / viewSizeWidth;
                if(autoRotation) {
                    if(imageRatio > 1.0f)
                        ((Activity)getContext()).setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
                    else
                        ((Activity)getContext()).setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
                } else {

                    if(rotationMode == 0) // Allow rotation
                        ((Activity)getContext()).setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
                    else if(rotationMode == 1) // Portrait
                        ((Activity)getContext()).setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
                    else if(rotationMode == 2) // Landscape (left or right following the sensor)
                        ((Activity)getContext()).setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);

                    if (autoZoom) {
                        if (imageRatio < 1.0f != viewRatio < 1.0f) {
                            // With have different screens orientations, so we automatically zoom
                            float translateX, translateY, scale;
                            if(viewRatio > imageRatio) {
                                float alpha = viewRatio / imageRatio;
                                scale = Math.min(2, alpha) * viewSizeWidth / virtualSizeWidth;
                                float screenHorizontalPositionRatio = ((float)NativeLib.getScreenPositionX() + (float)NativeLib.getScreenWidth() * 0.5f ) / (virtualSizeWidth + 0.1f);
                                if(screenHorizontalPositionRatio < 0.5f)
                                    // Screen seems to be at the left
                                    translateX = 0;
                                else
                                    // Screen seems to be at the right
                                    translateX = viewSizeWidth - scale * virtualSizeWidth;
                                translateY = (viewSizeHeight - scale * virtualSizeHeight) / 2.0f;
                            } else {
                                float beta = imageRatio / viewRatio;
                                scale = Math.min(2, beta) * viewSizeHeight / virtualSizeHeight;
                                translateX = (viewSizeWidth - scale * virtualSizeWidth) / 2.0f;
                                float screenVerticalPositionRatio = ((float)NativeLib.getScreenPositionY() + (float)NativeLib.getScreenHeight() * 0.5f ) / (virtualSizeHeight + 0.1f);
                                if(screenVerticalPositionRatio < 0.5f)
                                    // Screen seems to be at the top
                                    translateY = 0.0f;
                                else
                                    // Screen seems to be at the bottom
                                    translateY = viewSizeHeight - scale * virtualSizeHeight;
                            }

                            viewScaleFactorX = scale;
                            viewScaleFactorY = scale;
                            scaleFactorMin = scale;
                            scaleFactorMax = maxZoom * scaleFactorMin;
                            viewPanOffsetX = translateX;
                            viewPanOffsetY = translateY;

                            constrainPan();

                            defaultViewScaleFactorX = viewScaleFactorX;
                            defaultViewScaleFactorY = viewScaleFactorY;
                            defaultViewPanOffsetX = viewPanOffsetX;
                            defaultViewPanOffsetY = viewPanOffsetY;

                            if(this.onUpdateLayoutListener != null)
                                this.onUpdateLayoutListener.run();
                            return;
                        }
                    }
                }
            }
            // Else, the screens orientations are the same, so we set the calculator in fullscreen
            resetViewport();

            defaultViewScaleFactorX = viewScaleFactorX;
            defaultViewScaleFactorY = viewScaleFactorY;
            defaultViewPanOffsetX = viewPanOffsetX;
            defaultViewPanOffsetY = viewPanOffsetY;

            if(this.onUpdateLayoutListener != null)
                this.onUpdateLayoutListener.run();
        }
    }

	private Runnable onUpdateLayoutListener = null;

	public void setOnUpdateLayoutListener(Runnable onUpdateLayoutListener) {
		this.onUpdateLayoutListener = onUpdateLayoutListener;
	}

	private Runnable onUpdateDisplayListener = null;

	public void setOnUpdateDisplayListener(Runnable onUpdateDisplayListener) {
		this.onUpdateDisplayListener = onUpdateDisplayListener;
	}

	@Override
	protected void onCustomDraw(Canvas canvas) {
		if (debug) Log.d(TAG, "onCustomDraw()");

		canvas.drawColor(getBackgroundColor());

		// Copy the full calculator with antialiasing
		canvas.drawBitmap(bitmapMainScreen, 0, 0, paintFullCalc);

//		synchronized (invalidateRectangle) {
//			if (invalidateRectangle.isEmpty()) {
//				canvas.drawColor(getBackgroundColor());
//				canvas.drawBitmap(bitmapMainScreen, 0, 0, paintFullCalc);
//			} else {
//				canvas.drawBitmap(bitmapMainScreen, invalidateRectangle, invalidateRectangle, paintFullCalc);
//				invalidateRectangle.setEmpty();
//			}
//		}

		if(usePixelBorders) {
			// Copy the LCD part only without antialiasing
			int x = NativeLib.getScreenPositionX();
			int y = NativeLib.getScreenPositionY();
			srcBitmapCopy.set(x, y, x + NativeLib.getScreenWidth(), y + NativeLib.getScreenHeight());
			canvas.drawBitmap(bitmapMainScreen, srcBitmapCopy, srcBitmapCopy, paintLCD);
		}
	}

    @Override
    public void onDraw(Canvas canvas) {
	    if (debug)
	    	Log.d(TAG, "onDraw()");

        super.onDraw(canvas);

        if(usePixelBorders) {
	        int lcdWidthNative = NativeLib.getScreenWidthNative();
	        if(lcdWidthNative > 0) {
		        int lcdHeightNative = NativeLib.getScreenHeightNative();
		        int lcdPositionX = NativeLib.getScreenPositionX();
		        int lcdPositionY = NativeLib.getScreenPositionY();
		        int lcdWidth = NativeLib.getScreenWidth();
		        int lcdHeight = NativeLib.getScreenHeight();

		        float screenPositionX = lcdPositionX * viewScaleFactorX + viewPanOffsetX;
		        float screenPositionY = lcdPositionY * viewScaleFactorY + viewPanOffsetY;
		        float screenWidth = lcdWidth * viewScaleFactorX;
		        float screenHeight = lcdHeight * viewScaleFactorY;
		        drawPixelBorder(canvas, lcdWidthNative, lcdHeightNative, screenPositionX, screenPositionY, screenWidth, screenHeight, paintLCD);
	        }
        }
    }


	static float[] pointsBuffer = new float[0];
	static void drawPixelBorder(Canvas canvas, int lcdWidthNative, int lcdHeightNative, float screenPositionX, float screenPositionY, float screenWidth, float screenHeight, Paint paintLCD) {
		// Draw the LCD grid lines without antialiasing to emulate the genuine pixels borders
		int lcdBackgroundColor = 0xFF000000 | NativeLib.getLCDBackgroundColor();
		paintLCD.setColor(lcdBackgroundColor);
		float stepX = screenWidth / lcdWidthNative;

		// Optimized drawcalls
		int pointBufferLength = (lcdWidthNative + lcdHeightNative) << 2;
		if(pointsBuffer.length != pointBufferLength)
			pointsBuffer = new float[pointBufferLength]; // Adjust the buffer of points

		int pointsIndex = 0;
		for (int x = 0; x < lcdWidthNative; x++) {
			float screenX = screenPositionX + x * stepX;
			pointsBuffer[pointsIndex++] = screenX;
			pointsBuffer[pointsIndex++] = screenPositionY;
			pointsBuffer[pointsIndex++] = screenX;
			pointsBuffer[pointsIndex++] = screenPositionY + screenHeight;
		}
		float stepY = screenHeight / lcdHeightNative;
		for (int y = 0; y < lcdHeightNative; y++) {
			float screenY = screenPositionY + y * stepY;
			pointsBuffer[pointsIndex++] = screenPositionX;
			pointsBuffer[pointsIndex++] = screenY;
			pointsBuffer[pointsIndex++] = screenPositionX + screenWidth;
			pointsBuffer[pointsIndex++] = screenY;
		}
		canvas.drawLines(pointsBuffer, paintLCD);
	}

	public void updateCallback(int type, int param1, int param2) {
		try {
			switch (type) {
				case NativeLib.CALLBACK_TYPE_INVALIDATE:
					//	            int left = param1 >> 16;
					//	            int top = param1 & 0xFFFF;
					//	            int right = param2 >> 16;
					//	            int bottom = param2 & 0xFFFF;
					//	            if (debug) Log.d(TAG, "updateCallback() CALLBACK_TYPE_INVALIDATE postInvalidate() left: " + left + ", top: " + top + ", right: " + right + ", bottom: " + bottom);
					//	            synchronized (invalidateRectangle) {
					//		            invalidateRectangle.union(left, top, right, bottom);
					//	            }
					if (debug)
						Log.d(TAG, "updateCallback() CALLBACK_TYPE_INVALIDATE postInvalidate()");
					postInvalidate();
					if (this.onUpdateDisplayListener != null)
						this.onUpdateDisplayListener.run();
					break;
				case NativeLib.CALLBACK_TYPE_WINDOW_RESIZE:
					if (debug) Log.d(TAG, "updateCallback() CALLBACK_TYPE_WINDOW_RESIZE()");
					// New Bitmap size
					if (bitmapMainScreen == null || bitmapMainScreen.getWidth() != param1 || bitmapMainScreen.getHeight() != param2) {
						if (debug)
							Log.d(TAG, "updateCallback() Bitmap.createBitmap(x: " + Math.max(1, param1) + ", y: " + Math.max(1, param2) + ")");
						Bitmap oldBitmapMainScreen = bitmapMainScreen;
						bitmapMainScreen = Bitmap.createBitmap(Math.max(1, param1), Math.max(1, param2), Bitmap.Config.ARGB_8888);
						int globalColor = NativeLib.getGlobalColor();
						kmlBackgroundColor = Color.argb(255, (globalColor & 0x00FF0000) >> 16, (globalColor & 0x0000FF00) >> 8, globalColor & 0x000000FF);

						bitmapMainScreen.eraseColor(getBackgroundColor());
						NativeLib.changeBitmap(bitmapMainScreen);

						if (oldBitmapMainScreen != null) {
							oldBitmapMainScreen.recycle();
						}
						firstTime = true;
						setVirtualSize(bitmapMainScreen.getWidth(), bitmapMainScreen.getHeight());
						if (viewSized)
							updateLayout();
					}
					break;
			}
		} catch (Exception ex) {
			if (debug)
				Log.d(TAG, "updateCallback() Exception: " + ex.toString());
		}
    }

    public void setRotationMode(int rotationMode, boolean isDynamic) {
        this.rotationMode = rotationMode;
        if(isDynamic) {
            updateLayout();
            invalidate();
        }
    }

    public void setAutoLayout(int layoutMode, boolean isDynamic) {
        this.autoRotation = layoutMode == 1;
        this.autoZoom = layoutMode == 2;
        this.fillBounds = layoutMode == 3;
        if(isDynamic) {
            updateLayout();
            invalidate();
        }
    }

    public void setBackgroundKmlColor(boolean useKmlBackgroundColor) {
        this.useKmlBackgroundColor = useKmlBackgroundColor;
        postInvalidate();
    }

    public void setBackgroundFallbackColor(int fallbackBackgroundColorType) {
        this.fallbackBackgroundColorType = fallbackBackgroundColorType;
        postInvalidate();
    }

    public void setStatusBarColor(int statusBarColor) {
        this.statusBarColor = statusBarColor;
    }

	public void setUsePixelBorders(boolean usePixelBorders) {
		this.usePixelBorders = usePixelBorders;
		postInvalidate();
	}



    public int getBackgroundColor() {
        if(useKmlBackgroundColor) {
            return kmlBackgroundColor;
        } else switch(fallbackBackgroundColorType) {
            case 0:
                return 0xFF000000;
            case 1:
                return statusBarColor;
        }
        return 0;
    }

    public Bitmap getBitmap() {
        return bitmapMainScreen;
    }
}
