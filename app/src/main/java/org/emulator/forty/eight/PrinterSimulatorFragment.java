package org.emulator.forty.eight;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.ColorStateList;
import android.os.Bundle;
import android.text.Layout;
import android.text.method.ScrollingMovementMethod;
import android.view.GestureDetector;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.Scroller;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.Fragment;
import androidx.viewpager.widget.PagerAdapter;
import androidx.viewpager.widget.ViewPager;


public class PrinterSimulatorFragment extends DialogFragment {

    private PrinterSimulator printer;

    public PrinterSimulatorFragment() {

    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        // Inflate the layout for this fragment
        View view = inflater.inflate(R.layout.fragment_printer_simulator, container, false);

        ViewPager pager = view.findViewById(R.id.viewPagerPrinter);
        pager.setAdapter(new PagerAdapter() {

            @SuppressLint("ClickableViewAccessibility")
            public Object instantiateItem(ViewGroup container, int position) {
                switch (position) {
                    case 0: {
                        LinearLayout linearLayout = container.findViewById(R.id.page_printer_text);
                        final TextView textViewPrinterText = container.findViewById(R.id.printer_text);
                        if(textViewPrinterText != null) {
                            final Context context = getActivity();

//                            final ColorStateList colors = textViewPrinterText.getTextColors();
//                            textViewPrinterText.setTextColor(colors.withAlpha(255));

                            textViewPrinterText.setHorizontallyScrolling(true);

                            // http://stackoverflow.com/a/34316896
                            final Scroller scroller = new Scroller(context);
                            textViewPrinterText.setMovementMethod(new ScrollingMovementMethod());
                            textViewPrinterText.setScroller(scroller);
                            //final Layout textViewResultLayout = textViewPrinterText.getLayout();
                            textViewPrinterText.setOnTouchListener(new View.OnTouchListener() {

                                // Could make this a field member on your activity
                                GestureDetector gesture = new GestureDetector(context, new GestureDetector.SimpleOnGestureListener() {
                                    @Override
                                    public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {

                                        Layout textViewResultLayout = textViewPrinterText.getLayout();
                                        if(textViewResultLayout != null) {
                                            int scrollWidth = (int) textViewResultLayout.getLineWidth(0);
                                            int maxX = scrollWidth - textViewPrinterText.getWidth();
                                            int scrollHeight = textViewResultLayout.getHeight();
                                            //int scrollHeight = textViewPrinterText.getLineCount() * textViewPrinterText.getLineHeight();
                                            int maxY = scrollHeight - textViewPrinterText.getHeight();
                                            scroller.fling(
                                                    textViewPrinterText.getScrollX(), textViewPrinterText.getScrollY(), // int startX, int startY
                                                    (int) -velocityX, (int) -velocityY, // int velocityX, int velocityY,
                                                    0, maxX, // int minX, int maxX
                                                    0, maxY // int minY, int maxY
                                            );
                                        }

                                        return super.onFling(e1, e2, velocityX, velocityY);
                                    }

                                });

                                @Override
                                public boolean onTouch(View v, MotionEvent event) {
                                    gesture.onTouchEvent(event);
                                    return false;
                                }

                            });
                            textViewPrinterText.addOnLayoutChangeListener(new View.OnLayoutChangeListener() {
                                @Override
                                public void onLayoutChange(View v, int left, int top, int right, int bottom, int oldLeft, int oldTop, int oldRight, int oldBottom) {
                                    Layout textViewResultLayout = textViewPrinterText.getLayout();
                                    if(textViewResultLayout != null) {
                                        int scrollWidth = (int) textViewResultLayout.getLineWidth(0);
                                        int maxX = Math.max(0, scrollWidth - textViewPrinterText.getWidth());
                                        int scrollHeight = textViewResultLayout.getHeight();
                                        //int scrollHeight = textViewPrinterText.getLineCount() * textViewPrinterText.getLineHeight();
                                        int maxY = Math.max(0, scrollHeight - textViewPrinterText.getHeight());

                                        int scrollX = textViewPrinterText.getScrollX();
                                        if(scrollX > maxX)
                                            textViewPrinterText.setScrollX(maxX);
                                        int scrollY = textViewPrinterText.getScrollY();
                                        if(scrollY > maxY)
                                            textViewPrinterText.setScrollY(maxY);
                                    }
                                }
                            });
                        }
                        textViewPrinterText.setText(printer.getText());
                        //textViewPrinterText.scrollTo(0, -1);
                        return linearLayout;
                    }
                    case 1: {
                        LinearLayout linearLayout = container.findViewById(R.id.page_printer_graphic);
                        final ImageView imageViewPrinterGraphic = container.findViewById(R.id.printer_graphic);
                        if(imageViewPrinterGraphic != null) {
                            imageViewPrinterGraphic.setImageBitmap(printer.getImage());
                        }
                        return linearLayout;
                    }
                }
                return null;
            }

            @Override
            public CharSequence getPageTitle(int position) {
                // Generate title based on item position
                switch (position) {
                    case 0:
                        return "Text";
                    case 1:
                        return "Graphical";
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

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
    }

    @Override
    public void onDetach() {
        super.onDetach();
    }

    @Override
    public void onResume() {
        super.onResume();
        ViewGroup.LayoutParams params = getDialog().getWindow().getAttributes();
        params.width = ViewGroup.LayoutParams.MATCH_PARENT;
        params.height = ViewGroup.LayoutParams.MATCH_PARENT;
        getDialog().getWindow().setAttributes((android.view.WindowManager.LayoutParams) params);
    }

    public void setPrinter(PrinterSimulator printer) {
        this.printer = printer;
    }

}
