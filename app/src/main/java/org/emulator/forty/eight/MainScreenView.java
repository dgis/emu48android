package org.emulator.forty.eight;

import android.app.Activity;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Set;

public class MainScreenView extends PanAndScaleView {

    protected static final String TAG = "MainScreenView";
    protected final boolean debug = false;

    private Paint paint = new Paint();
    private Bitmap bitmapMainScreen;
    private HashMap<Integer, Integer> vkmap;
    private int kmlBackgroundColor = Color.BLACK;
    private boolean useKmlBackgroundColor = false;
    private int fallbackBackgroundColorType = 0;
    private int statusBarColor = 0;
    private boolean viewSized = false;
    private int rotationMode = 0;
    private boolean autoRotation = false;
    private boolean autoZoom = false;

    public MainScreenView(Context context) {
        super(context);

        setShowScaleThumbnail(true);
        setAllowDoubleTapZoom(false);

        paint.setFilterBitmap(true);
        paint.setAntiAlias(true);

        DisplayMetrics displayMetrics = new DisplayMetrics();
        ((Activity)context).getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        bitmapMainScreen = Bitmap.createBitmap(displayMetrics.widthPixels, displayMetrics.heightPixels, Bitmap.Config.ARGB_8888);
        bitmapMainScreen.eraseColor(Color.BLACK);

        vkmap = new HashMap<>();
        //vkmap.put(KeyEvent.KEYCODE_BACK, 0x08); // VK_BACK
        vkmap.put(KeyEvent.KEYCODE_TAB, 0x09); // VK_TAB
        vkmap.put(KeyEvent.KEYCODE_ENTER, 0x0D); // VK_RETURN
        vkmap.put(KeyEvent.KEYCODE_NUMPAD_ENTER, 0x0D); // VK_RETURN
        //vkmap.put(KeyEvent.KEYCODE_DEL, 0x2E); // VK_DELETE
        vkmap.put(KeyEvent.KEYCODE_DEL, 0x08); // VK_DELETE
        vkmap.put(KeyEvent.KEYCODE_INSERT, 0x2D); // VK_INSERT
        vkmap.put(KeyEvent.KEYCODE_SHIFT_LEFT, 0x10); // VK_SHIFT
        vkmap.put(KeyEvent.KEYCODE_SHIFT_RIGHT, 0x10); // VK_SHIFT
        vkmap.put(KeyEvent.KEYCODE_CTRL_LEFT, 0x11); // VK_CONTROL
        vkmap.put(KeyEvent.KEYCODE_CTRL_RIGHT, 0x11); // VK_CONTROL
        vkmap.put(KeyEvent.KEYCODE_ESCAPE, 0x1B); // VK_ESCAPE
        vkmap.put(KeyEvent.KEYCODE_SPACE, 0x20); // VK_SPACE
        vkmap.put(KeyEvent.KEYCODE_DPAD_LEFT, 0x25); // VK_LEFT
        vkmap.put(KeyEvent.KEYCODE_DPAD_UP, 0x26); // VK_UP
        vkmap.put(KeyEvent.KEYCODE_DPAD_RIGHT, 0x27); // VK_RIGHT
        vkmap.put(KeyEvent.KEYCODE_DPAD_DOWN, 0x28); // VK_DOWN
//        vkmap.put(KeyEvent.KEYCODE_0, 0x30); // 0
//        vkmap.put(KeyEvent.KEYCODE_1, 0x31); // 1
//        vkmap.put(KeyEvent.KEYCODE_2, 0x32); // 2
//        vkmap.put(KeyEvent.KEYCODE_3, 0x33); // 3
//        vkmap.put(KeyEvent.KEYCODE_4, 0x34); // 4
//        vkmap.put(KeyEvent.KEYCODE_5, 0x35); // 5
//        vkmap.put(KeyEvent.KEYCODE_6, 0x36); // 6
//        vkmap.put(KeyEvent.KEYCODE_7, 0x37); // 7
//        vkmap.put(KeyEvent.KEYCODE_8, 0x38); // 8
//        vkmap.put(KeyEvent.KEYCODE_9, 0x39); // 9
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
        vkmap.put(KeyEvent.KEYCODE_0, 0x60); // VK_NUMPAD0
        vkmap.put(KeyEvent.KEYCODE_1, 0x61); // VK_NUMPAD1
        vkmap.put(KeyEvent.KEYCODE_2, 0x62); // VK_NUMPAD2
        vkmap.put(KeyEvent.KEYCODE_3, 0x63); // VK_NUMPAD3
        vkmap.put(KeyEvent.KEYCODE_4, 0x64); // VK_NUMPAD4
        vkmap.put(KeyEvent.KEYCODE_5, 0x65); // VK_NUMPAD5
        vkmap.put(KeyEvent.KEYCODE_6, 0x66); // VK_NUMPAD6
        vkmap.put(KeyEvent.KEYCODE_7, 0x67); // VK_NUMPAD7
        vkmap.put(KeyEvent.KEYCODE_8, 0x68); // VK_NUMPAD8
        vkmap.put(KeyEvent.KEYCODE_9, 0x69); // VK_NUMPAD9
        vkmap.put(KeyEvent.KEYCODE_NUMPAD_MULTIPLY, 0x6A); // VK_MULTIPLY
        vkmap.put(KeyEvent.KEYCODE_NUMPAD_ADD, 0x6B); // VK_ADD
        vkmap.put(KeyEvent.KEYCODE_NUMPAD_SUBTRACT, 0x6D); // VK_SUBTRACT
        vkmap.put(KeyEvent.KEYCODE_NUMPAD_DOT, 0x6E); // VK_DECIMAL
        vkmap.put(KeyEvent.KEYCODE_NUMPAD_DIVIDE, 0x6F); // VK_DIVIDE
//        vkmap.put(KeyEvent.KEYCODE_SEMICOLON, 0xBA); // VK_OEM_1 (:);)
        vkmap.put(KeyEvent.KEYCODE_COMMA, 0xBC); // VK_OEM_COMMA
        vkmap.put(KeyEvent.KEYCODE_PERIOD, 0xBE); // VK_OEM_PERIOD
//        vkmap.put(KeyEvent.0x2C, 0xBF); // VK_OEM_2 (?)
//        vkmap.put(KeyEvent.0x32, 0xC0); // VK_OEM_3 (~)
//        vkmap.put(KeyEvent.KEYCODE_LEFT_BRACKET, 0xDB); // VK_OEM_4 ([{)
//        vkmap.put(KeyEvent.0x27, 0xDE);  // VK_OEM_7 (‘ »)

        this.setFocusable(true);
        this.setFocusableInTouchMode(true);

        // This call is necessary, or else the
        // draw method will not be called.
        setWillNotDraw(false);

//        scroller.setFriction(0.00001f); // ViewConfiguration.getScrollFriction(); // ViewConfiguration.SCROLL_FRICTION = 0.015f;
//        setOnTapDownListener(new OnTapListener() {
//            @Override
//            public boolean onTap(View v, float x, float y) {
//                if(NativeLib.buttonDown((int)x, (int)y)) {
//                    if(debug) Log.d(TAG, "onTapDown() true");
//                        return true;
//                }
//                if(debug) Log.d(TAG, "onTapDown() false");
//                return false;
//            }
//        });

//        setOnTapUpListener(new OnTapListener() {
//            @Override
//            public boolean onTap(View v, float x, float y) {
//                if(debug) Log.d(TAG, "onTapUp()");
//                NativeLib.buttonUp((int)x, (int)y);
//                return false;
//            }
//        });
    }

