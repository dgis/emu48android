package org.emulator.forty.eight;

import java.util.ArrayList;

/*
 * Based on the free HP82240B Printer Simulator by Christoph Giesselink
 */
public class PrinterSimulator {

    private ArrayList<Integer> data = new ArrayList<>();
    private StringBuilder m_Text = new StringBuilder("");


    private final int ESC = 27;							// ESC

    private final byte PM_HP82240A = 0;
    private final byte PM_HP82240B = 1;
    private final byte PM_GenericSerial = 2;

    private byte m_ePrinterModel;// selected printer model
    private boolean m_bExpChar; // printing expanded characters
    private boolean m_bUnderLined; // printing underlined characters
    private boolean m_bEcma94; // Roman 8 / ECMA 94 character set

    private boolean m_bEsc; // not ESC sequence
    private int m_byGraphLength; // remaining no. of graphic bytes

    public PrinterSimulator() {
        m_ePrinterModel = PM_HP82240B;			// HP82240B printer

        reset();								// reset printer state machine
    }

    //
    // reset printer state machine
    //
    void reset() {
        m_bExpChar = false;						// printing normal-width characters
        m_bUnderLined = false;					// printing non underlined characters

        // Roman 8 character set for HP82240A and HP82240B, ECMA 94 for serial printer type
        m_bEcma94 = (m_ePrinterModel == PM_GenericSerial);

        m_bEsc = false;							// not ESC sequence
        m_byGraphLength = 0;					// no remaining graphic bytes
    }

    //
    // printer selftest
    //
    void selftest() {
        // self test normally run in an endless loop, that's very hard to implement,
        // so this implementation printing all chracters only one time and then
        // go back to the communication interface

        int i,nLF;

        reset();								// reset printer state machine

        nLF = 1;
        write((byte)4);				    		// begin with special LF
        write((byte)95);						// '_' instead if ' ' as first character
        for (i = 33; i <= 256; ++i) {			// all ROMAN8 characters
            if (nLF == 0) write((byte)4);		// LF after 24 characters printing
            write((byte)i);						// print character
            nLF = ++nLF % 24;
        }

        // post code, identification number: HP82240A = D, HP82240B = G
        write((byte)(m_ePrinterModel == PM_HP82240A ? 'D' : 'G'));

        // rest of post code is battery state full
        String szPostPrt = "\u0004\u0004BAT: 5\u0004\u0004";

        for (int ci = 0; ci < szPostPrt.length(); ci++) { 	// write post print
            char c = szPostPrt.charAt(ci);
            write((byte)c);
        }
    }

    public synchronized void write(int byData) {
        data.add(byData);

        do {
            // check for begin of ESC sequence
            if (byData == ESC && !m_bEsc && m_byGraphLength == 0) {
                m_bEsc = true;					// ESC sequence mode
                break;
            }

            if (m_bEsc) {						// byte is ESC command
                switch (byData) {
                    case 255: // reset
                        reset();
                        break;
                    case 254: // start self test
                        selftest();
                        break;
                    case 253: // start printing expanded characters
                        m_bExpChar = true;
                        break;
                    case 252: // return to normal-width characters
                        m_bExpChar = false;
                        break;
                    case 251: // start printing underlined characters
                        m_bUnderLined = true;
                        break;
                    case 250: // stop underlining
                        m_bUnderLined = false;
                        break;
                    case 249: // ECMA 94 character set
                        if (m_ePrinterModel == PM_HP82240B)
                            m_bEcma94 = true;
                        break;
                    case 248: // Roman 8 character set
                        if (m_ePrinterModel == PM_HP82240B)
                            m_bEcma94 = false;
                        break;
                    default:
                        // graphic data
                        if (byData >= 1 && byData <= 166) {
                            // remaining graphic bytes
                            m_byGraphLength = byData;
                        }
                }
                m_bEsc = false;
                break;
            }

            // normal character
            if (m_byGraphLength == 0) { 	    // not a graphic character
                m_Text.append((char)byData);    // output to text window
            }

            // output to graphic window
            //m_Graph.WriteChar(byData,m_byGraphLength > 0);

            if (m_byGraphLength > 0) {  		// in graphic mode
                --m_byGraphLength;				// graphic character printed
            }
        } while (false);
    }

    String getText() {
        return m_Text.toString();
    }
}
