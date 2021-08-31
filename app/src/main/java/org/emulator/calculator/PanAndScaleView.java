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
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PointF;
import android.graphics.RectF;
import android.os.Handler;
import android.os.Looper;
import android.util.AttributeSet;
import android.util.Log;
import android.view.GestureDetector;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.View;
import android.widget.OverScroller;

import androidx.core.view.ViewCompat;

/* Inherit from View and not SurfaceView to prevent 2 SurfaceView with one over the other! */
public class PanAndScaleView extends View {

	protected static final String TAG = "PanAndScaleView";
	protected final boolean debug = false;

    protected ScaleGestureDetector scaleDetector;
    protected GestureDetector gestureDetector;
    protected OverScroller scroller;
	protected boolean preventToScroll = false;
	protected boolean hasScrolled = false;
    protected PointF panPrevious = new PointF(0f, 0f);
	protected boolean preventPinchZoom = false;

    protected RectF rectScaleView = new RectF();
    protected RectF rectScaleImage = new RectF();

    protected float scaleFactorMin = 1.f;
    protected float scaleFactorMax = 8.f;
    protected float maxZoom = 8.f;
    protected float screenDensity = 1.0f;
	protected float scaleStep = (float)Math.sqrt(2.0);

    protected boolean showCursor = false;
    protected PointF cursorLocation = new PointF(0f, 0f);
	protected boolean showScaleThumbnail = false;
	protected int scaleThumbnailColor = Color.WHITE;
	protected boolean allowDoubleTapZoom = true;
	protected boolean fillBounds = false;
	protected boolean enablePanAndScale = false;
	protected boolean enableZoomKeyboard = true;

	protected Paint paint = null;
	protected OnTapListener onTapDownListener;
	protected OnTapListener onTapUpListener;

	public float viewSizeWidth = 0.0f; // pixel of the view
	public float viewSizeHeight = 0.0f; // pixel of the view
	public float virtualSizeWidth = 0f; // pixel of the pan and zoom bitmap
	public float virtualSizeHeight = 0f; // pixel of the pan and zoom bitmap

	public float viewPanOffsetX; // pixel of the view
	public float viewPanOffsetY; // pixel of the view
	public float viewScaleFactorX; // pixel of the view / pixel of the pan and zoom bitmap
	public float viewScaleFactorY; // pixel of the view / pixel of the pan and zoom bitmap

	protected float previousViewPanOffsetX;
	protected float previousViewPanOffsetY;
	protected float previousViewScaleFactorX;
	protected float previousViewScaleFactorY;
	protected boolean firstTime = false;

	protected boolean viewHasChanged() {
		if((int)viewPanOffsetX != (int)previousViewPanOffsetX || (int)viewPanOffsetY != (int)previousViewPanOffsetY
		|| (int)viewScaleFactorX != (int)previousViewScaleFactorX || (int)viewScaleFactorY != (int)previousViewScaleFactorY) {
			if(debug) Log.d(TAG, "viewHasChanged() previousViewPanOffsetX: " + previousViewPanOffsetX + ", viewPanOffsetX: " + viewPanOffsetX
												   + ", previousViewPanOffsetY: " + previousViewPanOffsetY + ", viewPanOffsetY: " + viewPanOffsetY
												   + ", previousViewScaleFactorX: " + previousViewScaleFactorX + ", previousViewScaleFactorY: " + previousViewScaleFactorY
												   + ", previousViewScaleFactorY: " + previousViewScaleFactorY + ", viewScaleFactorY: " + viewScaleFactorY);

			previousViewPanOffsetX = viewPanOffsetX;
			previousViewPanOffsetY = viewPanOffsetY;
			previousViewScaleFactorX = viewScaleFactorX;
			previousViewScaleFactorY = viewScaleFactorY;
			return true;
		} else
			return false;
	}

	public PanAndScaleView(Context context) {
		super(context);
		
		commonInitialize(context);
	}
	
