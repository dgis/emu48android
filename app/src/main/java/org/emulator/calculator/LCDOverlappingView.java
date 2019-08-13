package org.emulator.calculator;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.preference.PreferenceManager;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.widget.FrameLayout;

public class LCDOverlappingView extends View {

    protected static final String TAG = "LCDOverlappingView";
    protected final boolean debug = false;

    private SharedPreferences sharedPreferences;
    private Paint paint = new Paint();
    private Rect srcBitmapCopy = new Rect();
    private Rect dstBitmapCopy = new Rect();
    private Bitmap bitmapLCD;
    private float bitmapRatio = -1;
    private float minViewSize = 200.0f;
    private int overlappingLCDMode = 1;
    private MainScreenView mainScreenView;

    public LCDOverlappingView(Context context, MainScreenView mainScreenView) {
        super(context);

        this.mainScreenView = mainScreenView;
        this.mainScreenView.setOnUpdateLayoutListener(() -> this.updateLayout(this.mainScreenView.viewPanOffsetX, this.mainScreenView.viewPanOffsetY, this.mainScreenView.viewScaleFactorX, this.mainScreenView.viewScaleFactorY));

        sharedPreferences = PreferenceManager.getDefaultSharedPreferences(context.getApplicationContext());

        //paint.setFilterBitmap(true);
        //paint.setAntiAlias(true);

        DisplayMetrics displayMetrics = new DisplayMetrics();
        ((Activity)context).getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        bitmapLCD = Bitmap.createBitmap(1, 1, Bitmap.Config.ARGB_8888);
        bitmapLCD.eraseColor(Color.BLACK);

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

                break;
            case MotionEvent.ACTION_MOVE:
                //if(debug) Log.d(TAG, "touchesMoved count: " + touchCount);

                if (touchCount == 1) {
                    FrameLayout.LayoutParams viewFlowLayout = (FrameLayout.LayoutParams)getLayoutParams();
                    viewFlowLayout.leftMargin += currentX0 - previousDownX0;
                    viewFlowLayout.topMargin += currentY0 - previousDownY0;
                    setLayoutParams(viewFlowLayout);
                    changeOverlappingLCDMode();
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
                        changeOverlappingLCDMode();
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
                break;
            case MotionEvent.ACTION_CANCEL:
                break;
            case MotionEvent.ACTION_OUTSIDE:
                break;
            default:
        }
        return true; // processed
    }

    @Override
    protected void onDraw(Canvas canvas) {
        //if(debug) Log.d(TAG, "onDraw()");

        canvas.drawColor(Color.RED); //TODO to remove

        if(this.overlappingLCDMode > 0 && bitmapLCD != null) {
            int x = NativeLib.getScreenPositionX();
            int y = NativeLib.getScreenPositionY();
            srcBitmapCopy.set(x, y, x + NativeLib.getScreenWidth(), y + NativeLib.getScreenHeight());
            dstBitmapCopy.set(0, 0, getWidth(), getHeight());
            canvas.drawBitmap(mainScreenView.getBitmap(), srcBitmapCopy, dstBitmapCopy, paint);
        }
    }

    public int updateCallback(int type, int param1, int param2, String param3, String param4) {
        if(this.overlappingLCDMode == 0)
            return -1;
        switch (type) {
            case NativeLib.CALLBACK_TYPE_INVALIDATE:
                //if(debug) Log.d(TAG, "PAINT updateCallback() postInvalidate()");
                if(debug) Log.d(TAG, "updateCallback() CALLBACK_TYPE_INVALIDATE");
                if(bitmapLCD.getWidth() > 1)
                    postInvalidate();
                break;
            case NativeLib.CALLBACK_TYPE_WINDOW_RESIZE:
                // New Bitmap size
                int newLCDWidth = NativeLib.getScreenWidth();
                int newLCDHeight = NativeLib.getScreenHeight();
                if(bitmapLCD == null || bitmapLCD.getWidth() != newLCDWidth || bitmapLCD.getHeight() != newLCDHeight) {
                    int newWidth = Math.max(1, newLCDWidth);
                    int newHeight = Math.max(1, newLCDHeight);

                    if(debug) Log.d(TAG, "updateCallback() Bitmap.createBitmap(x: " + newWidth + ", y: " + newHeight + ")");
                    Bitmap  oldBitmapLCD = bitmapLCD;
                    bitmapLCD = Bitmap.createBitmap(newWidth, newHeight, Bitmap.Config.ARGB_8888);
                    bitmapRatio = (float)newHeight / (float)newWidth;
                    if(oldBitmapLCD != null)
                        oldBitmapLCD.recycle();

                    if(this.overlappingLCDMode != 1) {
                        setVisibility(View.VISIBLE);
                        float scale = sharedPreferences.getFloat("settings_lcd_overlapping_scale", 1.0f);
                        if (scale < 0.01f)
                            scale = 0.01f;
                        int viewWidth = (int) (newWidth * scale);
                        int viewHeight = (int) (newHeight * scale);
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
                        viewFlowLayout.leftMargin = sharedPreferences.getInt("settings_lcd_overlapping_x", 20);
                        viewFlowLayout.topMargin = sharedPreferences.getInt("settings_lcd_overlapping_y", 80);
                        if (viewFlowLayout.leftMargin + viewWidth < 0)
                            viewFlowLayout.leftMargin = 0;
                        if (viewFlowLayout.topMargin + viewHeight < 0)
                            viewFlowLayout.topMargin = 0;
                        setLayoutParams(viewFlowLayout);
                    }
                }
                break;
        }
        return -1;
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);

