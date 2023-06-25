/*
 *   apple.c
 *
 *   This file is part of Emu48
 *
 *   Copyright (C) 2005 CdB for HP
 *   Copyright (C) 2006 Christoph Gie�elink
 *
 */
#include "pch.h"
#include "Emu48.h"
#include "Opcodes.h"
#include "apple.h"
#include "io.h"								// I/O register definitions
#include "i28f160.h"

#define w Chipset

#pragma intrinsic(memset,memcpy)

#include "Ops.h"

//
// ROM buffer access functions
//

static __inline void WrDirtyPage(DWORD d)
{
	if (pbyRomDirtyPage)					// using dirty ROM page table
	{
		DWORD dwPage = d / ROMPAGESIZE;		// this is the page

		_ASSERT(dwPage < dwRomDirtyPageSize);
		pbyRomDirtyPage[dwPage] = TRUE;		// page is dirty
	}
	return;
}

static __inline void EraseBlock(DWORD d,DWORD dwNibSize)
{
	LPBYTE pbyAddr = pbyRom + d;

	while (dwNibSize--)
	{
		WrDirtyPage(d++);					// make page dirty
		*pbyAddr++ = 0x0F;					// clear address
	}
	return;
}

static CONST LPBYTE ppReg[] = { w.A, w.B, w.C, w.D };

static QWORD DecodeReg64(LPBYTE R, BYTE byNF)
{
	QWORD qwVal = Npack64(R,16);			// generate 64bit number from register

	switch (byNF)							// field selector
	{
	case  0: return (qwVal >> (w.P*4)) & 0xf;			// P
	case  1: return qwVal & ~((QWORD)~0 << ((w.P+1)*4));// WP
	case  2: return (qwVal >> 8) & 0xf;					// XS
	case  3: return qwVal & 0xfff;						// X
	case  4: return (qwVal >> 60) & 0xf;				// S
	case  5: return (qwVal >> 12) & 0x0000ffffffffffff;	// M
	case  6: return qwVal & 0xff;						// B
	case  7: return qwVal;								// W
	case 15: return qwVal & 0xfffff;					// A
//	default: return qwVal & w.fld[byNF-8];				// F1-F7
	default: return qwVal;
	}
}

static void EncodeReg64(QWORD v, LPBYTE R, BYTE byNF)
{
	if (byNF > 7 && byNF < 15)				// user mask area F1-F7
	{
//		QWORD qwMask = w.fld[byNF-8];		// F1-F7
//		QWORD qwVal = Npack64(R,16);		// original content of register
//		v = (v & qwMask) | (qwVal & ~qwMask); // mask operation
		byNF = 7;							// write W area
	}

	Nunpack64(R+F_s[byNF], v, F_l[byNF]);
	return;
}

static QWORD o80BReg164(LPBYTE I)
{
	_ASSERT((I[5] & 3) < ARRAYSIZEOF(ppReg));
	return DecodeReg64(ppReg[I[5] & 3], I[6]);
}

static QWORD o80BReg264(LPBYTE I)
{
	_ASSERT((I[5] >> 2) < ARRAYSIZEOF(ppReg));
	return DecodeReg64(ppReg[I[5] >> 2], I[6]);
}

static void o80BRegWrite(QWORD v, LPBYTE I)
{
	_ASSERT((I[5] & 3) < ARRAYSIZEOF(ppReg));
	EncodeReg64(v, ppReg[I[5] & 3], I[6]);
	return;
}

// SETFLDx
VOID o80BF7x(LPBYTE I)
{
	QWORD qwVal;

_ASSERT(FALSE);								// not tested so far
	w.pc+=1;								// skip x nibble
	qwVal = Npack64(w.C,16);				// generate 64bit number from C register
	w.carry =  (qwVal == 0)					// set carry if field mask = 0
			|| (I[5] < 8) || (I[5] > 14);	// or x argument not in range 8..15

	if (!w.carry)							// field mask and argument are valid
	{
		_ASSERT(I[5] >= 8 && I[5] <= 14);
//		w.fld[I[5]-8] = qwVal;
	}
	return;
}

// RPL2 (normal LOOP with preserving Carry)
VOID o80B00(VOID)
{
	BYTE p[5];

	Nread(w.A, w.d0, 5);					// A=DAT0 A
	w.d0 = (w.d0 + 5) & 0xFFFFF;			// D0=D0+ 5
	Nread(p, Npack(w.A,5), 5);				// PC=(A)
	w.pc = Npack(p,5);
	return;
}

// FALSE +5
VOID o80B30(VOID)
{
	Ndec(w.D, 5, 0);			// D=D-1 A
	if (w.carry) w.pc = 0x03A9B; // memerr?
	w.d1 -= 5;					// D1=D1- 5 (don't care about carry here)
	Nwrite(w.A, w.d1, 5);		// DAT1=A A
	o80B00();					// LOOP
	return;
}

