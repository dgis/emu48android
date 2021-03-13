package org.emulator.calculator.usbserial;

import android.content.Context;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.fragment.app.ListFragment;

import org.emulator.calculator.Utils;
import org.emulator.calculator.usbserial.driver.UsbSerialDriver;
import org.emulator.calculator.usbserial.driver.UsbSerialProber;

import java.util.ArrayList;
import java.util.Locale;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class DevicesFragment extends ListFragment {

    static class ListItem {
        UsbDevice device;
        int port;
        UsbSerialDriver driver;

        ListItem(UsbDevice device, int port, UsbSerialDriver driver) {
            this.device = device;
            this.port = port;
            this.driver = driver;
        }
    }

    private final ArrayList<ListItem> listItems = new ArrayList<>();
    private ArrayAdapter<ListItem> listAdapter;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setHasOptionsMenu(true);
        listAdapter = new ArrayAdapter<ListItem>(getActivity(), 0, listItems) {
            @NonNull
            @Override
            public View getView(int position, View view, @NonNull ViewGroup parent) {
                ListItem item = listItems.get(position);
                if (view == null)
                    view = getActivity().getLayoutInflater().inflate(Utils.resId(DevicesFragment.this, "layout", "device_list_item"), parent, false);
                TextView text1 = view.findViewById(Utils.resId(DevicesFragment.this, "id", "text1"));
                TextView text2 = view.findViewById(Utils.resId(DevicesFragment.this, "id", "text2"));
                if(item.driver == null)
                    text1.setText(getString(Utils.resId(DevicesFragment.this, "string", "serial_ports_device_no_driver")));
                else {
                	String deviceName = item.driver.getClass().getSimpleName().replace("SerialDriver","");
                	if(item.driver.getPorts().size() == 1)
		                text1.setText(deviceName);
	                else
		                text1.setText(String.format(Locale.US, getString(Utils.resId(DevicesFragment.this, "string", "serial_ports_device_item_title")), deviceName, item.port));
                }
                text2.setText(String.format(Locale.US, getString(Utils.resId(DevicesFragment.this, "string", "serial_ports_device_item_description")), item.device.getVendorId(), item.device.getProductId()));
                return view;
            }
        };
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        setListAdapter(null);
        View header = getActivity().getLayoutInflater().inflate(Utils.resId(DevicesFragment.this, "layout", "device_list_header"), null, false);
        getListView().addHeaderView(header, null, false);
        setEmptyText(getString(Utils.resId(this, "string", "serial_no_device")));
        ((TextView) getListView().getEmptyView()).setTextSize(18);
        setListAdapter(listAdapter);
    }

    @Override
    public void onResume() {
        super.onResume();
        refresh();
    }

    public void refresh() {
        UsbManager usbManager = (UsbManager) getActivity().getSystemService(Context.USB_SERVICE);
        UsbSerialProber usbDefaultProber = UsbSerialProber.getDefaultProber();
        UsbSerialProber usbCustomProber = CustomProber.getCustomProber();
        listItems.clear();
        for(UsbDevice device : usbManager.getDeviceList().values()) {
            UsbSerialDriver driver = usbDefaultProber.probeDevice(device);
            if(driver == null) {
                driver = usbCustomProber.probeDevice(device);
            }
            if(driver != null) {
                for(int port = 0; port < driver.getPorts().size(); port++)
                    listItems.add(new ListItem(device, port, driver));
            } else {
                listItems.add(new ListItem(device, 0, null));
            }
        }
        listAdapter.notifyDataSetChanged();
    }

	public static class SerialConnectParameters {
		public int deviceId;
		public int port;
		public int vendorId;
		public int productId;
		public String modelName;

		public SerialConnectParameters() {
			this.deviceId = 0;
			this.port = 0;
			this.vendorId = 0;
			this.productId = 0;
			this.modelName = "";
		}
		public SerialConnectParameters(int device, int port, int vendorId, int productId, String modelName) {
			this.deviceId = device;
			this.port = port;
			this.vendorId = vendorId;
			this.productId = productId;
			this.modelName = modelName;
		}

		public String toSettingsString() {
			return String.format(Locale.US, "%d,%d,%d,%d,%s", deviceId, port, vendorId, productId, modelName);
		}

		public String toWin32String() {
			return String.format(Locale.US, "%d,%d", deviceId, port);
		}

		public String toDisplayString(Context context) {
			if(deviceId == 0 && port == 0)
				return context.getResources().getString(Utils.resId(context, "string", "serial_ports_device_no_driver"));
			else
				return String.format(Locale.US, context.getResources().getString(Utils.resId(context, "string", "serial_ports_device")), modelName, vendorId, productId, deviceId, port);
		}

		public static SerialConnectParameters fromSettingsString(String serialPorts) {
			SerialConnectParameters serialConnectParameters = new SerialConnectParameters();
			serialConnectParameters.fromSettingsStringExtractor(serialPorts);
			return serialConnectParameters;
		}

		private void fromSettingsStringExtractor(String serialPorts) {
			Pattern patternSerialPort = Pattern.compile("(\\d+),(\\d+),(\\d+),(\\d+),([^,]+)");
			Matcher m = patternSerialPort.matcher(serialPorts);
			if (m.find()) {
				String deviceText = m.group(1);
				String portText = m.group(2);
				String vendorIdText = m.group(3);
				String productIdText = m.group(4);
				modelName = m.group(5);
				try {
					deviceId = Integer.parseInt(deviceText);
				} catch (NumberFormatException ex) {
					// Catch bad number format
				}
				try {
					port = Integer.parseInt(portText);
				} catch (NumberFormatException ex) {
					// Catch bad number format
				}
				try {
					vendorId = Integer.parseInt(vendorIdText);
				} catch (NumberFormatException ex) {
					// Catch bad number format
				}
				try {
					productId = Integer.parseInt(productIdText);
				} catch (NumberFormatException ex) {
					// Catch bad number format
				}
			}
		}
	}

	/**
	 * Interface definition for a callback to be invoked when the printer just has print something.
	 */
	public interface OnSerialDeviceClickedListener {
		/**
		 * Called when the printer just has print something.
		 */
		void onSerialDeviceClicked(SerialConnectParameters serialConnectParameters);
	}

	private OnSerialDeviceClickedListener onSerialDeviceClickedListener;

	/**
	 * Register a callback to be invoked when a serial device has been chosen.
	 * @param onSerialDeviceClickedListener The callback that will run
	 */
	void setOnSerialDeviceClickedListener(OnSerialDeviceClickedListener onSerialDeviceClickedListener) {
		this.onSerialDeviceClickedListener = onSerialDeviceClickedListener;
	}

	@Override
    public void onListItemClick(@NonNull ListView l, @NonNull View v, int position, long id) {
        ListItem item = listItems.get(position-1);
        if(item.driver == null) {
            Toast.makeText(getActivity(), getString(Utils.resId(this, "string", "serial_ports_device_no_driver")), Toast.LENGTH_SHORT).show();
	        if(onSerialDeviceClickedListener != null)
		        onSerialDeviceClickedListener.onSerialDeviceClicked(new SerialConnectParameters(0, 0,
				        0, 0,
				        ""));
        } else if(item.device != null) {
        	if(onSerialDeviceClickedListener != null)
		        onSerialDeviceClickedListener.onSerialDeviceClicked(new SerialConnectParameters(item.device.getDeviceId(), item.port,
				        item.device.getVendorId(), item.device.getProductId(),
				        item.driver.getClass().getSimpleName().replace("SerialDriver","")));
        }
    }

}