	public PanAndScaleView(Context context, AttributeSet attrs) {
		super(context, attrs);
		
		commonInitialize(context);
	}
	
	/**
	 * Interface definition for a callback to be invoked when a view is tapped.
	 */
	public interface OnTapListener {
		/**
		 * Called when a view has been tapped.
		 *
		 * @param v The view that was clicked.
		 * @param x The virtual view x coordinate.
		 * @param y The virtual view y coordinate.
		 * @return true if onTap is handled.
		 */
		boolean onTap(View v, float x, float y);
	}

	/**
	 * Register a callback to be invoked when this view is tapped down.
	 * @param onTapDownListener The callback that will run
	 */
	@SuppressWarnings("unused")
	public void setOnTapDownListener(OnTapListener onTapDownListener) {
		this.onTapDownListener = onTapDownListener;
	}

	/**
	 * Register a callback to be invoked when this view is tapped up.
	 * @param onTapUpListener The callback that will run
	 */
	@SuppressWarnings("unused")
	public void setOnTapUpListener(OnTapListener onTapUpListener) {
		this.onTapUpListener = onTapUpListener;
	}

	@SuppressWarnings("unused")
	public float getMaxZoom() {
        return maxZoom;
    }

	@SuppressWarnings("unused")
    public void setMaxZoom(float maxZoom) {
    	this.maxZoom = maxZoom;
    }

	@SuppressWarnings("unused")
    public boolean getShowCursor() {
        return showCursor;
    }

	@SuppressWarnings("unused")
    public void setShowCursor(boolean showCursor) {
        this.showCursor = showCursor;
    }

	@SuppressWarnings("unused")
    public PointF getCursorLocation() {
        return cursorLocation;
    }

	@SuppressWarnings("unused")
    public void setCursorLocation(float x, float y) {
        cursorLocation.set(x, y);
    }

	/**
	 * @return true to show a small scale thumbnail in the bottom right; false otherwise.
	 */
	@SuppressWarnings("unused")
	public boolean getShowScaleThumbnail() {
		return showScaleThumbnail;
	}

	/**
	 * @param showScaleThumbnail true to show a small scale thumbnail in the bottom right; false otherwise.
	 */
	@SuppressWarnings("unused")
	public void setShowScaleThumbnail(boolean showScaleThumbnail) {
		this.showScaleThumbnail = showScaleThumbnail;
	}

	/**
	 * @return true to allow to zoom when double tapping; false otherwise.
	 */
	@SuppressWarnings("unused")
	public boolean getAllowDoubleTapZoom() {
		return allowDoubleTapZoom;
	}

	/**
	 * @param allowDoubleTapZoom true to allow to zoom when double tapping; false otherwise.
	 */
	@SuppressWarnings("unused")
	public void setAllowDoubleTapZoom(boolean allowDoubleTapZoom) {
		this.allowDoubleTapZoom = allowDoubleTapZoom;
	}


	/**
	 * @return true to allow the virtual space to fill view bounds the ; false otherwise.
	 */
	@SuppressWarnings("unused")
	public boolean getFillBounds() {
		return fillBounds;
	}

	/**
	 * @param fillBounds true to allow the virtual space to fill the view bounds; false otherwise.
	 */
	@SuppressWarnings("unused")
	public void setFillBounds(boolean fillBounds) {
		this.fillBounds = fillBounds;
	}

	/**
	 * @return true to allow to pan and scale; false otherwise.
	 */
	@SuppressWarnings("unused")
	public boolean getEnablePanAndScale() {
		return enablePanAndScale;
	}

	/**
	 * @param enablePanAndScale true to allow to pan and scale; false otherwise.
	 */
	public void setEnablePanAndScale(boolean enablePanAndScale) {
		this.enablePanAndScale = enablePanAndScale;
	}

	public void setAllowPinchZoom(boolean allowPinchZoom) {
		this.preventPinchZoom = !allowPinchZoom;
	}


	public void setVirtualSize(float width, float height) {
		virtualSizeWidth = width;
		virtualSizeHeight = height;
	}