// DOFALSE
VOID o80B40(VOID)
{
	w.P = 0;								// P= 0
	PCHANGED;
	Nunpack(w.C,0x03AC0,5);					// LC(5) =FALSE
	memcpy(w.A, w.C, 5);					// A=C A
	o80B30();								// PC=(A) (call FALSE)
	return;
}

// BEEP2
VOID o80B50(CHIPSET* w)
{
	BYTE  fbeep;
	DWORD freq,dur;

	freq = Npack(w->D,5);					// frequency in Hz
	dur = Npack(w->C,5);					// duration in ms
	Nread(&fbeep,0x80F0F,1);				// fetch system flags -53 to -56

	w->carry = TRUE;						// setting of no beep
	if (!(fbeep & 0x8) && freq)				// bit -56 clear and frequency > 0 Hz
	{
		if (freq > 4400) freq = 4400;		// high limit of HP (SX)

		SoundBeep(freq,dur);				// beeping

		// estimate cpu cycles for beeping time (4MHz)
		w->cycles += dur * 4000;

		// original routine return with...
		w->P = 0;							// P=0
		w->intk = TRUE;						// INTON
		w->carry = FALSE;					// RTNCC
	}
	w->pc = rstkpop();
	return;
}

// MOVEDOWN
VOID o80B60(VOID)
{
	BYTE  byData[16];
	DWORD dwC,s;

	for (dwC = Npack(w.C,5); dwC > 0; dwC -= s)
	{
		s = ARRAYSIZEOF(byData);			// max size for data
		if (dwC < s) s = dwC;

		Npeek(byData,w.d0,s);				// read source without CRC update
		Nwrite(byData,w.d1,s);				// write destination

		w.d0 = (w.d0 + s) & 0xFFFFF;
		w.d1 = (w.d1 + s) & 0xFFFFF;
	}

	w.P = 0;
	PCHANGED;
	w.carry = FALSE;
	return;
}

// MOVEUP
VOID o80B70(VOID)
{
	BYTE  byData[16];
	DWORD dwC,s;

	for (dwC = Npack(w.C,5); dwC > 0; dwC -= s)
	{
		s = ARRAYSIZEOF(byData);			// max size for data
		if (dwC < s) s = dwC;

		w.d0 = (w.d0 - s) & 0xFFFFF;
		w.d1 = (w.d1 - s) & 0xFFFFF;

		Npeek(byData,w.d0,s);				// read source without CRC update
		Nwrite(byData,w.d1,s);				// write destination
	}

	w.P = 0;
	PCHANGED;
	w.carry = FALSE;
	return;
}

// CREATETEMP
VOID o80B80(VOID)
{
	DWORD dwC,dwAddress;

	dwC = Npack(w.C,5);						// desired size of hole
	dwAddress = RPL_CreateTemp(dwC,FALSE);	// allocate memory
	if (dwAddress)
	{
		w.d0 = dwAddress;					// object position
		w.d1 = (w.d0 + dwC) & 0xFFFFF;		// link field of hole
		w.carry = FALSE;					// no error
	}
	else
	{
		w.carry = TRUE;						// error
	}

	dwC += 6;								// desired size + link field
	Nunpack(w.B,dwC,5);						// B[A] = size + 6
	Nunpack(w.C,dwC,5);						// C[A] = size + 6
	return;
}

// RCKBp
VOID o80B90(CHIPSET* w)						// ROM Check Beep patch
{
	DWORD dw2F,dwCpuFreq;
	DWORD freq,dur;
	BYTE f,d;

	f = w->C[1];							// f = freq ctl
	d = w->C[0];							// d = duration ctl

	// CPU strobe frequency @ RATE 27 = 3.67MHz
	// CPU strobe frequency @ RATE 29 = 3.93MHz
	dwCpuFreq = ((27 + 1) * 524288) >> 2;

	dw2F = f * 180 + 367;					// F=f*90+183.5

	freq = dwCpuFreq / dw2F;
	dur = (dw2F * (256 - 16 * d)) * 1000 / 2 / dwCpuFreq;

	if (freq > 4400) freq = 4400;			// high limit of HP

	SoundBeep(freq,dur);					// beeping

	// estimate cpu cycles for beeping time (4MHz)
	w->cycles += dur * 4000;

	w->P = 0;								// P=0
	w->carry = FALSE;						// RTNCC
	w->pc = rstkpop();
	return;
}

