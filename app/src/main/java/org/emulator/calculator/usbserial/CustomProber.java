package org.emulator.calculator.usbserial;

import org.emulator.calculator.usbserial.driver.CdcAcmSerialDriver;
import org.emulator.calculator.usbserial.driver.ProbeTable;
import org.emulator.calculator.usbserial.driver.UsbSerialProber;

/**
 * add devices here, that are not known to DefaultProber
 *
 * if the App should auto start for these devices, also
 * add IDs to app/src/main/res/xml/device_filter.xml
 */
public class CustomProber {

	public static UsbSerialProber getCustomProber() {
        ProbeTable customTable = new ProbeTable();
        customTable.addProduct(0x16d0, 0x087e, CdcAcmSerialDriver.class); // e.g. Digispark CDC
        return new UsbSerialProber(customTable);
    }

}
