package com.regis.cosnier.emu48;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.support.v4.view.ViewCompat;
import android.view.GestureDetector;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.SurfaceView;
import android.view.View;
import android.widget.OverScroller;

public class MainScreenView extends SurfaceView {
    protected static final String TAG = "MainScreenView";

    public MainScreenView(Context context) {
        super(context);

        //commonInitialize(context, new Project());
    }

//    public MainScreenView(Context context, final Project currentProject) {
//        super(context);
//
//        commonInitialize(context, currentProject);
//    }

//    /**
//     * Common initialize method.
//     * @param context The activity context.
//     * @param currentProject The current project.
//     */
//    private void commonInitialize(Context context, final Project currentProject) {
//        //this.mContext = context;
//        this.currentProject = currentProject;
//
//        // Prevent the out of memory in OpenGL with Tegra 2 devices
//        //if(!Utils.hasHighMemory(context))
//        setLayerType(View.LAYER_TYPE_SOFTWARE, null);
//        //mAntiAliased = Utils.hasHighMemory(context);
//        mScreenDensity = getResources().getDisplayMetrics().density;
//
//        mVectorsCanvasRenderer = new VectorsCanvasRenderer();
//        mVectorsCanvasRenderer.setStrokeColor(0xff000000); // Black
//        Paint paint = mVectorsCanvasRenderer.getPaint();
//        paint.setColor(Color.BLACK);
//        paint.setStyle(Paint.Style.STROKE);
//        paint.setStrokeWidth(1.0f * mScreenDensity);
//        paint.setAntiAlias(mAntiAliased);
//
//        mGestureDetector = new GestureDetector(context, new GestureDetector.SimpleOnGestureListener() {
//
//            @Override
//            public boolean onDoubleTap(MotionEvent e) {
//                float scaleFactorPrevious = currentProject.viewScaleFactor;
//                currentProject.viewScaleFactor *= 2f;
//                if(currentProject.viewScaleFactor > mScaleFactorMax)
//                    currentProject.viewScaleFactor = mScaleFactorMin;
//                else {
//                    constrainScale();
//                    scaleWithFocus(e.getX(), e.getY(), scaleFactorPrevious);
//                }
//                constrainPan();
//                invalidate();
//                return true;
//            }
//
//            @Override
//            public boolean onDown(MotionEvent e) {
//                //Log.d(TAG, "GestureDetector.onDown()");
//                if(!mOverScroller.isFinished()) {
//                    mOverScroller.forceFinished(true);
//                    //ViewCompat.postInvalidateOnAnimation(CanvasView.this);
//                    //invalidate();
//                }
//                return true;
//            }
//
//        });
//
//        this.setFocusable(true);
//        this.setFocusableInTouchMode(true);
//
//        // This call is necessary, or else the
//        // draw method will not be called.
//        setWillNotDraw(false);
//    }

//
//    @SuppressLint("ClickableViewAccessibility")
//    public boolean onTouchEvent(MotionEvent event) {
//        // Let the ScaleGestureDetector inspect all events.
//        boolean retValScaleDetector = mScaleDetector.onTouchEvent(event);
//        boolean retValGestureDetector = mGestureDetector.onTouchEvent(event);
//        boolean retVal = retValGestureDetector || retValScaleDetector;
//
//        int touchCount = event.getPointerCount();
//        if(touchCount == 1) {
//            int action = event.getAction();
//            switch (action) {
////			case MotionEvent.ACTION_DOWN:
////
////			    mPanPrevious.x = event.getX(0);
////			    mPanPrevious.y = event.getY(0);
////			    mIsPanning = true;
////
////			    //Log.d(TAG, "ACTION_DOWN count: " + touchCount + ", PanPrevious: " + mPanPrevious.toString());
////
////				break;
////			case MotionEvent.ACTION_MOVE:
////				float panCurrentX = event.getX(0);
////				float panCurrentY = event.getY(0);
////				if(mIsPanning && Math.abs(panCurrentX - mPanPrevious.x) > 1.0f && Math.abs(panCurrentY - mPanPrevious.y) > 1.0f) {
////					mPanCurrent.x = panCurrentX;
////					mPanCurrent.y = panCurrentY;
////					currentProject.viewPanOffsetX += mPanCurrent.x - mPanPrevious.x;
////					currentProject.viewPanOffsetY += mPanCurrent.y - mPanPrevious.y;
////
////					//Log.d(TAG, "Before viewPanOffsetX: " + currentProject.viewPanOffsetX + ", viewPanOffsetY: " + currentProject.viewPanOffsetY);
////
////				    constrainScale();
////				    constrainPan();
////
////					//Log.d(TAG, "After  viewPanOffsetX: " + currentProject.viewPanOffsetX + ", viewPanOffsetY: " + currentProject.viewPanOffsetY);
////
////				    //Log.d(TAG, "ACTION_MOVE count: " + touchCount + ", PanPrevious: " + mPanPrevious.toString() + ", PanCurrent: " + mPanCurrent.toString());
////
////					mPanPrevious.x = mPanCurrent.x;
////					mPanPrevious.y = mPanCurrent.y;
////
////			        invalidate();
////				}
////				break;
//                case MotionEvent.ACTION_UP:
//                    if(mIsPanning) {
//                        //Log.d(TAG, "Stop PANNING");
//                        mIsPanning = false;
//                        if(!mIsFlinging)
//                            invalidate();
//                    }
//                    break;
////			case MotionEvent.ACTION_CANCEL:
////				break;
////			case MotionEvent.ACTION_OUTSIDE:
////				break;
//                default:
//            }
//            return true;
//        }
//        return retVal || super.onTouchEvent(event);
//    }
//
//    @Override
//    public boolean onGenericMotionEvent(MotionEvent event) {
//        if ((event.getSource() & InputDevice.SOURCE_CLASS_POINTER) != 0) {
//            switch (event.getAction()) {
//                case MotionEvent.ACTION_SCROLL:
//                    float wheelDelta = event.getAxisValue(MotionEvent.AXIS_VSCROLL);
//                    if(wheelDelta > 0f)
//                        scaleByStep(mScaleStep, event.getX(), event.getY());
//                    else if(wheelDelta < 0f)
//                        scaleByStep(1.0f / mScaleStep, event.getX(), event.getY());
//
//                    return true;
//            }
//        }
//        return super.onGenericMotionEvent(event);
//    }
//
//    @Override
//    public boolean onKeyUp(int keyCode, KeyEvent event) {
//        char character = (char)event.getUnicodeChar();
//        if(character == '+') {
//            scaleByStep(mScaleStep, getWidth() / 2.0f, getHeight() / 2.0f);
//            return true;
//        } else if(character == '-') {
//            scaleByStep(1.0f / mScaleStep, getWidth() / 2.0f, getHeight() / 2.0f);
//            return true;
//        }
//        return super.onKeyUp(keyCode, event);
//    }

