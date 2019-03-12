/*
 *   redeye.c
 *
 *   This file is part of Emu48
 *
 *   Copyright (C) 2011 Christoph Gießelink
 *
 */
#include "pch.h"
#include "Emu48.h"
#include "io.h"

#define ERR_CHAR	127						// character for transfer error

#define H1  0x78
#define H2  0xE6
#define H3  0xD5
#define H4  0x8B

// HP redeye correction masks
static CONST BYTE byEmask[] = { H1, H2, H3, H4 };

static __inline UINT MAX(UINT a, UINT b)
{
	return (a>b)?a:b;
}

static __inline BYTE Parity(BYTE b)
{
	b ^= (b >> 4);
	b ^= (b >> 2);
	b ^= (b >> 1);
	return b & 1;
}

static __inline BYTE CreateCorrectionBits(BYTE b)
{
	UINT i;
	BYTE byVal = 0;

	for (i = 0; i < ARRAYSIZEOF(byEmask);++i)
	{
		byVal <<= 1;
		byVal |= Parity((BYTE) (b & byEmask[i]));
	}
	return byVal;
}

static __inline WORD CorrectData(WORD wData,WORD wMissed)
{
	while ((wMissed & 0xFF) != 0)			// clear every missed bit in data area
	{
		BYTE byBitMask;

		// detect valid H(i) mask
		WORD wMi = 0x800;					// first M(i) bit
		INT  i = 0;							// index to first H(i) mask

		while (TRUE)
		{
			if ((wMissed & wMi) == 0)		// possible valid mask
			{
				_ASSERT(i < ARRAYSIZEOF(byEmask));

				// select bit to correct
				byBitMask = wMissed & byEmask[i];

				if (Parity(byBitMask))		// only one bit set (parity odd)
					break;					// -> valid H(i) mask
			}

			wMi >>= 1;						// next M(i) bit
			i++;							// next H(i) mask
		}

		// correct bit with H(i) mask
		wMissed ^= byBitMask;				// clear this missed bit

		// parity odd -> wrong data value
		if (Parity((BYTE) ((wData & byEmask[i]) ^ ((wData & wMi) >> 8))))
			wData ^= byBitMask;				// correct value
	}
	return wData & 0xFF;					// only data byte is correct
}

VOID IrPrinter(BYTE c)
{
	static INT   nFrame = 0;				// frame counter
	static DWORD dwData = 0;				// half bit data container
	static INT   nStart = 0;				// frame counter disabled

	BOOL bLSRQ;

	dwData = (dwData << 1) | (c & LBO);		// grab the last 32 bit send through IR

	// Led Service ReQuest on Led Buffer Empty enabled
	bLSRQ = (Chipset.IORam[LCR] & ELBE) != 0;

	IOBit(SRQ2,LSRQ,bLSRQ);					// update LSRQ bit
	if (bLSRQ)								// interrupt on Led Buffer Empty enabled
	{
		Chipset.SoftInt = TRUE;				// execute interrupt
		bInterrupt = TRUE;
	}

	// HP40G and HP49G have no IR transmitter Led
	if ((cCurrentRomType == 'E' && nCurrentClass == 40) || cCurrentRomType == 'X')
		return;

	if (nFrame == 0)						// waiting for start bit condition
	{
		if ((dwData & 0x3F) == 0x07)		// start bit condition (000111 pattern)
		{
			nStart = 1;						// enable frame counter
		}
	}

	if (nFrame == 24)						// 24 half bit received
	{
		INT i;

		WORD wData = 0;						// data container
		WORD wMissed = 0;					// missed bit container
		INT  nCount = 0;					// no. of missed bits

		nFrame = 0;							// reset for next character
		nStart = 0;							// disable frame counter

		// separate to data and missed bits
		for (i = 0; i < 12; ++i)			// 12 bit frames
		{
			BYTE b = (BYTE) (dwData & 3);	// last 2 half bits

			if (b == 0x0 || b == 0x3)		// illegal half bit combination
			{
				wMissed |= (1 << i);		// this is a missed bit
				++nCount;					// incr. number of missed bits
			}
			else							// valid data bit
			{
				wData |= ((b >> 1) << i);	// add data bit
			}
			dwData >>= 2;					// next 2 half bits
		}

		if (nCount <= 2)					// error can be fixed
		{
			BYTE byOrgParity,byNewParity;

			byOrgParity = wData >> 8;		// the original parity information with missed bits
			byNewParity = ~(wMissed >> 8);	// missed bit mask for recalculated parity

			if (nCount > 0)					// error correction
			{
				wData = CorrectData(wData,wMissed);
			}

			wData &= 0xFF;					// remove parity information

			// recalculate parity data
			byNewParity &= CreateCorrectionBits((BYTE) wData);

			// wrong parity
			if (byOrgParity != byNewParity)
				wData = ERR_CHAR;			// character for transfer error
		}
		else
		{
			wData = ERR_CHAR;				// character for transfer error
		}

		SendByteUdp((BYTE) wData);			// send data byte
		return;
	}
	nFrame += nStart;						// next frame
	return;
}