	private void commonInitialize(Context context) {

		paint = new Paint();
		paint.setColor(Color.BLACK);
		paint.setStyle(Paint.Style.STROKE);
		screenDensity = getResources().getDisplayMetrics().density;
		paint.setStrokeWidth(1.0f * screenDensity);
//		paint.setStrokeJoin(paint.Join.ROUND);
//		paint.setStrokeCap(paint.Cap.ROUND);
		//paint.setPathEffect(new CornerPathEffect(10));
		paint.setDither(true);
		paint.setAntiAlias(true);

		setPadding(0, 0, 0, 0);
		
		scroller = new OverScroller(context);

		scaleDetector = new ScaleGestureDetector(context, new ScaleGestureDetector.OnScaleGestureListener() {
		    @Override
		    public boolean onScaleBegin(ScaleGestureDetector detector) {
				if(debug) Log.d(TAG, "onScaleBegin()");
				return true;
			}

			@Override
		    public boolean onScale(ScaleGestureDetector detector) {
				if(debug) Log.d(TAG, "onScale()");
		    	if(fillBounds || preventPinchZoom)
		    		return false;
				float scaleFactorPreviousX = viewScaleFactorX;
				float scaleFactorPreviousY = viewScaleFactorY;
				float detectorScaleFactor = detector.getScaleFactor();
				viewScaleFactorX *= detectorScaleFactor;
				viewScaleFactorY *= detectorScaleFactor;
			    constrainScale();
	            scaleWithFocus(detector.getFocusX(), detector.getFocusY(), scaleFactorPreviousX, scaleFactorPreviousY);
			    constrainPan();
		        invalidate();
		        return true;
		    }

		    @Override
	        public void onScaleEnd(ScaleGestureDetector detector) {
				if(debug) Log.d(TAG, "onScaleEnd()");
		    }
		});
		
		gestureDetector = new GestureDetector(context, new GestureDetector.SimpleOnGestureListener() {

			@Override
			public boolean onDown(MotionEvent e) {
				if(debug) Log.d(TAG, "onDown() actionIndex: " + e.getActionIndex() + ", X: " + e.getX() + ", Y: " + e.getY());
				scroller.forceFinished(true);
				ViewCompat.postInvalidateOnAnimation(PanAndScaleView.this);
				if(onTapDownListener != null) {
					float scaleAndPanX = (e.getX() - viewPanOffsetX) / viewScaleFactorX;
					float scaleAndPanY = (e.getY() - viewPanOffsetY) / viewScaleFactorY;
					if(onTapDownListener.onTap(PanAndScaleView.this, scaleAndPanX, scaleAndPanY))
						return true;
				}
				return super.onDown(e);
			}

			@Override
			public boolean onSingleTapConfirmed(MotionEvent e) {
				if(debug) Log.d(TAG, "onSingleTapConfirmed() actionIndex: " + e.getActionIndex() + ", X: " + e.getX() + ", Y: " + e.getY());
				if(onTapUpListener != null) {
					float scaleAndPanX = (e.getX() - viewPanOffsetX) / viewScaleFactorX;
					float scaleAndPanY = (e.getY() - viewPanOffsetY) / viewScaleFactorY;
					if(onTapUpListener.onTap(PanAndScaleView.this, scaleAndPanX, scaleAndPanY))
						return true;
				}

				return super.onSingleTapConfirmed(e);
			}

			@Override
			public boolean onDoubleTap(MotionEvent e) {
				if(debug) Log.d(TAG, "onDoubleTap() actionIndex: " + e.getActionIndex() + ", X: " + e.getX() + ", Y: " + e.getY());
				if(!allowDoubleTapZoom || fillBounds)
					return false;
				float scaleFactorPreviousX = viewScaleFactorX;
				float scaleFactorPreviousY = viewScaleFactorY;
				viewScaleFactorX *= 2f;
				viewScaleFactorY *= 2f;
				if(viewScaleFactorX > scaleFactorMax || viewScaleFactorY > scaleFactorMax)
					viewScaleFactorX = viewScaleFactorY = scaleFactorMin;
				else {
					constrainScale();
		            scaleWithFocus(e.getX(), e.getY(), scaleFactorPreviousX, scaleFactorPreviousY);
				}
			    constrainPan();
		        invalidate();
				return true;
			}

			@Override
			public boolean onScroll(MotionEvent downEvent, MotionEvent moveEvent, float distanceX, float distanceY) {
				if(debug)
					Log.d(TAG, "onScroll() downEvent.actionIndex: " + (downEvent != null ? downEvent.getActionIndex() : "null")
							+ ", moveEvent.actionIndex: " + (downEvent != null ? moveEvent.getActionIndex() : "null")
							+ ", distanceX: " + distanceX + ", distanceY: " + distanceY
					);
				if(fillBounds || preventToScroll)
					return false;

				doScroll(distanceX, distanceY);
				return true;
			}

			@Override
			public boolean onFling(MotionEvent downEvent, MotionEvent moveEvent, float velocityX, float velocityY) {
				if(debug) Log.d(TAG, "onFling(..., velocityX: " + velocityX + ", velocityY: " + velocityY + ") downEvent.actionIndex: " + (downEvent != null ? downEvent.getActionIndex() : "null") + ", moveEvent.actionIndex: " + (downEvent != null ? moveEvent.getActionIndex() : "null"));
				if(fillBounds || preventToScroll)
					return false;

				float viewPanMinX = viewSizeWidth - virtualSizeWidth * viewScaleFactorX;
				float viewPanMinY = viewSizeHeight - virtualSizeHeight * viewScaleFactorY;

				// https://developer.android.com/training/gestures/scroll
				scroller.forceFinished(true);
				float velocityFactor = -1.0f;
				//scroller.setFriction(0.00001f); // ViewConfiguration.getScrollFriction(); // ViewConfiguration.SCROLL_FRICTION = 0.015f;
				//scroller.setFriction(0.0015f); // ViewConfiguration.getScrollFriction(); // ViewConfiguration.SCROLL_FRICTION = 0.015f;
				if(debug) Log.d(TAG, "scroller.fling(startX: " + (int) viewPanOffsetX + ", startY: " + (int) viewPanOffsetY
						+ ", velocityX: " + (int)(velocityFactor * velocityX) + ", velocityY: " + (int)(velocityFactor * velocityY)
						+ ", minX: " + viewPanMinX + ", maxX: " + 0
						+ ", minY: " + viewPanMinY + ", maxY: " + 0
						+ ")");
				scroller.fling(-(int)viewPanOffsetX, -(int)viewPanOffsetY,
						(int)(velocityFactor * velocityX), (int)(velocityFactor * velocityY),
						0, -(int)viewPanMinX,
						0, -(int)viewPanMinY);
				ViewCompat.postInvalidateOnAnimation(PanAndScaleView.this);
				return true;
			}
		});
		
		this.setFocusable(true);
		this.setFocusableInTouchMode(true);

		// This call is necessary, or else the draw method will not be called.
		setWillNotDraw(false);
	}