// setup basic memory configuration
VOID o80B04(VOID)
{
	DWORD a;

	a = Npack(w.C,5);						// save C[A]
	Reset();								// unconfig all devices
	Nunpack(w.C,0x100,5);					// IO:  0x00100
	Config();								// addr
	Nunpack(w.C,0x80000,5);					// RAM: 0x80000 size 256KB
	Config();								// size
	Config();								// addr
	Nunpack(w.C,a,5);						// restore C[A]
	w.P = 0;
	PCHANGED;
	return;
}

// erase flash bank
VOID o80B14(VOID)
{
	DWORD dwStart,dwStop;

	BYTE byBank = w.C[15];					// C[S] = bank to erase

_ASSERT(FALSE);								// not tested so far
	// ROM is logically organized in 16 banks with 128KB
	dwStart = byBank * _KB(128);			// start address
	dwStop = dwStart + _KB(128);			// last address
	if (byBank == 0) dwStart += _KB(64);	// skip boot loader

	// clear bank
	EraseBlock(dwStart,dwStop-dwStart);

	w.carry = FALSE;						// no error
	return;
}

// write bytes to flash
VOID o80B24(VOID)
{
	LPBYTE pbyBuffer;
	DWORD  dwNib,dwAddr,dwSize;

	dwNib = Npack(w.C,5) * 2;				// no. of nibbles to copy
	dwAddr = FlashROMAddr(w.d1);			// linear addr in flash chip

	dwSize = dwRomSize - dwAddr;			// remaining memory size in flash
	if (dwNib > dwSize) dwNib = dwSize;		// prevent buffer overflow

	pbyBuffer = (LPBYTE) malloc(dwNib);		// allocate data buffer
	if (pbyBuffer != NULL)
	{
		DWORD i;

		Npeek(pbyBuffer,w.d0,dwNib);		// get data

		for (i = 0; i < dwNib; ++i)
		{
			WrDirtyPage(dwAddr);			// make page dirty
			pbyRom[dwAddr++] = pbyBuffer[i]; // write data
		}
		free(pbyBuffer);
	}

	w.d0 += dwNib;							// update source register
	w.d1 += dwNib;							// update destination register
	w.carry = FALSE;						// no error
	return;
}

// OUTBYT
VOID o80B65(VOID)
{
	// set Transmitting annunciator
	BYTE byAnn = w.IORam[ANNCTRL+1] | 0x2;
	WriteIO(&byAnn,ANNCTRL+1,1);

	SendByteUdp((BYTE) Npack(w.A,2));		// send data byte

	w.P = 0;
	PCHANGED;
	w.carry = FALSE;						// no error
	return;
}

