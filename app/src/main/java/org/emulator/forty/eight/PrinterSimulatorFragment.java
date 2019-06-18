package org.emulator.forty.eight;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.Paint;
import android.os.Bundle;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatDialogFragment;
import androidx.appcompat.widget.Toolbar;
import androidx.core.content.FileProvider;
import androidx.viewpager.widget.PagerAdapter;
import androidx.viewpager.widget.ViewPager;

import java.io.File;
import java.io.FileOutputStream;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;


public class PrinterSimulatorFragment extends AppCompatDialogFragment {

    private static final String TAG = "PrinterSimulator";
    private PrinterSimulator printerSimulator;
    private Toolbar toolbar;
    private TextView textViewPrinterText;
    private PrinterGraphView printerGraphView;

    ColorMatrixColorFilter colorFilterAlpha8ToRGB = new ColorMatrixColorFilter(new float[]{
            0, 0, 0, 1, 0,
            0, 0, 0, 1, 0,
            0, 0, 0, 1, 0,
            0, 0, 0, 0, 255
    });

    public PrinterSimulatorFragment() {

    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        //setStyle(STYLE_NO_FRAME, android.R.style.Theme_Holo_Light);
        //setStyle(STYLE_NO_FRAME, 0);
        setStyle(STYLE_NO_TITLE, android.R.style.Theme_Holo_Light);
        //setStyle(STYLE_NO_TITLE, 0);
    }
//    @Override
//    public void onResume() {
//        ViewGroup.LayoutParams params = getDialog().getWindow().getAttributes();
//        params.width = WindowManager.LayoutParams.MATCH_PARENT;
//        params.height = ViewGroup.LayoutParams.MATCH_PARENT;
//        getDialog().getWindow().setAttributes((WindowManager.LayoutParams) params);
//        //getDialog().getWindow().setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
//        super.onResume();
//    }


    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        Dialog dialog = super.onCreateDialog(savedInstanceState);
        dialog.getWindow().requestFeature(Window.FEATURE_NO_TITLE);
        return dialog;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {

        String title = getString(R.string.dialog_printer_simulator_title);
        getDialog().setTitle(title);


        // Inflate the layout for this fragment
        View view = inflater.inflate(R.layout.fragment_printer_simulator, container, false);

        // Toolbar

        toolbar = view.findViewById(R.id.my_toolbar);
        toolbar.setTitle(title);
        //toolbar.setLogo(R.drawable.ic_launcher);
        toolbar.setNavigationIcon(R.drawable.ic_keyboard_backspace_white_24dp);
        toolbar.setNavigationOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        dismiss();
                    }
                }
        );
        toolbar.inflateMenu(R.menu.fragment_printer_simulator);
        toolbar.setOnMenuItemClickListener(new Toolbar.OnMenuItemClickListener() {
            @Override
            public boolean onMenuItemClick(MenuItem item) {
                if(item.getItemId() == R.id.menu_printer_simulator_share_text) {
                    Intent intent = new Intent(android.content.Intent.ACTION_SEND);
                    intent.setType("text/plain");
                    //intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    intent.putExtra(Intent.EXTRA_SUBJECT, R.string.message_printer_share_text);
                    intent.putExtra(Intent.EXTRA_TEXT, printerSimulator.getText());
//                    intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
//                    intent.putExtra(Intent.EXTRA_STREAM, FileProvider.getUriForFile(getActivity(),getActivity().getPackageName() + ".provider", imageFile));
                    startActivity(Intent.createChooser(intent, getString(R.string.message_printer_share_text)));
                } else if(item.getItemId() == R.id.menu_printer_simulator_share_graphic) {
                    String imageFilename = "HPPrinter-" + new SimpleDateFormat("yyyy-MM-dd_HH-mm-ss", Locale.US).format(new Date());
                    try {
                        Bitmap paperBitmap = printerSimulator.getImage();
                        Bitmap croppedPaperBitmap = Bitmap.createBitmap(paperBitmap.getWidth(), printerSimulator.getPaperHeight(), Bitmap.Config.ARGB_8888);
                        //convertedBitmap.eraseColor(0xFFFFFFFF);
                        Canvas canvas = new Canvas(croppedPaperBitmap);
                        Paint paint = new Paint();
                        paint.setColorFilter(colorFilterAlpha8ToRGB);
                        canvas.drawBitmap(paperBitmap, 0, 0, paint);

                        File storagePath = new File(getActivity().getExternalCacheDir(), "");
                        File imageFile = File.createTempFile(imageFilename, ".png", storagePath);
                        FileOutputStream fileOutputStream = new FileOutputStream(imageFile);
                        croppedPaperBitmap.compress(Bitmap.CompressFormat.PNG, 90, fileOutputStream);
                        fileOutputStream.close();
                        String mimeType = "application/png";
                        Intent intent = new Intent(android.content.Intent.ACTION_SEND);
                        intent.setType(mimeType);
                        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                        intent.putExtra(Intent.EXTRA_SUBJECT, R.string.message_printer_share_graphic);
                        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                        intent.putExtra(Intent.EXTRA_STREAM, FileProvider.getUriForFile(getActivity(),getActivity().getPackageName() + ".provider", imageFile));
                        startActivity(Intent.createChooser(intent, getString(R.string.message_printer_share_graphic)));
                    } catch (Exception e) {
                        e.printStackTrace();
                        ((MainActivity)getActivity()).showAlert(e.getMessage());
                    }

                }
                return true;
            }
        });
        TypedValue tv = new TypedValue();
        Context context = getContext();
        if(context != null) {
            Resources.Theme theme = context.getTheme();
            if (theme != null) {
                //theme.resolveAttribute(android.R.attr.actionBarSize, tv, true);
                theme.resolveAttribute(androidx.appcompat.R.attr.actionBarSize, tv, true);
                int actionBarHeight = getResources().getDimensionPixelSize(tv.resourceId);
                toolbar.setMinimumHeight(actionBarHeight);
            }
        }
        setMenuVisibility(true);

        // ViewPager with Text and Graph

        ViewPager pager = view.findViewById(R.id.viewPagerPrinter);
        pager.setAdapter(new PagerAdapter() {

            @SuppressLint("ClickableViewAccessibility")
            public Object instantiateItem(ViewGroup container, int position) {
                switch (position) {
                    case 0: {
                        ViewGroup layoutPagePrinterText = container.findViewById(R.id.page_printer_text);
                        textViewPrinterText = container.findViewById(R.id.printer_text);
//                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
//                            textViewPrinterText.setOnScrollChangeListener(new View.OnScrollChangeListener() {
//                                @Override
//                                public void onScrollChange(View v, int scrollX, int scrollY, int oldScrollX, int oldScrollY) {
//                                    int virtualHeight = -1;
//                                    final Layout layout = textViewPrinterText.getLayout();
//                                    if(layout != null) {
//                                        virtualHeight = layout.getLineTop(textViewPrinterText.getLineCount());
//                                    }
//
//                                    Log.d(TAG, "onScrollChange() getScrollY: " + textViewPrinterText.getScrollY() + ", getHeight: " + textViewPrinterText.getHeight() + ", virtualHeight: " + virtualHeight);
//
//                                }
//                            });
//                        }

//                        if(textViewPrinterText != null) {
//                            textViewPrinterText.setTextIsSelectable(true);
//                            final Context context = getActivity();
//
////                            final ColorStateList colors = textViewPrinterText.getTextColors();
////                            textViewPrinterText.setTextColor(colors.withAlpha(255));
//
//                            textViewPrinterText.setHorizontallyScrolling(true);
//
//                            // http://stackoverflow.com/a/34316896
//                            final Scroller scroller = new Scroller(context);
//                            textViewPrinterText.setMovementMethod(new ScrollingMovementMethod());
//                            textViewPrinterText.setScroller(scroller);
//                            //final Layout textViewResultLayout = textViewPrinterText.getLayout();
//                            textViewPrinterText.setOnTouchListener(new View.OnTouchListener() {
//
//                                // Could make this a field member on your activity
//                                GestureDetector gesture = new GestureDetector(context, new GestureDetector.SimpleOnGestureListener() {
//                                    @Override
//                                    public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
//
//                                        Layout textViewResultLayout = textViewPrinterText.getLayout();
//                                        if(textViewResultLayout != null) {
//                                            int scrollWidth = (int) textViewResultLayout.getLineWidth(0);
//                                            int maxX = scrollWidth - textViewPrinterText.getWidth();
//                                            int scrollHeight = textViewResultLayout.getHeight();
//                                            //int scrollHeight = textViewPrinterText.getLineCount() * textViewPrinterText.getLineHeight();
//                                            int maxY = scrollHeight - textViewPrinterText.getHeight();
//                                            scroller.fling(
//                                                    textViewPrinterText.getScrollX(), textViewPrinterText.getScrollY(), // int startX, int startY
//                                                    (int) -velocityX, (int) -velocityY, // int velocityX, int velocityY,
//                                                    0, maxX, // int minX, int maxX
//                                                    0, maxY // int minY, int maxY
//                                            );
//                                        }
//
//                                        return super.onFling(e1, e2, velocityX, velocityY);
//                                    }
//
//                                });
//
//                                @Override
//                                public boolean onTouch(View v, MotionEvent event) {
//                                    gesture.onTouchEvent(event);
//                                    return false;
//                                }
//
//                            });
//                            textViewPrinterText.addOnLayoutChangeListener(new View.OnLayoutChangeListener() {
//                                @Override
//                                public void onLayoutChange(View v, int left, int top, int right, int bottom, int oldLeft, int oldTop, int oldRight, int oldBottom) {
//                                    Layout textViewResultLayout = textViewPrinterText.getLayout();
//                                    if(textViewResultLayout != null) {
//                                        int scrollWidth = (int) textViewResultLayout.getLineWidth(0);
//                                        int maxX = Math.max(0, scrollWidth - textViewPrinterText.getWidth());
//                                        int scrollHeight = textViewResultLayout.getHeight();
//                                        //int scrollHeight = textViewPrinterText.getLineCount() * textViewPrinterText.getLineHeight();
//                                        int maxY = Math.max(0, scrollHeight - textViewPrinterText.getHeight());
//
//                                        int scrollX = textViewPrinterText.getScrollX();
//                                        if(scrollX > maxX)
//                                            textViewPrinterText.setScrollX(maxX);
//                                        int scrollY = textViewPrinterText.getScrollY();
//                                        if(scrollY > maxY)
//                                            textViewPrinterText.setScrollY(maxY);
//                                    }
//                                }
//                            });
//                        }
                        addToPrinterText(printerSimulator.getText());
                        return layoutPagePrinterText;
                    }
                    case 1: {
                        ViewGroup layoutPagePrinterGraphic = container.findViewById(R.id.page_printer_graphic);
                        ViewGroup pagePrinterGraphicContainer = container.findViewById(R.id.printer_graphic_container);
                        printerGraphView = new PrinterGraphView(getActivity());
                        printerGraphView.setBitmap(printerSimulator.getImage());
                        pagePrinterGraphicContainer.addView(printerGraphView);
                        return layoutPagePrinterGraphic;
                    }
                }
                return null;
            }

            @Override
            public CharSequence getPageTitle(int position) {
                // Generate title based on item position
                switch (position) {
                    case 0:
                        return getString(R.string.tab_printer_textual);
                    case 1:
                        return getString(R.string.tab_printer_graphical);
                    default:
                        return null;
                }
            }

            @Override
            public int getCount() {
                return 2;
            }

            @Override
            public boolean isViewFromObject(@NonNull View view, @NonNull Object object) {
                return view == object;
            }
        });

        return view;
    }