	@Override
	public void computeScroll() {
		super.computeScroll();

		if(fillBounds || preventToScroll)
			return;

		if (scroller.computeScrollOffset() && !fillBounds) {
			if(debug) Log.d(TAG, "computeScroll()");

			if (!hasScrolled) {
				panPrevious.x = scroller.getStartX();
				panPrevious.y = scroller.getStartY();
			}

			float deltaX = scroller.getCurrX() - panPrevious.x;
			float deltaY = scroller.getCurrY() - panPrevious.y;

			panPrevious.x = scroller.getCurrX();
			panPrevious.y = scroller.getCurrY();

			doScroll(deltaX, deltaY);

			hasScrolled = true;
		} else
			hasScrolled = false;
	}    

	public void doScroll(float deltaX, float deltaY) {
		if(debug) Log.d(TAG, "doScroll(" + deltaX + "," + deltaY + ")");

		viewPanOffsetX -= deltaX;
		viewPanOffsetY -= deltaY;
		if(debug) Log.d(TAG, "doScroll() before constraint viewPanOffsetX: " + viewPanOffsetX + ", viewPanOffsetY: " + viewPanOffsetY);
		constrainScale();
		constrainPan();
		if(debug) Log.d(TAG, "doScroll() after constraint viewPanOffsetX: " + viewPanOffsetX + ", viewPanOffsetY: " + viewPanOffsetY);
		invalidate();
	}

