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

import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.os.Bundle;
import android.util.AttributeSet;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.ScrollView;
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
    private boolean debug = false;
    private PrinterSimulator printerSimulator;
    private TextView textViewPrinterText;
    private ScrollView scrollViewPrinterText;
    private PrinterGraphView printerGraphView;

    public PrinterSimulatorFragment() {
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

	    setStyle(AppCompatDialogFragment.STYLE_NO_FRAME, Utils.resId(this, "style", "AppTheme"));
    }

    @NonNull
    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        Dialog dialog = super.onCreateDialog(savedInstanceState);
	    dialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
        return dialog;
    }

    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {

        String title = getString(Utils.resId(this, "string", "dialog_printer_simulator_title"));
        if(printerSimulator != null)
            title = printerSimulator.getTitle();
        Dialog dialog = getDialog();
        if(dialog != null)
            dialog.setTitle(title);


        // Inflate the layout for this fragment
        View view = inflater.inflate(Utils.resId(this, "layout", "fragment_printer_simulator"), container, false);

        // Toolbar

        Toolbar toolbar = view.findViewById(Utils.resId(this, "id", "my_toolbar"));
	    toolbar.setTitle(title);
	    Utils.colorizeDrawableWithColor(requireContext(), toolbar.getNavigationIcon(), android.R.attr.colorForeground);
        toolbar.setNavigationOnClickListener(
                v -> dismiss()
        );
        toolbar.inflateMenu(Utils.resId(this, "menu", "fragment_printer_simulator"));
        toolbar.setOnMenuItemClickListener(item -> {
            if(item.getItemId() == Utils.resId(PrinterSimulatorFragment.this, "id", "menu_printer_simulator_share_text")) {
                Intent intent = new Intent(Intent.ACTION_SEND);
                intent.setType("text/plain");
                intent.putExtra(Intent.EXTRA_SUBJECT, Utils.resId(PrinterSimulatorFragment.this, "string", "message_printer_share_text"));
                intent.putExtra(Intent.EXTRA_TEXT, printerSimulator.getText());
                startActivity(Intent.createChooser(intent, getString(Utils.resId(PrinterSimulatorFragment.this, "string", "message_printer_share_text"))));
            } else if(item.getItemId() == Utils.resId(PrinterSimulatorFragment.this, "id", "menu_printer_simulator_share_graphic")) {
                String imageFilename = "HPPrinter-" + new SimpleDateFormat("yyyy-MM-dd_HH-mm-ss", Locale.US).format(new Date());
                try {
                    Bitmap paperBitmap = printerSimulator.getImage();
                    Bitmap croppedPaperBitmap = Bitmap.createBitmap(paperBitmap.getWidth(), printerSimulator.getPaperHeight(), Bitmap.Config.ARGB_8888);
                    Canvas canvas = new Canvas(croppedPaperBitmap);
                    Paint paint = new Paint();
	                paint.setAntiAlias(false);
	                paint.setFilterBitmap(false);
                    canvas.drawBitmap(paperBitmap, 0, 0, paint);

                    Activity activity = getActivity();
                    if(activity != null) {
                        File storagePath = new File(activity.getExternalCacheDir(), "");
                        File imageFile = File.createTempFile(imageFilename, ".png", storagePath);
                        FileOutputStream fileOutputStream = new FileOutputStream(imageFile);
                        croppedPaperBitmap.compress(Bitmap.CompressFormat.PNG, 90, fileOutputStream);
                        fileOutputStream.close();

                        String subject = getString(Utils.resId(PrinterSimulatorFragment.this, "string", "message_printer_share_graphic"));
	                    Intent intent = new Intent(android.content.Intent.ACTION_SEND);
	                    intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
	                    intent.putExtra(Intent.EXTRA_SUBJECT, subject);
	                    intent.putExtra(Intent.EXTRA_TITLE, subject);
	                    intent.setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
	                    intent.putExtra(Intent.EXTRA_STREAM, FileProvider.getUriForFile(getActivity(), getActivity().getPackageName() + ".provider", imageFile));
	                    intent.setType("image/png");
//	                    intent.setDataAndType(FileProvider.getUriForFile(getActivity(), getActivity().getPackageName() + ".provider", imageFile), "image/png");
                        startActivity(Intent.createChooser(intent, getString(Utils.resId(PrinterSimulatorFragment.this, "string", "message_printer_share_graphic"))));
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                    Utils.showAlert(getActivity(), e.getMessage());
                }
            } else if(item.getItemId() == Utils.resId(PrinterSimulatorFragment.this, "id", "menu_printer_simulator_change_paper")) {
                printerSimulator.changePaper();
                printerGraphView.updatePaperLayout();
                textViewPrinterText.setText("");
                if(printerGraphView != null) {
                    printerGraphView.updatePaperLayout();
                    printerGraphView.invalidate();
                }
            }
            return true;
        });
        setMenuVisibility(true);

        // ViewPager with Text and Graph

        ViewPager pager = view.findViewById(Utils.resId(this, "id", "viewPagerPrinter"));
        pager.setAdapter(new PagerAdapter() {

            @NonNull
            public Object instantiateItem(@NonNull ViewGroup container, int position) {
                if (position == 0) {
                    ViewGroup layoutPagePrinterText = container.findViewById(Utils.resId(PrinterSimulatorFragment.this, "id", "page_printer_text"));
                    textViewPrinterText = container.findViewById(Utils.resId(PrinterSimulatorFragment.this, "id", "printer_text"));
                    scrollViewPrinterText = container.findViewById(Utils.resId(PrinterSimulatorFragment.this, "id", "printer_text_scroll"));
                    updatePaper(printerSimulator.getText());
                    return layoutPagePrinterText;
                } else {
                    ViewGroup layoutPagePrinterGraphic = container.findViewById(Utils.resId(PrinterSimulatorFragment.this, "id", "page_printer_graphic"));
                    ViewGroup pagePrinterGraphicContainer = container.findViewById(Utils.resId(PrinterSimulatorFragment.this, "id", "printer_graphic_container"));
                    printerGraphView = new PrinterGraphView(getActivity());
                    printerGraphView.setBitmap(printerSimulator.getImage());
                    pagePrinterGraphicContainer.addView(printerGraphView);
                    return layoutPagePrinterGraphic;
                }
            }

            @Override
            public CharSequence getPageTitle(int position) {
                // Generate title based on item position
                switch (position) {
                    case 0:
                        return getString(Utils.resId(PrinterSimulatorFragment.this, "string", "tab_printer_textual"));
                    case 1:
                        return getString(Utils.resId(PrinterSimulatorFragment.this, "string", "tab_printer_graphical"));
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

    private void updatePaper(String textAppended) {
        if(debug) Log.d(TAG, "updatePaper(" + textAppended + ")");
        if(textViewPrinterText != null) {
            textViewPrinterText.append(textAppended);
            scrollViewPrinterText.fullScroll(View.FOCUS_DOWN);
        }
        if(printerGraphView != null) {
            printerGraphView.updatePaperLayout();
            printerGraphView.invalidate();
        }
    }

    public void setPrinterSimulator(PrinterSimulator printerSimulator) {
        this.printerSimulator = printerSimulator;
        this.printerSimulator.setOnPrinterUpdateListener(textAppended -> {
            Activity activity = getActivity();
            if(activity != null)
                activity.runOnUiThread(() -> {
                    if(debug) Log.d(TAG, "onPrinterUpdate(" + textAppended + ")");
                    updatePaper(textAppended);
                });
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
            setShowScaleThumbnail(true);
            scaleThumbnailColor = Color.GRAY;

	        paintBitmap.setAntiAlias(false);
	        paintBitmap.setFilterBitmap(false);
        }

        public void setBitmap(Bitmap bitmap) {
            this.bitmap = bitmap;
        }

        public void updateLayoutView() {
            if(bitmap != null && virtualSizeHeight > 1 && bitmap.getWidth() > 0.0f && getWidth() > 0.0f) {
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
            }
        }

        protected void constrainPan(boolean center) {

            // Keep the panning limits and the image centered.
            float viewWidth = viewSizeWidth;
            float viewHeight = viewSizeHeight;
            if(viewWidth == 0.0f){
                viewWidth = 1.0f;
                viewHeight = 1.0f;
            }
            float virtualWidth = virtualSizeWidth;
            float virtualHeight = virtualSizeHeight;
            if(virtualWidth == 0.0f){
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

                if(/*center &&*/ viewPanMinY > 0f)
                    // Keep the image at the bottom of the view.
                    viewPanOffsetY = viewPanMinY; // / 2.0f;
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

        public void updatePaperLayout() {
            int paperHeight = printerSimulator.getPaperHeight();
            setEnablePanAndScale(paperHeight > 1);
            setVirtualSize(bitmap.getWidth(), paperHeight);
            updateLayoutView();
        }

        @Override
        protected void onSizeChanged(int viewWidth, int viewHeight, int oldViewWidth, int oldViewHeight) {
            viewSizeWidth = viewWidth;
            viewSizeHeight = viewHeight;
            updatePaperLayout();
        }

        @Override
        protected void onCustomDraw(Canvas canvas) {
            canvas.drawColor(Color.BLACK);
            Rect paperclip = new Rect(0, 0, bitmap.getWidth(), printerSimulator.getPaperHeight());
            canvas.drawBitmap(this.bitmap, paperclip, paperclip, paintBitmap);
        }
    }
}