//    @Override
//    public void onAttach(Context context) {
//        super.onAttach(context);
//    }
//
//    @Override
//    public void onDetach() {
//        super.onDetach();
//    }

//    @Override
//    public void onResume() {
//        super.onResume();
//        ViewGroup.LayoutParams params = getDialog().getWindow().getAttributes();
//        params.width = ViewGroup.LayoutParams.MATCH_PARENT;
//        params.height = ViewGroup.LayoutParams.MATCH_PARENT;
//        getDialog().getWindow().setAttributes((android.view.WindowManager.LayoutParams) params);
//    }

    private void addToPrinterText(String text) {
        if(textViewPrinterText != null) {
//            Log.d(TAG, "addToPrinterText() getScrollY: " + textViewPrinterText.getScrollY() + ", getHeight: " + textViewPrinterText.getHeight());

//            boolean needToScroll = false;
//            Layout layout = textViewPrinterText.getLayout();
//            if(layout != null) {
//                int currentScrollPosition = textViewPrinterText.getScrollY();
//                int maxScrollPosition = layout.getLineTop(textViewPrinterText.getLineCount()) - textViewPrinterText.getHeight();
//                needToScroll = currentScrollPosition == maxScrollPosition;
//            }
            textViewPrinterText.setText(text);
//            layout = textViewPrinterText.getLayout();
//            if(layout != null) {
//                int virtualHeight = layout.getLineTop(textViewPrinterText.getLineCount());
//                int newScrollPosition = virtualHeight - textViewPrinterText.getHeight();
//                if (needToScroll && newScrollPosition > 0) {
//                    Log.d(TAG, "addToPrinterText() NEED to scroll to: " + newScrollPosition);
//                    textViewPrinterText.scrollTo(0, newScrollPosition);
//                }
//            }
        }
    }

    Runnable onPrinterUpdate = new Runnable() {
        @Override
        public void run() {
            if(textViewPrinterText != null) {
                addToPrinterText(printerSimulator.getText());
            }
            if(printerGraphView != null) {
                printerGraphView.setVirtualSize(printerSimulator.getImage().getWidth(), printerSimulator.getPaperHeight());
                printerGraphView.updateLayoutView();
                printerGraphView.invalidate();
            }
        }
    };

    public void setPrinterSimulator(final PrinterSimulator printerSimulator) {
        this.printerSimulator = printerSimulator;
        this.printerSimulator.setOnPrinterUpdateListener(new PrinterSimulator.OnPrinterUpdateListener() {
            @Override
            public void onPrinterUpdate() {
                Activity activity = getActivity();
                if(activity != null)
                    activity.runOnUiThread(onPrinterUpdate);
            }
        });
    }


    class PrinterGraphView extends PanAndScaleView {

        Paint paintBitmap = new Paint();
        Bitmap bitmap;

        public PrinterGraphView(Context context) {
            super(context);
            commonInitialization();
        }

        public PrinterGraphView(Context context, AttributeSet attrs) {
            super(context, attrs);
            commonInitialization();
        }

        private void commonInitialization() {
            //setLayerType(View.LAYER_TYPE_SOFTWARE, null);
            setShowScaleThumbnail(true);
            scaleThumbnailColor = Color.RED;
            //setWillNotDraw(false);
        }

        public void setBitmap(Bitmap bitmap) {
            this.bitmap = bitmap;
        }

        public void updateLayoutView() {
            if(bitmap != null && virtualSizeHeight > 1) {
                if (bitmap.getWidth() > 0.0f && getWidth() > 0.0f) {
                    float translateX, translateY, scale;
                    scale = viewSizeWidth / virtualSizeWidth;
                    translateX = 0;
                    translateY = viewSizeHeight - scale * virtualSizeHeight;

                    viewScaleFactorX = scale;
                    viewScaleFactorY = scale;
                    scaleFactorMin = scale;
                    scaleFactorMax = maxZoom * scaleFactorMin;
                    viewPanOffsetX = translateX;
                    viewPanOffsetY = translateY;

                    constrainPan();
                    return;
                }
                // Else, the screens orientations are the same, so we set the calculator in fullscreen
                resetViewport();
            }
        }

        @Override
        protected void onSizeChanged(int viewWidth, int viewHeight, int oldViewWidth, int oldViewHeight) {
            //super.onSizeChanged(viewWidth, viewHeight, oldViewWidth, oldViewHeight);

            viewSizeWidth = viewWidth;
            viewSizeHeight = viewHeight;

            setVirtualSize(bitmap.getWidth(), printerSimulator.getPaperHeight());
            updateLayoutView();
        }

        @Override
        protected void onCustomDraw(Canvas canvas) {
            paintBitmap.setColorFilter(colorFilterAlpha8ToRGB);
            canvas.drawColor(Color.BLACK); // LTGRAY);
            canvas.drawBitmap(this.bitmap, 0, 0, paintBitmap);
        }
    }
}
