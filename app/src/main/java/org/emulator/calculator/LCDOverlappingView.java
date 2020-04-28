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
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.widget.FrameLayout;

import static org.emulator.calculator.MainScreenView.drawPixelBorder;

@SuppressLint("ViewConstructor")
public class LCDOverlappingView extends View {

    protected static final String TAG = "LCDOverlappingView";
    protected final boolean debug = false;

    private Settings settings;
    private Paint paint = new Paint();
    private Rect srcBitmapCopy = new Rect();
    private Rect dstBitmapCopy = new Rect();
    private float bitmapRatio = -1;
    private float minViewSize = 200.0f;
    public static int OVERLAPPING_LCD_MODE_NONE = 0;
    public static int OVERLAPPING_LCD_MODE_AUTO = 1;
    public static int OVERLAPPING_LCD_MODE_MANUAL = 2;
    private int overlappingLCDMode = OVERLAPPING_LCD_MODE_AUTO;
    private MainScreenView mainScreenView;
    private boolean firstTime = true;
	private boolean usePixelBorders = true;

    public LCDOverlappingView(Context context, MainScreenView mainScreenView) {
        super(context);

        this.mainScreenView = mainScreenView;
	    this.mainScreenView.setOnUpdateLayoutListener(this::updateLayout);
	    this.mainScreenView.setOnUpdateDisplayListener(this::postInvalidate);

	    settings = EmuApplication.getSettings();

	    paint.setStyle(Paint.Style.STROKE);
	    paint.setStrokeWidth(1.0f);
        paint.setAntiAlias(false);

        DisplayMetrics displayMetrics = new DisplayMetrics();
        ((Activity)context).getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);

