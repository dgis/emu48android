/*
 *   romcrc.c
 *
 *   This file is part of Emu48
 *
 *   Copyright (C) 2022 Christoph Gießelink
 *
 */
#include "pch.h"
#include "Emu48.h"
#include "ops.h"

// flash page types
#define BOOT	0x86
#define FS		0x18
#define SYSTEM	0x32
#define ROM		0x0F
#define RAM		0xF0

//################
//#
//#    Restore HP38G/HP48GX/SX ROM CRC
//#
//################

// Clarke/Yorke CRC for HP38G and HP48GX/SX
#define a0	0x0								// Start Address
#define d0	(n0*16)							// Address offset
#define n0	0x4000							// Reads/Half-Sector
#define s0	1								// #Sectors (Sector Size=2*d)

// rebuild of the calculator =CHECKSUM function for the Clarke and the Yorke chip ROM
static WORD Checksum(LPBYTE pbyROM, DWORD dwStart, DWORD dwOffset, INT nReads, INT nSector)
{
	int i,j;

	WORD wCrc = 0;

	for (;nSector > 0; --nSector)			// evaluate each sector
	{
		LPBYTE pbyAddr1 = pbyROM + dwStart;
		LPBYTE pbyAddr2 = pbyAddr1 + dwOffset;

		for (i = 0; i < nReads; ++i)		// no. of reads in sector
		{
			for (j = 0; j < 16; ++j) wCrc = UpCRC(wCrc,*pbyAddr1++);
			for (j = 0; j < 16; ++j) wCrc = UpCRC(wCrc,*pbyAddr2++);
		}

		dwStart += 2 * dwOffset;			// next start page
	}
	return wCrc;
}

// calculate proper checksum to produce final CRC of FFFF
static __inline WORD CalcChksum(WORD wCrc, WORD wChksum)
{
	WORD q, r = wCrc, s = wChksum;

	// first take the last 4-nib back out of the CRC
	r = (((r >> 12) * 0x811) ^ (r << 4) ^ (s & 0xf));
	s >>= 4;
	r = (((r >> 12) * 0x811) ^ (r << 4) ^ (s & 0xf));
	s >>= 4;
	r = (((r >> 12) * 0x811) ^ (r << 4) ^ (s & 0xf));
	s >>= 4;
	r = (((r >> 12) * 0x811) ^ (r << 4) ^ (s & 0xf));

	// calculate new checksum to correct the CRC
	s = 0xf831;								// required MSNs to make goal
	q = (q<<4) | ((r ^ s) & 0xf);			// get 1st (least sig) nib
	r = (r>>4) ^ ((s & 0xf) * 0x1081);
	s >>= 4;
	q = (q<<4) | ((r ^ s) & 0xf);
	r = (r>>4) ^ ((s & 0xf) * 0x1081);
	s >>= 4;
	q = (q<<4) | ((r ^ s) & 0xf);
	r = (r>>4) ^ ((s & 0xf) * 0x1081);
	s >>= 4;
	q = (q<<4) | ((r ^ s) & 0xf);
	return q;
}

static VOID CorrectCrc(DWORD dwAddrCrc, WORD wCrc)
{
	if (wCrc != 0xFFFF)						// wrong crc result
	{
		INT s;

		// get actual crc correction value
		const WORD wChkAct =   (pbyRom[dwAddrCrc+0] << 12)
							 | (pbyRom[dwAddrCrc+1] << 8)
							 | (pbyRom[dwAddrCrc+2] << 4)
							 | (pbyRom[dwAddrCrc+3]);

		wCrc = CalcChksum(wCrc,wChkAct);	// calculate new checksum

		for (s = 3; s >= 0; --s)			// write new checksum
		{
			PatchNibble(dwAddrCrc + s,(BYTE) (wCrc & 0xf));
			wCrc >>= 4;
		}
	}
	return;
}


//################
//#
//#    Restore HP49G ROM CRC
//#
//################