	@SuppressWarnings("unused")
	public void postDoScroll(float deltaX, float deltaY, boolean center) {
		if(fillBounds)
			return;

		viewPanOffsetX -= deltaX;
		viewPanOffsetY -= deltaY;
		constrainScale();
		constrainPan(center);
		postInvalidate();
	}

	@SuppressLint("ClickableViewAccessibility")
	@Override
	public boolean onTouchEvent(MotionEvent event) {
		if(enablePanAndScale) {
			// The order of operations is important
			boolean result = scaleDetector.onTouchEvent(event);
			result = gestureDetector.onTouchEvent(event) || result;
			return result || super.onTouchEvent(event);
		} else
			return super.onTouchEvent(event);
	}
	
	@Override
	public boolean onGenericMotionEvent(MotionEvent event) {
		if (enablePanAndScale && !fillBounds && (event.getSource() & InputDevice.SOURCE_CLASS_POINTER) != 0) {
			if (event.getAction() == MotionEvent.ACTION_SCROLL) {
				float wheelDelta = event.getAxisValue(MotionEvent.AXIS_VSCROLL);
				if (wheelDelta > 0f)
					scaleByStep(scaleStep, event.getX(), event.getY());
				else if (wheelDelta < 0f)
					scaleByStep(1.0f / scaleStep, event.getX(), event.getY());

				return true;
			}
		}
		return super.onGenericMotionEvent(event);
	}
	
	@Override
	public boolean onKeyUp(int keyCode, KeyEvent event) {
		if(enableZoomKeyboard && enablePanAndScale && !fillBounds) {
			char character = (char) event.getUnicodeChar();
			if (character == '+') {
				scaleByStep(scaleStep, getWidth() / 2.0f, getHeight() / 2.0f);
				return true;
			} else if (character == '-') {
				scaleByStep(1.0f / scaleStep, getWidth() / 2.0f, getHeight() / 2.0f);
				return true;
			}
		}
		return super.onKeyUp(keyCode, event);
	}
	
	// Scale the view step by step following the mouse/touch position
	private void scaleByStep(float scaleFactor, float x, float y) {
		float scaleFactorPreviousX = viewScaleFactorX;
		float scaleFactorPreviousY = viewScaleFactorY;
		viewScaleFactorX *= scaleFactor;
		viewScaleFactorY *= scaleFactor;
		if(viewScaleFactorX > scaleFactorMax || viewScaleFactorY > scaleFactorMax)
			viewScaleFactorX = viewScaleFactorY = scaleFactorMax;
		else if(viewScaleFactorX < scaleFactorMin || viewScaleFactorY < scaleFactorMin)
			viewScaleFactorX = viewScaleFactorY = scaleFactorMin;
		else {
			constrainScale();
		    scaleWithFocus(x, y, scaleFactorPreviousX, scaleFactorPreviousY);
		}
		constrainPan();
		invalidate();
	}
	
	private void scaleWithFocus(float focusX, float focusY, float scaleFactorPreviousX, float scaleFactorPreviousY) {
		viewPanOffsetX = viewPanOffsetX + (focusX - viewPanOffsetX) - (focusX - viewPanOffsetX) * viewScaleFactorX / scaleFactorPreviousX;
		viewPanOffsetY = viewPanOffsetY + (focusY - viewPanOffsetY) - (focusY - viewPanOffsetY) * viewScaleFactorY / scaleFactorPreviousY;
	}