    protected Set<Integer> currentButtonTouched = new HashSet<Integer>();
    public boolean onTouchEvent(MotionEvent event) {
        int actionIndex = event.getActionIndex();
        int action = event.getActionMasked();
        switch (action) {
        case MotionEvent.ACTION_DOWN:
        case MotionEvent.ACTION_POINTER_DOWN:
            //Log.d(TAG, "ACTION_DOWN/ACTION_POINTER_DOWN count: " + touchCount + ", actionIndex: " + actionIndex);
                //NativeLib.buttonDown((int)((event.getX(actionIndex) - screenOffsetX) / screenScaleX), (int)((event.getY(actionIndex) - screenOffsetY) / screenScaleY));
                currentButtonTouched.remove(actionIndex);
                if(actionIndex == 0 && event.getPointerCount() == 1)
                    currentButtonTouched.clear();
                if (NativeLib.buttonDown((int) ((event.getX(actionIndex) - viewPanOffsetX) / viewScaleFactorX),
                        (int) ((event.getY(actionIndex) - viewPanOffsetY) / viewScaleFactorY))) {
                    currentButtonTouched.add(actionIndex);
                    preventToScroll = true;
//                    if (debug) Log.d(TAG, "onTouchEvent() ACTION_DOWN true, actionIndex: " + actionIndex + ", currentButtonTouched: " + currentButtonTouched.size());
//                    return true;
                }
                if (debug) Log.d(TAG, "onTouchEvent() ACTION_DOWN false, actionIndex: " + actionIndex
                        + ", currentButtonTouched: " + currentButtonTouched.size()
                        + ", preventToScroll: " + preventToScroll + ", getPointerCount: " + event.getPointerCount());
            break;
        case MotionEvent.ACTION_UP:
        case MotionEvent.ACTION_POINTER_UP:
            //Log.d(TAG, "ACTION_UP/ACTION_POINTER_UP count: " + touchCount + ", actionIndex: " + actionIndex);
                //NativeLib.buttonUp((int)((event.getX(actionIndex) - screenOffsetX) / screenScaleX), (int)((event.getY(actionIndex) - screenOffsetY) / screenScaleY));
                NativeLib.buttonUp((int) ((event.getX(actionIndex) - viewPanOffsetX) / viewScaleFactorX), (int) ((event.getY(actionIndex) - viewPanOffsetY) / viewScaleFactorY));
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
        if((event.getFlags() & KeyEvent.FLAG_VIRTUAL_HARD_KEY) == 0) {
            Integer windowsKeycode = vkmap.get(keyCode);
            if (windowsKeycode != null)
                NativeLib.keyDown(windowsKeycode);
            else
                Log.e(TAG, String.format("Unknown keyCode: %d", keyCode));
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if((event.getFlags() & KeyEvent.FLAG_VIRTUAL_HARD_KEY) == 0) {
            Integer windowsKeycode = vkmap.get(keyCode);
            if (windowsKeycode != null)
                NativeLib.keyUp(windowsKeycode);
            else
                Log.e(TAG, String.format("Unknown keyCode: %d", keyCode));
        }
        return super.onKeyUp(keyCode, event);
    }

    @Override
    protected void onSizeChanged(int viewWidth, int viewHeight, int oldViewWidth, int oldViewHeight) {
//        super.onSizeChanged(viewWidth, viewHeight, oldViewWidth, oldViewHeight);

        viewSizeWidth = viewWidth;
        viewSizeHeight = viewHeight;

        viewSized = true;
        updateLayout();
    }

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
                        //((Activity) getContext()).setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
                        if (imageRatio < 1.0f != viewRatio < 1.0f) {
                            // With have different screens orientations, so we automatically zoom
                            float translateX, translateY, scale;
                            if(viewRatio > imageRatio) {
                                float alpha = viewRatio / imageRatio;
                                scale = Math.min(2, alpha) * viewSizeWidth / virtualSizeWidth;
                                translateX = viewSizeWidth - scale * virtualSizeWidth;
                                translateY = (viewSizeHeight - scale * virtualSizeHeight) / 2.0f;
                            } else {
                                float beta = imageRatio / viewRatio;
                                scale = Math.min(2, beta) * viewSizeHeight / virtualSizeHeight;
                                translateX = (viewSizeWidth - scale * virtualSizeWidth) / 2.0f;
                                translateY = 0.0f;
                            }

                            viewScaleFactorX = scale;
                            viewScaleFactorY = scale;
                            scaleFactorMin = scale;
                            scaleFactorMax = maxZoom * scaleFactorMin;
                            viewPanOffsetX = translateX;
                            viewPanOffsetY = translateY;

                            constrainPan();
                            return;
                        }
                    } else {
                        //((Activity) getContext()).setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
                    }
                }
            }
            // Else, the screens orientations are the same, so we set the calculator in fullscreen
            resetViewport();
        }
    }

    /**
     * Draw the score.
     * @param canvas The canvas to draw to coming from the View.onDraw() method.
     */
    @Override
    protected void onCustomDraw(Canvas canvas) {
        //Log.d(TAG, "onCustomDraw()");

        canvas.drawColor(getBackgroundColor());
        canvas.drawBitmap(bitmapMainScreen, 0, 0, paint);
    }

    final int CALLBACK_TYPE_INVALIDATE = 0;
    final int CALLBACK_TYPE_WINDOW_RESIZE = 1;

    int updateCallback(int type, int param1, int param2, String param3, String param4) {
        switch (type) {
            case CALLBACK_TYPE_INVALIDATE:
                //Log.d(TAG, "PAINT updateCallback() postInvalidate()");
                postInvalidate();
                break;
            case CALLBACK_TYPE_WINDOW_RESIZE:
                // New Bitmap size
                if(bitmapMainScreen == null || bitmapMainScreen.getWidth() != param1 || bitmapMainScreen.getHeight() != param2) {
                    if(debug) Log.d(TAG, "updateCallback() Bitmap.createBitmap(x: " + Math.max(1, param1) + ", y: " + Math.max(1, param2) + ")");
                    Bitmap  oldBitmapMainScreen = bitmapMainScreen;
                    bitmapMainScreen = Bitmap.createBitmap(Math.max(1, param1), Math.max(1, param2), Bitmap.Config.ARGB_8888);
                    int globalColor = NativeLib.getGlobalColor();
                    kmlBackgroundColor = Color.argb(255, (globalColor & 0x00FF0000) >> 16, (globalColor & 0x0000FF00) >> 8, globalColor & 0x000000FF);

                    bitmapMainScreen.eraseColor(getBackgroundColor());
                    NativeLib.changeBitmap(bitmapMainScreen);

                    if(oldBitmapMainScreen != null) {
                        oldBitmapMainScreen.recycle();
                    }
                    firstTime = true;
                    setVirtualSize(bitmapMainScreen.getWidth(), bitmapMainScreen.getHeight());
                    if(viewSized)
                        updateLayout();
                }
                //postInvalidate();
                break;
        }
        return -1;
    }

    public Bitmap getBitmapMainScreen() {
        return bitmapMainScreen;
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



    private int getBackgroundColor() {
        if(useKmlBackgroundColor) {
            return kmlBackgroundColor;
        } else switch(fallbackBackgroundColorType) {
            case 0:
                return 0;
            case 1:
                return statusBarColor;
        }
        return 0;
    }
}
