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
import android.os.SystemClock;
import android.util.Log;

import org.emulator.calculator.usbserial.CustomProber;
import org.emulator.calculator.usbserial.driver.SerialTimeoutException;
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

	private static final int PURGE_TXABORT = 0x0001;
	private static final int PURGE_RXABORT = 0x0002;
	private static final int PURGE_TXCLEAR = 0x0004;
	private static final int PURGE_RXCLEAR = 0x0008;


	private final Handler mainLooper;
	private SerialInputOutputManager usbIoManager;
	private UsbSerialPort usbSerialPort;
	private enum UsbPermission { Unknown, Requested, Granted, Denied }
	private UsbPermission usbPermission = UsbPermission.Unknown;
	private boolean connected = false;
	private String connectionStatus = "";

	private final ArrayDeque<Byte> readByteQueue = new ArrayDeque<>();


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

	public synchronized boolean connect(String serialPort) {
		if(debug) Log.d(TAG, "connect( " + serialPort + ")");

		Pattern patternSerialPort = Pattern.compile("\\\\.\\\\([0-9a-fA-F]+):([0-9a-fA-F]+),(\\d+)");
		Matcher m = patternSerialPort.matcher(serialPort);
		if (m.find()) {
			String vendorIdText = m.group(1);
			String productIdText = m.group(2);
			String portText = m.group(3);
			int vendorId = 0;
			if(vendorIdText != null) {
				try {
					vendorId = Integer.parseInt(vendorIdText, 16);
				} catch (NumberFormatException ex) {
					// Catch bad number format
				}
			}
			int productId = 0;
			if(productIdText != null) {
				try {
					productId = Integer.parseInt(productIdText, 16);
				} catch (NumberFormatException ex) {
					// Catch bad number format
				}
			}
			int portNum = 0;
			if(portText != null) {
				try {
					portNum = Integer.parseInt(portText);
				} catch (NumberFormatException ex) {
					// Catch bad number format
				}
			}
			return connect(vendorId, productId, portNum);
		}
		return false;
	}

	public synchronized String getConnectionStatus() {
		return connectionStatus;
	}

	public synchronized boolean connect(int vendorId, int productId, int portNum) {
		if(debug) Log.d(TAG, String.format("connect('%04X:%04X', portNum: %d)", vendorId, productId, portNum));

		UsbDevice device = null;
		UsbManager usbManager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
		for(UsbDevice v : usbManager.getDeviceList().values())
			if(v.getVendorId() == vendorId && v.getProductId() == productId) {
				device = v;
				break;
			}
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
		UsbDeviceConnection usbConnection;
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
			connectionStatus = "serial_connection_failed_user_has_not_given_permission";
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
					boolean wasEmpty = readByteQueue.isEmpty();
					synchronized (readByteQueue) {
						for (byte datum : data)
							readByteQueue.add(datum);
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
			connectionStatus = "";
			if(debug) Log.d(TAG, "connected!");
			purgeComm(PURGE_TXCLEAR | PURGE_RXCLEAR);
			return true;
		} catch (Exception e) {
			connectionStatus = "serial_connection_failed_open_failed";
			if(debug) Log.d(TAG, "connectionStatus = " + connectionStatus + ", " + e.getMessage());
			disconnect();
		}
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
			//disconnect();
		});
		//disconnect();
	}

	public synchronized boolean setParameters(int baudRate) {
		return setParameters(baudRate, 8, UsbSerialPort.STOPBITS_1, UsbSerialPort.PARITY_NONE);
	}

	public synchronized boolean setParameters(int baudRate, int dataBits, int stopBits, int parity) {
		if(debug) Log.d(TAG, "setParameters(baudRate: " + baudRate + ", dataBits: " + dataBits + ", stopBits: " + stopBits + ", parity: " + parity +")");

		try {
			usbSerialPort.setParameters(baudRate, dataBits, stopBits, parity);
			return true;
		} catch (Exception e) {
			if(debug) Log.d(TAG, "setParameters() failed: " + e.getMessage());
		}
		return false;
	}

	public synchronized byte[] read(int nNumberOfBytesToRead) {
		if(debug) Log.d(TAG, "read(nNumberOfBytesToRead: " + nNumberOfBytesToRead + ")");

		if(!connected)
			return new byte[0];

		byte[] result;
		synchronized (readByteQueue) {
			int nNumberOfReadBytes = Math.min(nNumberOfBytesToRead, readByteQueue.size());
			result = new byte[nNumberOfReadBytes];
			for (int i = 0; i < nNumberOfReadBytes; i++) {
				Byte byteRead = readByteQueue.poll();
				if(byteRead != null)
					result[i] = byteRead;
			}
		}
		if(readByteQueue.size() > 0)
			mainLooper.post(() -> NativeLib.commEvent(serialPortId, NativeLib.EV_RXCHAR));
		return result;
	}

	long maxWritePeriod = 4; //ms
	long lastTime = 0;
	public synchronized int write(byte[] data) {
		if(!connected)
			return 0;

		long currentTime = SystemClock.elapsedRealtime();
		long writePeriod = currentTime - lastTime;

		if(debug) Log.d(TAG, "write(data: [" + data.length + "]: " + Utils.bytesToHex(data) + ") writePeriod: " + writePeriod + "ms");

		if(lastTime > 0 && writePeriod < maxWritePeriod) {
			// Wait 1ms - (currentTime - lastTime)ms
			android.os.SystemClock.sleep(maxWritePeriod - writePeriod);
		}
		lastTime = SystemClock.elapsedRealtime();

		try {
			usbSerialPort.write(data, WRITE_WAIT_MILLIS);
			if(debug) Log.d(TAG, "write() return: " + data.length);
			// No exception, so, all the data has been sent!

			// Consider that the write buffer is empty?
			NativeLib.commEvent(serialPortId, NativeLib.EV_TXEMPTY);

			return data.length;
		} catch (SerialTimeoutException e) {
			if(debug) Log.d(TAG, "write() Exception: " + e.toString());
			return data.length - e.bytesTransferred;
		} catch (Exception e) {
			if(debug) Log.d(TAG, "write() Exception: " + e.toString());
			onReceivedError(e);
		}
		return 0;
	}


	public synchronized int purgeComm(int dwFlags) {
		if(!connected)
			return 0;

		if(debug) Log.d(TAG, "purgeComm(" + dwFlags + ")");

		try {
			boolean purgeWriteBuffers = (dwFlags & PURGE_TXABORT) == PURGE_TXABORT || (dwFlags & PURGE_TXCLEAR) == PURGE_TXCLEAR;
			boolean purgeReadBuffers = (dwFlags & PURGE_RXABORT) == PURGE_RXABORT || (dwFlags & PURGE_RXCLEAR) == PURGE_RXCLEAR;
			if(purgeReadBuffers)
				readByteQueue.clear();
			usbSerialPort.purgeHwBuffers(purgeWriteBuffers, purgeReadBuffers);
			return 1;
		} catch (Exception e) {
			// Exception mean return 0
		}
		return 0;
	}

	public synchronized int setBreak() {
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

	public synchronized int clearBreak() {
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

	public synchronized void disconnect() {
		if(debug) Log.d(TAG, "disconnect()");

		connected = false;
		if(usbIoManager != null)
			usbIoManager.stop();
		usbIoManager = null;
		try {
			usbSerialPort.close();
		} catch (IOException ignored) {

		}
		usbSerialPort = null;
	}
}