	protected void constrainScale() {
        // Don't let the object get too small or too large.
		viewScaleFactorX = Math.max(scaleFactorMin, Math.min(viewScaleFactorX, scaleFactorMax));
		viewScaleFactorY = Math.max(scaleFactorMin, Math.min(viewScaleFactorY, scaleFactorMax));
	}
	protected void constrainPan() {
		constrainPan(true);
	}
	protected void constrainPan(boolean center) {

        // Keep the panning limits and the image centered.
		float viewWidth = viewSizeWidth;
		float viewHeight = viewSizeHeight;
		if(viewWidth == 0.0f) {
			viewWidth = 1.0f;
			viewHeight = 1.0f;
		}
		float virtualWidth = virtualSizeWidth;
		float virtualHeight = virtualSizeHeight;
		if(virtualWidth == 0.0f) {
			virtualWidth = 1.0f;
			virtualHeight = 1.0f;
		}
		float viewRatio = viewHeight / viewWidth;
		float imageRatio = virtualHeight / virtualWidth;
		float viewPanMinX = viewSizeWidth - virtualWidth * viewScaleFactorX;
		float viewPanMinY = viewSizeHeight - virtualHeight * viewScaleFactorY;
		if(viewRatio > imageRatio) {
	        // Keep the panning X limits.
			if(viewPanOffsetX < viewPanMinX)
				viewPanOffsetX = viewPanMinX;
			else if(viewPanOffsetX > 0f)
				viewPanOffsetX = 0f;
			
	        // Keep the image centered vertically.
			if(center && viewPanMinY > 0f)
				viewPanOffsetY = viewPanMinY / 2.0f;
			else {
				if(viewPanOffsetY > 0f)
					viewPanOffsetY = 0f;
				else if(viewPanOffsetY < viewPanMinY)
					viewPanOffsetY = viewPanMinY;
			}
		} else {
	        // Keep the panning Y limits.
			if(viewPanOffsetY < viewPanMinY)
				viewPanOffsetY = viewPanMinY;
			else if(viewPanOffsetY > 0f)
				viewPanOffsetY = 0f;

	        // Keep the image centered horizontally.
			if(center && viewPanMinX > 0f)
				viewPanOffsetX = viewPanMinX / 2.0f;
			else {
				if(viewPanOffsetX > 0f)
					viewPanOffsetX = 0f;
				else if(viewPanOffsetX < viewPanMinX)
					viewPanOffsetX = viewPanMinX;
			}
		}
	}

	public void resetViewport() {
		resetViewport(viewSizeWidth, viewSizeHeight);
	}

	public void resetViewport(float viewWidth, float viewHeight) {
		if(virtualSizeWidth > 0 && virtualSizeHeight > 0 && viewWidth > 0.0f && viewHeight > 0.0f) {
			if(fillBounds) {
				viewPanOffsetX = 0.0f;
				viewPanOffsetY = 0.0f;
				viewScaleFactorX = viewSizeWidth / virtualSizeWidth;
				viewScaleFactorY = viewSizeHeight / virtualSizeHeight;
			} else {
				// Find the scale factor and the translate offset to fit and to center the vectors in the view bounds.
				float translateX, translateY, scale;
				float viewRatio = viewHeight / viewWidth;
				float imageRatio = virtualSizeHeight / virtualSizeWidth;
				if (viewRatio > imageRatio) {
					scale = viewWidth / virtualSizeWidth;
					translateX = 0.0f;
					translateY = (viewHeight - scale * virtualSizeHeight) / 2.0f;
				} else {
					scale = viewHeight / virtualSizeHeight;
					translateX = (viewWidth - scale * virtualSizeWidth) / 2.0f;
					translateY = 0.0f;
				}

				viewScaleFactorX = scale;
				viewScaleFactorY = scale;
				scaleFactorMin = scale;
				scaleFactorMax = maxZoom * scaleFactorMin;
				viewPanOffsetX = translateX;
				viewPanOffsetY = translateY;
			}
		}
	}


