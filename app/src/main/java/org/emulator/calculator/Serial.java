package org.emulator.calculator;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import org.emulator.calculator.usbserial.CustomProber;
import org.emulator.calculator.usbserial.driver.UsbSerialDriver;
import org.emulator.calculator.usbserial.driver.UsbSerialPort;
import org.emulator.calculator.usbserial.driver.UsbSerialProber;
import org.emulator.calculator.usbserial.util.SerialInputOutputManager;

import java.io.IOException;
import java.util.ArrayDeque;
import java.util.Objects;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class Serial {

	private static final String TAG = "Serial";
	private final boolean debug = true;

	private final Context context;
	private final int serialPortId;
	private static final String INTENT_ACTION_GRANT_USB = "EMU48.GRANT_USB";
	private static final int WRITE_WAIT_MILLIS = 2000;

	private final Handler mainLooper;
	private SerialInputOutputManager usbIoManager;
	private UsbSerialPort usbSerialPort;
	private enum UsbPermission { Unknown, Requested, Granted, Denied }
	private UsbPermission usbPermission = UsbPermission.Unknown;
	private boolean connected = false;
	private String connectionStatus = "";

	private final ArrayDeque<Byte> reveivedByteQueue = new ArrayDeque<>();


	public Serial(Context context, int serialPortId) {
		this.context = context;
		this.serialPortId = serialPortId;

		// Will connect at the next try!
		new BroadcastReceiver() {
			@Override
			public void onReceive(Context context, Intent intent) {
				if (intent != null && Objects.equals(intent.getAction(), INTENT_ACTION_GRANT_USB)) {
					usbPermission = intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)
							? UsbPermission.Granted : UsbPermission.Denied;
					// Will connect at the next try!
				}
			}
		};
		mainLooper = new Handler(Looper.getMainLooper());
	}

	public boolean connect(String serialPort) {
		if(debug) Log.d(TAG, "connect( " + serialPort + ")");

		Pattern patternSerialPort = Pattern.compile("\\\\.\\\\(\\d+),(\\d+)");
		Matcher m = patternSerialPort.matcher(serialPort);
		if (m.find()) {
			String deviceText = m.group(1);
			String portText = m.group(2);
			int deviceId = 0;
			try {
				deviceId = Integer.parseInt(deviceText);
			} catch (NumberFormatException ex) {
				// Catch bad number format
			}
			int portNum = 0;
			try {
				portNum = Integer.parseInt(portText);
			} catch (NumberFormatException ex) {
				// Catch bad number format
			}
			return connect(deviceId, portNum);
		}
		return false;
	}

	public String getConnectionStatus() {
		return connectionStatus;
	}

	public boolean connect(int deviceId, int portNum) {
		if(debug) Log.d(TAG, "connect(deviceId: " + deviceId + ", portNum" + portNum + ")");

		UsbDevice device = null;
		UsbManager usbManager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
		for(UsbDevice v : usbManager.getDeviceList().values())
			if(v.getDeviceId() == deviceId)
				device = v;
		if(device == null) {
			connectionStatus = "serial_connection_failed_device_not_found";
			if(debug) Log.d(TAG, "connectionStatus = " + connectionStatus);
			return false;
		}
		UsbSerialDriver driver = UsbSerialProber.getDefaultProber().probeDevice(device);
		if(driver == null) {
			driver = CustomProber.getCustomProber().probeDevice(device);
		}
		if(driver == null) {
			connectionStatus = "serial_connection_failed_no_driver_for_device";
			if(debug) Log.d(TAG, "connectionStatus = " + connectionStatus);
			return false;
		}
		if(driver.getPorts().size() < portNum) {
			connectionStatus = "serial_connection_failed_not_enough_ports_at_device";
			if(debug) Log.d(TAG, "connectionStatus = " + connectionStatus);
			return false;
		}
		usbSerialPort = driver.getPorts().get(portNum);
		UsbDeviceConnection usbConnection = null;
		try {
			usbConnection = usbManager.openDevice(driver.getDevice());
		} catch (SecurityException e) {
			connectionStatus = "serial_connection_failed_user_has_not_given_permission";
			if(debug) Log.d(TAG, "connectionStatus = " + connectionStatus + ", " + e.getMessage());
			return false;
		} catch (Exception e) {
			connectionStatus = "serial_connection_failed_for_unknown_reason";
			if(debug) Log.d(TAG, "connectionStatus = " + connectionStatus + ", " + e.getMessage());
			return false;
		}
		if(usbConnection == null && usbPermission == UsbPermission.Unknown && !usbManager.hasPermission(driver.getDevice())) {
			usbPermission = UsbPermission.Requested;
			PendingIntent usbPermissionIntent = PendingIntent.getBroadcast(context, 0, new Intent(INTENT_ACTION_GRANT_USB), 0);
			usbManager.requestPermission(driver.getDevice(), usbPermissionIntent);
			if(debug) Log.d(TAG, "Request permission");
			return false;
		}
		if(usbConnection == null) {
			if (!usbManager.hasPermission(driver.getDevice()))
				connectionStatus = "serial_connection_failed_permission_denied";
			else
				connectionStatus = "serial_connection_failed_open_device_failed";
			if(debug) Log.d(TAG, "connectionStatus = " + connectionStatus);
			return false;
		}

		try {
			usbSerialPort.open(usbConnection);
			usbIoManager = new SerialInputOutputManager(usbSerialPort, new SerialInputOutputManager.Listener() {
				@Override
				public void onNewData(byte[] data) {
					if(debug) Log.d(TAG, "onNewData: " + Utils.bytesToHex(data));
					boolean wasEmpty = reveivedByteQueue.isEmpty();
					synchronized (reveivedByteQueue) {
						for (byte datum : data)
							reveivedByteQueue.add(datum);
					}
					if (wasEmpty)
						onReceivedByteQueueNotEmpty();
				}

				@Override
				public void onRunError(Exception e) {
					onReceivedError(e);
				}
			});
			Thread usbIo = new Thread(usbIoManager);
			usbIo.setDaemon(true);
			usbIo.start();
			connected = true;
			return true;
		} catch (Exception e) {
			connectionStatus = "serial_connection_failed_open_failed";
			if(debug) Log.d(TAG, "connectionStatus = " + connectionStatus + ", " + e.getMessage());
			disconnect();
		}
		if(debug) Log.d(TAG, "connected!");
		connectionStatus = "";
		return false;
	}

	private void onReceivedByteQueueNotEmpty() {
		NativeLib.commEvent(serialPortId, NativeLib.EV_RXCHAR);
	}


	private void onReceivedError(Exception e) {
		mainLooper.post(() -> {
			if(debug) {
				Log.d(TAG, "onRunError: " + e.getMessage());
				e.printStackTrace();
			}
			disconnect();
		});
	}

	public boolean setParameters(int baudRate) {
		return setParameters(baudRate, 8, UsbSerialPort.STOPBITS_1, UsbSerialPort.PARITY_NONE);
	}

	public boolean setParameters(int baudRate, int dataBits, int stopBits, int parity) {
		if(debug) Log.d(TAG, "setParameters(baudRate: " + baudRate + ", dataBits: " + dataBits + ", stopBits: " + stopBits + ", parity: " + parity +")");

		try {
			usbSerialPort.setParameters(baudRate, dataBits, stopBits, parity);
			return true;
		} catch (Exception e) {
			if(debug) Log.d(TAG, "setParameters() failed: " + e.getMessage());
		}
		return false;
	}

	public byte[] receive(int nNumberOfBytesToRead) {
		byte[] result;
		synchronized (reveivedByteQueue) {
			int nNumberOfReadBytes = Math.min(nNumberOfBytesToRead, reveivedByteQueue.size());
			result = new byte[nNumberOfReadBytes];
			for (int i = 0; i < nNumberOfReadBytes; i++) {
				Byte byteRead = reveivedByteQueue.poll();
				if(byteRead != null)
					result[i] = byteRead;
			}
		}
		if(result == null) {
			result = new byte[0];
		} else if(reveivedByteQueue.size() > 0) {
			mainLooper.post(() -> NativeLib.commEvent(serialPortId, NativeLib.EV_RXCHAR));
		}
		return result;
	}

	public int send(byte[] data) {
		if(!connected)
			return 0;

		try {
			return usbSerialPort.write(data, WRITE_WAIT_MILLIS);
		} catch (Exception e) {
			onReceivedError(e);
		}
		return 0;
	}

	public int setBreak() {
		if(!connected)
			return 0;

		try {
			usbSerialPort.setBreak(true);
			return 1;
		} catch (Exception e) {
			// Exception mean return 0
		}
		return 0;
	}

	public int clearBreak() {
		if(!connected)
			return 0;

		try {
			usbSerialPort.setBreak(false);
			return 1;
		} catch (Exception e) {
			// Exception mean return 0
		}
		return 0;
	}

	public void disconnect() {
		if(debug) Log.d(TAG, "disconnect()");

		connected = false;
		if(usbIoManager != null)
			usbIoManager.stop();
		usbIoManager = null;
		try {
			usbSerialPort.close();
		} catch (IOException ignored) {}
		usbSerialPort = null;
	}
}