// CdB for HP: add apples BUSCC commands
VOID o80BExt(LPBYTE I) // Saturnator extentions
{
	DWORD a;

	w.pc+=2;
	switch (I[3]+(I[4]<<4))
	{
	case 0x00: o80B00(); break; // RPL2 (preserve Carry)
	case 0x03: o80B30(); break; // FALSE
	case 0x04: o80B40(); break; // DOFALSE
	case 0x05: o80B50(&w); PCHANGED; break; // BEEP2 implemented using Emu48's beep
	case 0x06: o80B60(); break; // MOVEDOWN
	case 0x07: o80B70(); break; // MOVEUP
	case 0x08: o80B80(); break; // CREATETEMP
	case 0x09: o80B90(&w); PCHANGED; break; // RCKBp (ROM Check Beep patch)
	case 0x0A: break; // KEYDN not implemented
	case 0x0B: break; // no doslow implemented
	case 0x10: // simulate off function
		{
			BOOL bShutdn = TRUE;			// shut down

			// only shut down when no timer wake up
			if (w.IORam[TIMER1_CTRL]&WKE)	// WKE bit of timer1 is set
			{
				if (ReadT1()&0x08)			// and MSB of timer1 is set
				{
					w.IORam[TIMER1_CTRL] &= ~WKE;	// clear WKE
					bShutdn = FALSE;		// don't shut down
					}
				}
			if (w.IORam[TIMER2_CTRL]&WKE)	// WKE bit of timer2 is set
			{
				if (ReadT2()&0x80000000)	// and MSB of timer2 is set
				{
					w.IORam[TIMER2_CTRL] &= ~WKE;	// clear WKE
					bShutdn = FALSE;		// don't shut down
				}
			}
			if (w.in==0 && bShutdn)			// shut down only when enabled
			{
				w.Shutdn = TRUE;			// set mode before exit emulation loop
				bInterrupt = TRUE;
			}
		}
		break;
	case 0x11: w.pc+=2; break; // do not do gettime, just skip the RTN after it to fall in the normal gettime function (only valid in untouched ROM)
	case 0x12: break; // do not do settime, fall in the normal settime function (only valid in untouched ROM)
	case 0x13: break; // RESETOS not implemented
	case 0x14: break; // AUTOTEST not implemented
	case 0x15: break; // NATIVE? not implemented
	case 0x17: break; // SERIAL not implemented
	case 0x28: w.HST |= I[5]; w.pc+=1; break; // HST=1.x
	case 0x29: w.A[4]= w.A[3]= w.A[2]= w.A[0]= 0; if (cCurrentRomType=='Q') w.A[1]=5; else w.A[1]=4; break; // screen height = 0x50 = 80
	case 0x2A: w.A[4]= w.A[3]= w.A[2]= 0; w.A[1]=8; w.A[0]=3; break; // screen width = 0x83 = 131
	case 0x2B: w.carry = (cCurrentRomType == '2'); break; // it is medium apple
	case 0x2C: w.carry = (cCurrentRomType == 'Q'); break; // it is big apple
	case 0x2E: w.carry = (nCurrentClass == 50);    break; // it is big apple V2
	case 0x30: w.d0address= Npack(w.C,5)>>12; Map(0,0xff); break; //config_disp0 Ca:address 4K data
	case 0x31: w.d0address=0; Map(0,0xff); RefreshDisp0(); break; //unconfig_disp0 does the refresh
	case 0x32: RefreshDisp0(); break; //refresh_disp0 force refresh
	case 0x33: a= Npack(w.C,2); if (a>(DWORD)SCREENHEIGHT) a= SCREENHEIGHT; /* w.lcounter = (SCREENHEIGHT-a) */; w.d0size= a; RefreshDisp0(); break; //set_lines_disp0 nb in Cb
	case 0x34: w.d0offset= Npack(w.C,5); w.d0offset &= 0x7FF; break; //set_offset_disp0 offset to disp in disp0
	case 0x35: Nunpack(w.C,w.d0offset,5); break;			// native_get_line_disp0
	case 0x38: w.HST |= I[5]; w.pc+=3; break; // ?HST=1.x not implemented
	case 0x40: o80B04(); break; // setup basic memory configuration
//	case 0x41: o80B14(); break; // erase flash bank
	case 0x42: o80B24(); break; // write bytes into flash
//	case 0x43: ???				// format flash bank
	case 0x50: break; // REMON not implemented
	case 0x51: break; // REMOFF not implemented
	case 0x56: o80B65(); break; // OUTBYT
	case 0x57: w.D[0]= w.D[1]= 0; break;
	case 0x60: break; // ACCESSSD not implemented
	case 0x61: break; // PORTTAG? not implemented
	case 0x64: w.carry = FALSE; break; // no SD card inserted
	case 0x66: w.carry = FALSE; break; // simulate format fail card inserted
	case 0x7F: w.carry = TRUE; w.pc+=1; break; // SETFLDn not implemented, set carry for failed
	case 0x80: { QWORD b = o80BReg264(I); o80BRegWrite(b, I);                             w.pc+=2; break; } // r=s
	case 0x81: { QWORD a = o80BReg164(I); QWORD b = o80BReg264(I); o80BRegWrite(a+b, I);  w.pc+=2; break; } // r=r+s
	case 0x82: { QWORD a = o80BReg164(I); QWORD b = o80BReg264(I); o80BRegWrite(a-b, I);  w.pc+=2; break; } // r=r-s
	case 0x83: { QWORD a = o80BReg164(I); QWORD b = o80BReg264(I); o80BRegWrite(a*b, I);  w.pc+=2; break; } // r=r*s
	case 0x84: { QWORD a = o80BReg164(I); QWORD b = o80BReg264(I); o80BRegWrite(a/b, I);  w.pc+=2; break; } // r=r/s
	case 0x85: { QWORD a = o80BReg164(I); QWORD b = o80BReg264(I); o80BRegWrite(a%b, I);  w.pc+=2; break; } // r=r%s
	case 0x86: { QWORD b = o80BReg264(I); o80BRegWrite(~b, I);                            w.pc+=2; break; } // r=-r-1
	case 0x87: { QWORD b = o80BReg264(I); o80BRegWrite((QWORD)(-(__int64)b), I);          w.pc+=2; break; } // r=-r
	case 0x88: { QWORD a = o80BReg164(I); QWORD b = o80BReg264(I); o80BRegWrite(a<<b, I); w.pc+=2; break; } // r=r<s
	case 0x89: { QWORD a = o80BReg164(I); QWORD b = o80BReg264(I); o80BRegWrite(a>>b, I); w.pc+=2; break; } // r=r>s
	case 0x8A: { QWORD a = o80BReg164(I); QWORD b = o80BReg264(I); o80BRegWrite(a^b,  I); w.pc+=2; break; } // r=r^s
	case 0x90: break; // data streamer not implemented
	case 0xEE: break; // ARMFLUSH not implemented
	case 0xEF: break; // ARMSYS not implemented
	case 0xFF: break; // ARMSAT not implemented
	default: w.pc-= 2;
	}
	return;
}