        if(debug) Log.d(TAG, "onSizeChanged() width: " + w + ", height: " + h);
    }

    public void updateLayout(float viewPanOffsetX, float viewPanOffsetY, float viewScaleFactorX, float viewScaleFactorY) {
        if(debug) Log.d(TAG, "updateLayout()");
        if(this.overlappingLCDMode == 0) // No Overlapping LCD
            return;
        if(this.overlappingLCDMode == 1) { // Auto
            int newLCDWidth = NativeLib.getScreenWidth();
            int newLCDHeight = NativeLib.getScreenHeight();
            int newWidth = Math.max(1, newLCDWidth);
            int newHeight = Math.max(1, newLCDHeight);

            post(() -> {
                FrameLayout.LayoutParams viewFlowLayout = new FrameLayout.LayoutParams((int) (newWidth * viewScaleFactorX), (int) (newHeight * viewScaleFactorY));
                viewFlowLayout.leftMargin = (int)(viewScaleFactorX * NativeLib.getScreenPositionX() + viewPanOffsetX);
                viewFlowLayout.topMargin = (int)(viewScaleFactorY * NativeLib.getScreenPositionY() + viewPanOffsetY);
                if(debug) Log.d(TAG, "updateLayout() leftMargin: " + viewFlowLayout.leftMargin + ", topMargin: " + viewFlowLayout.topMargin
                        + ", width: " + viewFlowLayout.width + ", height: " + viewFlowLayout.height);
                setLayoutParams(viewFlowLayout);
                setVisibility(View.VISIBLE);
            });
        }
    }

    public void saveViewLayout() {
        if(this.overlappingLCDMode > 0)
            return;
        FrameLayout.LayoutParams viewFlowLayout = (FrameLayout.LayoutParams)getLayoutParams();
        SharedPreferences.Editor editor = sharedPreferences.edit();
        editor.putString("settings_lcd_overlapping_mode", Integer.toString(this.overlappingLCDMode));
        editor.putInt("settings_lcd_overlapping_x", viewFlowLayout.leftMargin);
        editor.putInt("settings_lcd_overlapping_y", viewFlowLayout.topMargin);
        editor.putFloat("settings_lcd_overlapping_scale", bitmapLCD != null && bitmapLCD.getWidth() > 0 ? (float)viewFlowLayout.width / (float)bitmapLCD.getWidth() : 1.0f);
        editor.apply();
    }

    private void changeOverlappingLCDMode() {
        if(this.overlappingLCDMode == 1) { // Mode Auto
            this.overlappingLCDMode = 2; // We change the mode to Manual
            SharedPreferences.Editor editor = sharedPreferences.edit();
            editor.putString("settings_lcd_overlapping_mode", Integer.toString(this.overlappingLCDMode));
            editor.apply();
            Context context = getContext();
            if(context != null)
                Utils.showAlert(context, context.getString(Utils.resId(context, "string", "message_change_overlapping_lcd_mode_to_manual")));
        }
    }

    public void setOverlappingLCDMode(int overlappingLCDMode, boolean isDynamic) {
        if(debug) Log.d(TAG, "setOverlappingLCDMode(" + overlappingLCDMode + ")");
        int previousOverlappingLCDMode = this.overlappingLCDMode;
        this.overlappingLCDMode = overlappingLCDMode;
        if(previousOverlappingLCDMode == 0) {          // Off 0
            if(overlappingLCDMode == 1)                 // Auto 1
                this.updateLayout(this.mainScreenView.viewPanOffsetX, this.mainScreenView.viewPanOffsetY, this.mainScreenView.viewScaleFactorX, this.mainScreenView.viewScaleFactorY);
            else if(overlappingLCDMode == 2)           // Manual 2
                setVisibility(VISIBLE);
        } else if(previousOverlappingLCDMode == 1) {   // Auto 1
            if(overlappingLCDMode == 0)                 // Off 0
                setVisibility(GONE);
        } else if(previousOverlappingLCDMode == 2) {   // Manual 2
            if(overlappingLCDMode == 0)                 // Off 0
                setVisibility(GONE);
            else if(overlappingLCDMode == 1)            // Auto 1
                this.updateLayout(this.mainScreenView.viewPanOffsetX, this.mainScreenView.viewPanOffsetY, this.mainScreenView.viewScaleFactorX, this.mainScreenView.viewScaleFactorY);
        }
    }
}