        this.setFocusable(true);
        this.setFocusableInTouchMode(true);
    }

    private float previousX0 = -1.0f, previousY0 = -1.0f, previousX1 = -1.0f, previousY1 = -1.0f;
    private float previousDownX0 = 0, previousDownY0 = 0;

    private float distanceBetweenTwoPoints(float fromPointX, float fromPointY, float toPointX, float toPointY) {
        float x = toPointX - fromPointX;
        float y = toPointY - fromPointY;
        return (float)Math.sqrt(x * x + y * y);
    }

    private MotionEvent currentDownEvent;
    private MotionEvent previousUpEvent;

    private boolean isDoubleTap(MotionEvent firstDown, MotionEvent firstUp, MotionEvent secondDown) {
        long deltaTime = secondDown.getEventTime() - firstUp.getEventTime();
        if (deltaTime > 300 || deltaTime < 40)
            return false;

        int deltaX = (int)firstDown.getX() - (int)secondDown.getX();
        int deltaY = (int)firstDown.getY() - (int)secondDown.getY();
        return deltaX * deltaX + deltaY * deltaY < 100 * 100;
    }

    @SuppressLint("ClickableViewAccessibility")
    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if(debug) Log.d(TAG, "onTouchEvent() getAction(): " + event.getAction() + ", getPointerCount(): " + event.getPointerCount());

        int touchCount = event.getPointerCount();
        float currentX0 = 0f, currentY0 = 0f, currentX1 = 0f, currentY1 = 0f;


        if(touchCount > 0) {
            currentX0 = event.getX(0);
            currentY0 = event.getY(0);
        }
        if(touchCount > 1) {
            currentX1 = event.getX(1);
            currentY1 = event.getY(1);
        }
        if(debug) {
            Log.d(TAG, "onTouchEvent() currentX0: " + currentX0 + ", currentY0: " + currentY0);
            Log.d(TAG, "onTouchEvent() currentX1: " + currentX1 + ", currentY1: " + currentY1);
            Log.d(TAG, "onTouchEvent() touchCount: " + touchCount);
        }

        int action = event.getAction();
        switch (action) {
            case MotionEvent.ACTION_DOWN:
                if(debug) Log.d(TAG, "onTouchEvent() touchesBegan count: " + touchCount);

                previousDownX0 = currentX0;
                previousDownY0 = currentY0;

                if(currentDownEvent != null && previousUpEvent != null && isDoubleTap(currentDownEvent, previousUpEvent, event)) {
                    // This is a double tap
                    if(debug) Log.d(TAG, "onTouchEvent() Double tap");
                    if(overlappingLCDMode != OVERLAPPING_LCD_MODE_AUTO)
                        post(() -> {
                            setOverlappingLCDMode(OVERLAPPING_LCD_MODE_AUTO);
                            changeOverlappingLCDModeToAuto();
                        });
                }
                if (currentDownEvent != null)
                    currentDownEvent.recycle();
                currentDownEvent = MotionEvent.obtain(event);

                break;
            case MotionEvent.ACTION_MOVE:
                //if(debug) Log.d(TAG, "touchesMoved count: " + touchCount);

                if (touchCount == 1) {
                    FrameLayout.LayoutParams viewFlowLayout = (FrameLayout.LayoutParams)getLayoutParams();
                    viewFlowLayout.leftMargin += currentX0 - previousDownX0;
                    viewFlowLayout.topMargin += currentY0 - previousDownY0;
                    setLayoutParams(viewFlowLayout);
                    changeOverlappingLCDModeToManual();
                } else if (touchCount == 2) {
                    if(previousX0 != -1.0f) {
                        FrameLayout.LayoutParams viewFlowLayout = (FrameLayout.LayoutParams)getLayoutParams();
                        float scaleFactor = distanceBetweenTwoPoints(currentX0, currentY0, currentX1, currentY1) / distanceBetweenTwoPoints(previousX0, previousY0, previousX1, previousY1);
                        float scaledViewWidth = (float)viewFlowLayout.width * scaleFactor;
                        float scaledViewHeight = (float)viewFlowLayout.height * scaleFactor;

                        float deltaFactor = (scaleFactor - 1.0f) / 2.0f;
                        int deltaW = (int) ((float) viewFlowLayout.width * deltaFactor);
                        int deltaH = (int) ((float) viewFlowLayout.height * deltaFactor);

                        if (debug)
                            Log.d(TAG, "onTouchEvent() scaleFactor: " + scaleFactor + ", currentWidth: " + viewFlowLayout.width + ", currentHeight: " + viewFlowLayout.height
                                    + ", deltaW: " + deltaW + ", deltaH: " + deltaH);
                        if (debug)
                            Log.d(TAG, "onTouchEvent() BEFORE leftMargin: " + viewFlowLayout.leftMargin + ", topMargin: " + viewFlowLayout.topMargin
                                    + ", width: " + viewFlowLayout.width + ", height: " + viewFlowLayout.height);

                        viewFlowLayout.leftMargin -= deltaW;
                        viewFlowLayout.topMargin -= deltaH;
                        int newViewWidth, newViewHeight;
                        if(bitmapRatio > 0.0f) {
                            if(bitmapRatio < 1.0f) {
                                newViewWidth = (int) scaledViewWidth;
                                newViewHeight = (int) (newViewWidth * bitmapRatio);
                            } else {
                                newViewHeight = (int) scaledViewHeight;
                                newViewWidth = (int) (newViewHeight / bitmapRatio);
                            }
                        } else {
                            newViewWidth = (int) scaledViewWidth;
                            newViewHeight = (int) scaledViewHeight;
                        }

                        if(newViewWidth >= minViewSize && newViewWidth >= minViewSize) {
                            viewFlowLayout.width = newViewWidth;
                            viewFlowLayout.height = newViewHeight;
                        }

                        if (debug)
                            Log.d(TAG, "onTouchEvent() AFTER leftMargin: " + viewFlowLayout.leftMargin + ", topMargin: " + viewFlowLayout.topMargin
                                    + ", width: " + viewFlowLayout.width + ", height: " + viewFlowLayout.height);

                        setLayoutParams(viewFlowLayout);
                        changeOverlappingLCDModeToManual();
                    }
                    previousX0 = currentX0;
                    previousY0 = currentY0;
                    previousX1 = currentX1;
                    previousY1 = currentY1;
                }

                break;
            case MotionEvent.ACTION_UP:
                previousX0 = -1.0f;
                previousY0 = -1.0f;
                previousX1 = -1.0f;
                previousY1 = -1.0f;

                if (previousUpEvent != null)
                    previousUpEvent.recycle();
                previousUpEvent = MotionEvent.obtain(event);
                break;
            case MotionEvent.ACTION_CANCEL:
            case MotionEvent.ACTION_OUTSIDE:
                break;
            default:
        }
        return true; // processed
    }

    @Override
    protected void onDraw(Canvas canvas) {
        if(debug)
        	Log.d(TAG, "onDraw()");

        if(this.overlappingLCDMode != OVERLAPPING_LCD_MODE_NONE) {
            int lcdPositionX = NativeLib.getScreenPositionX();
            int lcdPositionY = NativeLib.getScreenPositionY();
            srcBitmapCopy.set(lcdPositionX, lcdPositionY, lcdPositionX + NativeLib.getScreenWidth(), lcdPositionY + NativeLib.getScreenHeight());
            dstBitmapCopy.set(0, 0, getWidth(), getHeight());
            canvas.drawBitmap(mainScreenView.getBitmap(), srcBitmapCopy, dstBitmapCopy, paint);

	        if(usePixelBorders) {
		        int lcdWidthNative = NativeLib.getScreenWidthNative();
		        if(lcdWidthNative > 0) {
			        int lcdHeightNative = NativeLib.getScreenHeightNative();
			        drawPixelBorder(canvas, lcdWidthNative, lcdHeightNative, 0.0f, 0.0f, getWidth(), getHeight(), paint);
		        }
	        }
        }
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);

        if(debug) Log.d(TAG, "onSizeChanged() width: " + w + ", height: " + h);

        updateLayout();
    }

    public void updateLayout() {
        if(debug) Log.d(TAG, "updateLayout()");
        if(this.overlappingLCDMode != OVERLAPPING_LCD_MODE_NONE) {
            int lcdWidth = Math.max(1, NativeLib.getScreenWidth());
            int lcdHeight = Math.max(1, NativeLib.getScreenHeight());

            if(this.overlappingLCDMode == OVERLAPPING_LCD_MODE_AUTO) {
                post(() -> {
                    int viewLCDWidth = (int) (lcdWidth * this.mainScreenView.defaultViewScaleFactorX);
                    int viewLCDHeight = (int) (lcdHeight * this.mainScreenView.defaultViewScaleFactorY);
                    FrameLayout.LayoutParams viewFlowLayout = new FrameLayout.LayoutParams(viewLCDWidth, viewLCDHeight);
                    viewFlowLayout.leftMargin = (int)(this.mainScreenView.defaultViewScaleFactorX * (float)NativeLib.getScreenPositionX() + this.mainScreenView.defaultViewPanOffsetX);
                    viewFlowLayout.topMargin = (int)(this.mainScreenView.defaultViewScaleFactorY * (float)NativeLib.getScreenPositionY() + this.mainScreenView.defaultViewPanOffsetY);

                    int tolerance = 80; // In case of miscalculation
                    if (viewFlowLayout.leftMargin + viewLCDWidth < tolerance)
                        viewFlowLayout.leftMargin = 0;
                    if (viewFlowLayout.leftMargin + tolerance > this.mainScreenView.getWidth())
                        viewFlowLayout.leftMargin = this.mainScreenView.getWidth() - viewLCDWidth;
                    if (viewFlowLayout.topMargin + viewLCDHeight < tolerance)
                        viewFlowLayout.topMargin = 0;
                    if (viewFlowLayout.topMargin + tolerance > this.mainScreenView.getHeight())
                        viewFlowLayout.topMargin = this.mainScreenView.getHeight() - viewLCDHeight;
                    if(debug) Log.d(TAG, "updateLayout() leftMargin: " + viewFlowLayout.leftMargin + ", topMargin: " + viewFlowLayout.topMargin
                            + ", width: " + viewFlowLayout.width + ", height: " + viewFlowLayout.height);
                    setLayoutParams(viewFlowLayout);
                    setVisibility(View.VISIBLE);
                });
            } else if(this.overlappingLCDMode == OVERLAPPING_LCD_MODE_MANUAL) {
                if(firstTime) {
                    firstTime = false;
                    post(() -> {
                        float scale = settings.getFloat("settings_lcd_overlapping_scale", 1.0f);
                        if (scale < 0.5f)
                            scale = 0.5f;
                        if (scale > 20.0f)
                            scale = 20.0f;
                        int viewWidth = (int) (lcdWidth * scale);
                        int viewHeight = (int) (lcdHeight * scale);
                        if (viewWidth < minViewSize && viewHeight < minViewSize) {
                            if (bitmapRatio > 0.0f) {
                                if (bitmapRatio < 1.0f) {
                                    viewWidth = (int) minViewSize;
                                    viewHeight = (int) (viewWidth * bitmapRatio);
                                } else {
                                    viewHeight = (int) minViewSize;
                                    viewWidth = (int) (viewHeight / bitmapRatio);
                                }
                            } else {
                                viewWidth = (int) minViewSize;
                                viewHeight = (int) minViewSize;
                            }
                        }

                        FrameLayout.LayoutParams viewFlowLayout = new FrameLayout.LayoutParams(viewWidth, viewHeight);
                        viewFlowLayout.leftMargin = settings.getInt("settings_lcd_overlapping_x", 20);
                        viewFlowLayout.topMargin = settings.getInt("settings_lcd_overlapping_y", 80);
                        int tolerance = 80;
                        if (viewFlowLayout.leftMargin + viewWidth < tolerance)
                            viewFlowLayout.leftMargin = tolerance - viewWidth;
                        if (viewFlowLayout.leftMargin + tolerance > this.mainScreenView.getWidth())
                            viewFlowLayout.leftMargin = this.mainScreenView.getWidth() - tolerance;
                        if (viewFlowLayout.topMargin + viewHeight < tolerance)
                            viewFlowLayout.topMargin = tolerance - viewHeight;
                        if (viewFlowLayout.topMargin + tolerance > this.mainScreenView.getHeight())
                            viewFlowLayout.topMargin = this.mainScreenView.getHeight() - tolerance;
                        if(debug) Log.d(TAG, "updateLayout() leftMargin: " + viewFlowLayout.leftMargin + ", topMargin: " + viewFlowLayout.topMargin
                                + ", width: " + viewFlowLayout.width + ", height: " + viewFlowLayout.height);
                        setLayoutParams(viewFlowLayout);
                        setVisibility(View.VISIBLE);
                    });
                }
            }
        }
    }

    public void saveViewLayout() {
        if(this.overlappingLCDMode == OVERLAPPING_LCD_MODE_NONE)
            return;
        FrameLayout.LayoutParams viewFlowLayout = (FrameLayout.LayoutParams)getLayoutParams();
        settings.putString("settings_lcd_overlapping_mode", Integer.toString(this.overlappingLCDMode));
        settings.putInt("settings_lcd_overlapping_x", viewFlowLayout.leftMargin);
        settings.putInt("settings_lcd_overlapping_y", viewFlowLayout.topMargin);
        settings.putFloat("settings_lcd_overlapping_scale", (float)viewFlowLayout.width / (float)Math.max(1, NativeLib.getScreenWidth()));
    }

    private void changeOverlappingLCDModeToManual() {
        if(this.overlappingLCDMode == OVERLAPPING_LCD_MODE_AUTO) { // Mode Auto
            this.overlappingLCDMode = OVERLAPPING_LCD_MODE_MANUAL; // We change the mode to Manual
	        settings.putString("settings_lcd_overlapping_mode", Integer.toString(this.overlappingLCDMode));
            Context context = getContext();
            if(context != null)
                Utils.showAlert(context, context.getString(Utils.resId(context, "string", "message_change_overlapping_lcd_mode_to_manual")));
        }
    }

    private void changeOverlappingLCDModeToAuto() {
        this.overlappingLCDMode = OVERLAPPING_LCD_MODE_AUTO; // We change the mode to Auto
	    settings.putString("settings_lcd_overlapping_mode", Integer.toString(this.overlappingLCDMode));
        Context context = getContext();
        if(context != null)
            Utils.showAlert(context, context.getString(Utils.resId(context, "string", "message_change_overlapping_lcd_mode_to_auto")));
    }

    public void setOverlappingLCDMode(int overlappingLCDMode) {
        if(debug) Log.d(TAG, "setOverlappingLCDMode(" + overlappingLCDMode + ")");
        int previousOverlappingLCDMode = this.overlappingLCDMode;
        this.overlappingLCDMode = overlappingLCDMode;
        if(previousOverlappingLCDMode == OVERLAPPING_LCD_MODE_NONE) {
            if(overlappingLCDMode == OVERLAPPING_LCD_MODE_AUTO)
                this.updateLayout();
            else if(overlappingLCDMode == OVERLAPPING_LCD_MODE_MANUAL)
                setVisibility(VISIBLE);
        } else if(previousOverlappingLCDMode == OVERLAPPING_LCD_MODE_AUTO) {
            if(overlappingLCDMode == OVERLAPPING_LCD_MODE_NONE)
                setVisibility(GONE);
        } else if(previousOverlappingLCDMode == OVERLAPPING_LCD_MODE_MANUAL) {
            if(overlappingLCDMode == OVERLAPPING_LCD_MODE_NONE)
                setVisibility(GONE);
            else if(overlappingLCDMode == OVERLAPPING_LCD_MODE_AUTO)
                this.updateLayout();
        }
    }

	public void setUsePixelBorders(boolean usePixelBorders) {
		this.usePixelBorders = usePixelBorders;
		postInvalidate();
	}
}