static VOID CorrectFlashCrc(LPBYTE pbyMem, DWORD dwSize, DWORD dwOffset, DWORD dwLength)
{
	// address overflow (data length + 4 nibble CRC)
	if (dwOffset + dwLength + 4 <= dwSize)
	{
		WORD wRefCrc,wCrc = 0;

		pbyMem += dwOffset;						// start address

		for (; dwLength > 0; --dwLength)		// update CRC
		{
			wCrc = UpCRC(wCrc,*pbyMem++);
		}

		wRefCrc = (WORD) Npack(pbyMem,4);		// read reference CRC

		if(wRefCrc != wCrc)						// wrong page CRC
		{
			INT s;

			// linear CRC address in ROM
			DWORD dwAddrCrc = (DWORD) (pbyMem - pbyRom);

			wRefCrc = wCrc;						// working copy of CRC
			for (s = 0; s < 4; ++s)				// write new checksum
			{
				PatchNibble(dwAddrCrc++,(BYTE) (wRefCrc & 0xf));
				wRefCrc >>= 4;
			}

			_ASSERT(wCrc == (WORD) Npack(pbyMem,4));
		}
	}
	return;
}

static __inline VOID CorrectFlashRom(LPBYTE pbyMem, DWORD dwSize)
{
	CorrectFlashCrc(pbyMem,dwSize,0x20A,Npack(pbyMem+0x20A,5));
	return;
}

static __inline VOID CorrectFlashSystem(LPBYTE pbyMem, DWORD dwSize)
{
	CorrectFlashCrc(pbyMem,dwSize,0x20A,Npack(pbyMem+0x100,5));
	return;
}

static __inline VOID CorrectFlashPage(LPBYTE pbyMem, DWORD dwSize, DWORD dwPage)
{
	_ASSERT(dwPage >= 0 && dwPage < 16);

	dwPage *= _KB(128);						// convert page no. to data offset
	if (dwPage + _KB(128) <= dwSize)		// page inside flash chip
	{
		BYTE byType;

		pbyMem += dwPage;					// page address
		dwPage = _KB(128);					// page size

		// get bank type
		byType = (BYTE) Npack(pbyMem+0x200,2);
		if (byType == BOOT)					// 1st half of page is the boot bank
		{
			pbyMem += _KB(64);				// 2nd half of page
			dwPage = _KB(64);				// page size

			// get bank type
			byType = (BYTE) Npack(pbyMem+0x200,2);
		}

		_ASSERT(dwPage == _KB(64) || dwPage == _KB(128));

		switch (byType)
		{
		case FS:  // FRBankFileSystem
		case ROM: // FRBankRom
			CorrectFlashRom(pbyMem,dwPage);
			break;
		case SYSTEM: // FRBankSystem
			CorrectFlashSystem(pbyMem,dwPage);
			break;
		case RAM: // FRBankRam
		default: // illegal bank identifier
			break;
		}
	}
	return;
}

static __inline VOID CorrectAllFlashPages(VOID)
{
	DWORD dwPage;

	// check CRC of all pages
	CONST DWORD dwLastPage = dwRomSize / _KB(128);
	for (dwPage = 0; dwPage < dwLastPage; ++dwPage)
	{
		// correct CRC of page
		CorrectFlashPage(pbyRom,dwRomSize,dwPage);
	}
	return;
}


//################
//#
//#    Restore ROM CRC
//#
//################

VOID RebuildRomCrc(VOID)
{
	// HP38G, HP48GX, HP48SX
	if ((strchr("6AGS",cCurrentRomType)) && dwRomSize >= _KB(256))
	{
		// first 256KB
		CorrectCrc(0x7FFFC,Checksum(pbyRom,a0,d0,n0,s0));
	}
	// HP38G, HP48GX
	if ((strchr("6AG",cCurrentRomType)) && dwRomSize == _KB(512))
	{
		// second 256KB
		CorrectCrc(0xFFFFC,Checksum(pbyRom,a0+_KB(256),d0,n0,s0));
	}
	// HP39G/40G
	if (cCurrentRomType == 'E')
	{
		// has no Crc
	}
	// HP49G
	if (cCurrentRomType == 'X' && dwRomSize == _KB(2048))
	{
		CorrectAllFlashPages();				// go through all pages
	}
	return;
}