	boolean osdAllowed = false;
	Handler osdTimerHandler = new Handler(Looper.getMainLooper());
	Runnable osdTimerRunnable = () -> {
		// OSD should stop now!
		osdAllowed = false;
		invalidate();
	};

	void startOSDTimer() {
		osdTimerHandler.removeCallbacks(osdTimerRunnable);
		osdAllowed = true;
		osdTimerHandler.postDelayed(osdTimerRunnable, 500);
	}


	@Override
	protected void onSizeChanged(int viewWidth, int viewHeight, int oldViewWidth, int oldViewHeight) {
		if(debug) Log.d(TAG, "onSizeChanged(viewWidth: " + viewWidth + ", viewHeight: " + viewHeight + ", oldViewWidth: " + oldViewWidth + ", oldViewHeight: " + oldViewHeight + ")");
		super.onSizeChanged(viewWidth, viewHeight, oldViewWidth, oldViewHeight);
		
		viewSizeWidth = viewWidth;
		viewSizeHeight = viewHeight;
		resetViewport((float)viewWidth, (float)viewHeight);
	}

	protected void onCustomDraw(Canvas canvas) {
	}

	@Override
    public void onDraw(Canvas canvas) {

        canvas.save();
        canvas.translate(viewPanOffsetX, viewPanOffsetY);
       	canvas.scale(viewScaleFactorX, viewScaleFactorY);
       	onCustomDraw(canvas);
        canvas.restore();
       	
       	if(showCursor) {
			paint.setColor(Color.RED);
			float cx = cursorLocation.x * viewScaleFactorX + viewPanOffsetX;
			float cy = cursorLocation.y * viewScaleFactorY + viewPanOffsetY;
       		float rayon = 2.0f * screenDensity;
       		float size = 10.0f * screenDensity;
       		canvas.drawLine(cx, cy - rayon - size, cx, cy - rayon, paint);
       		canvas.drawLine(cx - rayon - size, cy, cx - rayon, cy, paint);
       		canvas.drawLine(cx, cy + rayon + size, cx, cy + rayon, paint);
       		canvas.drawLine(cx + rayon + size, cy, cx + rayon, cy, paint);
       		canvas.drawCircle(cx, cy, rayon, paint);
       	}

		boolean viewHasChanged = viewHasChanged();
		if(viewHasChanged) {
			if(firstTime) {
				firstTime = false;
			} else
				startOSDTimer();
		}

		if(!fillBounds && osdAllowed && showScaleThumbnail
		//&& (viewScaleFactorX > scaleFactorMin || virtualSizeWidth > viewSizeWidth || virtualSizeHeight > viewSizeHeight)
		) {
			// Draw the scale thumbnail
			paint.setColor(scaleThumbnailColor);
			
			float scale = 0.2f;
			if(virtualSizeWidth > virtualSizeHeight)
				scale *= viewSizeWidth / virtualSizeWidth;
			else
				scale *= viewSizeHeight / virtualSizeHeight;
			float marginX = 10f * screenDensity, marginY = 10f * screenDensity;
	    	rectScaleImage.set(viewSizeWidth - marginX - scale * virtualSizeWidth,
    			viewSizeHeight - marginY - scale * virtualSizeHeight,
    			viewSizeWidth - marginX,
    			viewSizeHeight - marginY
	    	);
			canvas.drawRect(rectScaleImage, paint);
    		rectScaleView.set(rectScaleImage.left + scale * (-viewPanOffsetX / viewScaleFactorX),
				rectScaleImage.top + scale * (-viewPanOffsetY / viewScaleFactorY),
				rectScaleImage.left + scale * (viewSizeWidth - viewPanOffsetX) / viewScaleFactorX,
				rectScaleImage.top + scale * (viewSizeHeight - viewPanOffsetY) / viewScaleFactorY
    		);
			canvas.drawRect(rectScaleView, paint);
		}
    }
}