    @Override
    protected void onSizeChanged(int viewWidth, int viewHeight, int oldViewWidth, int oldViewHeight) {
        super.onSizeChanged(viewWidth, viewHeight, oldViewWidth, oldViewHeight);

//        mViewBounds.set(0.0f, 0.0f, viewWidth, viewHeight);
//        resetViewport((float)viewWidth, (float)viewHeight);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        //Log.d(TAG, "onDraw() mIsScaling: " + mIsScaling + ", mIsPanning: " + mIsPanning + ", mIsFlinging: " + mIsFlinging);

//        //renderPlasma(mBitmap, System.currentTimeMillis() - mStartTime);
//        canvas.drawBitmap(bitmapMainScreen, 0, 0, null);

//        Paint paint = mVectorsCanvasRenderer.getPaint();
//
//        canvas.drawColor(Color.rgb(224, 224, 224));
//
//        canvas.save();
//        canvas.translate(currentProject.viewPanOffsetX, currentProject.viewPanOffsetY);
//        canvas.scale(currentProject.viewScaleFactor, currentProject.viewScaleFactor);
//        paint.setColor(Color.BLACK);
//        mVectorsCanvasRenderer.setCanvas(canvas);
//        mVectorsCanvasRenderer.setShowLabels(currentProject.showLabels);
//        if(mIsPanning || mIsScaling || mIsFlinging)
//        {
//            // We are scaling or panning, trying to optimize...
//            if(mVectorsCanvasRenderer.getBackground() == null)
//                // Display polygons only if there is no background map
//                mVectorsCanvasRenderer.drawVectors(false, true, true, false);
//            else
//                // Best optimization
//                mVectorsCanvasRenderer.drawBackgroundMap();
//        }
//        else
//            // Draw everything following the parameters
//            mVectorsCanvasRenderer.drawVectors();
//        canvas.restore();
//
//        if(currentProject.viewScaleFactor > mScaleFactorMin) {
//            // Draw the scale thumbnail
//            paint.setColor(Color.RED);
//            paint.setStrokeWidth(1.0f * mScreenDensity);
//
//            float scale = 0.2f;
//            if(mImageSize.x > mImageSize.y)
//                scale *= mViewBounds.width() / mImageSize.x;
//            else
//                scale *= mViewBounds.height() / mImageSize.y;
//            float marginX = 10f * mScreenDensity, marginY = 10f * mScreenDensity;
//            mRectScaleImage.set(mViewBounds.right - marginX - scale * mImageSize.x,
//                    mViewBounds.bottom - marginY - scale * mImageSize.y,
//                    mViewBounds.right - marginX,
//                    mViewBounds.bottom - marginY
//            );
//            canvas.drawRect(mRectScaleImage, paint);
//            if(backgroundMapIndex >= 0 && mBitmap != null) {
//                //Bitmap background = nativePNP.getBitmap(ParamImageSourceMat);
//                mRectBitmapSource.set(0, 0, (int)mImageSize.x, (int)mImageSize.y);
//                canvas.drawBitmap(mBitmap, mRectBitmapSource, mRectScaleImage, paint);
//            }
//
//            mRectScaleView.set(mRectScaleImage.left + scale * (-currentProject.viewPanOffsetX / currentProject.viewScaleFactor),
//                    mRectScaleImage.top + scale * (-currentProject.viewPanOffsetY / currentProject.viewScaleFactor),
//                    mRectScaleImage.left + scale * (mViewBounds.width() - currentProject.viewPanOffsetX) / currentProject.viewScaleFactor,
//                    mRectScaleImage.top + scale * (mViewBounds.height() - currentProject.viewPanOffsetY) / currentProject.viewScaleFactor
//            );
//            canvas.drawRect(mRectScaleView, paint);
        }
    }
}
