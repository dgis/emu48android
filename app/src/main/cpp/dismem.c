/*
 *   dismem.c
 *
 *   This file is part of Emu48
 *
 *   Copyright (C) 2012 Christoph Gieﬂelink
 *
 */
#include "pch.h"
#include "Emu48.h"

typedef struct								// type of model memory mapping
{
	BYTE         byType;					// calculator type
	CONST LPBYTE *ppbyNCE1;					// NCE1 data
	CONST DWORD  *pdwNCE1Size;				// NCE1 size
	CONST LPBYTE *ppbyNCE2;					// NCE2 data
	CONST DWORD  *pdwNCE2Size;				// NCE2 size
	CONST LPBYTE *ppbyCE1;					// CE1 data
	CONST DWORD  *pdwCE1Size;				// CE1 size
	CONST LPBYTE *ppbyCE2;					// CE2 data
	CONST DWORD  *pdwCE2Size;				// CE2 size
	CONST LPBYTE *ppbyNCE3;					// NCE3 data
	CONST DWORD  *pdwNCE3Size;				// NCE3 size
} MODEL_MAP_T;

static CONST LPBYTE pbyNoMEM = NULL;		// no memory module

static CONST MODEL_MAP_T MemMap[] =
{
	{
		0,									// default
		&pbyNoMEM, NULL,					// nc.
		&pbyNoMEM, NULL,					// nc.
		&pbyNoMEM, NULL,					// nc.
		&pbyNoMEM, NULL,					// nc.
		&pbyNoMEM, NULL						// nc.
	},
	{
		'6',								// HP38G (64K)
		&pbyRom,   &dwRomSize,				// ROM
		&Port0,    &Chipset.Port0Size,		// RAM
		&pbyNoMEM, NULL,					// nc.
		&pbyNoMEM, NULL,					// nc.
		&pbyNoMEM, NULL						// nc.
	},
	{
		'A',								// HP38G
		&pbyRom,   &dwRomSize,				// ROM
		&Port0,    &Chipset.Port0Size,		// RAM
		&pbyNoMEM, NULL,					// nc.
		&pbyNoMEM, NULL,					// nc.
		&pbyNoMEM, NULL						// nc.
	},
	{
		'E',								// HP39/40G
		&pbyRom,   &dwRomSize,				// ROM
		&Port0,    &Chipset.Port0Size,		// RAM part 1
		&pbyNoMEM, NULL,					// BS
		&pbyNoMEM, NULL,					// nc.
		&Port2,    &Chipset.Port2Size		// RAM part 2
	},
	{
		'G',								// HP48GX
		&pbyRom,   &dwRomSize,				// ROM
		&Port0,    &Chipset.Port0Size,		// RAM
		&pbyNoMEM, NULL,					// BS
		&Port1,    &Chipset.Port1Size,		// Card slot 1
		&pbyPort2, &dwPort2Size				// Card slot 2
	},
	{
		'S',								// HP48SX
		&pbyRom,   &dwRomSize,				// ROM
		&Port0,    &Chipset.Port0Size,		// RAM
		&Port1,    &Chipset.Port1Size,		// Card slot 1
		&pbyPort2, &dwPort2Size,			// Card slot 2
		&pbyNoMEM, NULL						// nc.
	},
	{
		'X',								// HP49G
		&pbyRom,   &dwRomSize,				// Flash
		&Port0,    &Chipset.Port0Size,		// RAM
		&pbyNoMEM, NULL,					// BS
		&Port1,    &Chipset.Port1Size,		// Port 1 part 1
		&Port2,    &Chipset.Port2Size		// Port 1 part 2
	},
	{ // CdB for HP: add Q type
		'Q',								// HP49g+
		&pbyRom,   &dwRomSize,				// Flash
		&Port0,	   &Chipset.Port0Size,		// RAM
		&pbyNoMEM, NULL,					// BS
		&Port1,    &Chipset.Port1Size,		// Port 1 part 1
		&Port2,    &Chipset.Port2Size		// Port 1 part 2
	},
	{ // CdB for HP: add 2 type
		'2',								// HP48gII
		&pbyRom,   &dwRomSize,				// ROM
		&Port0,	   &Chipset.Port0Size,		// RAM
		&pbyNoMEM, NULL,					// BS
		&pbyNoMEM, NULL,					// Port 1 part 1
		&pbyNoMEM, NULL,					// Port 1 part 2
	},
	{ // CdB for HP: add P type
		'P',								// HP39g+/gs
		&pbyRom,   &dwRomSize,				// ROM
		&Port0,	   &Chipset.Port0Size,		// RAM
		&pbyNoMEM, NULL,					// BS
		&pbyNoMEM, NULL,					// nc.
		&Port2,	   &Chipset.Port2Size		// RAM part 2
	}
};

static MODEL_MAP_T CONST *pMapping = MemMap; // model specific memory mapping
static enum MEM_MAPPING eMapType = MEM_MMU;	// MMU memory mapping

static LPBYTE pbyMapData = NULL;
static DWORD  dwMapDataSize = 0;
static DWORD  dwMapDataMask = 0;

BOOL SetMemRomType(BYTE cCurrentRomType)
{
	UINT i;

	pMapping = MemMap;						// init default mapping

	// scan for all table entries
	for (i = 0; i < ARRAYSIZEOF(MemMap); ++i)
	{
		if (MemMap[i].byType == cCurrentRomType)
		{
			pMapping = &MemMap[i];			// found entry
			return TRUE;
		}
	}
	return FALSE;
}

BOOL SetMemMapType(enum MEM_MAPPING eType)
{
	BOOL bSucc = TRUE;

	eMapType = eType;

	switch (eMapType)
	{
	case MEM_MMU:
		pbyMapData = NULL;					// data
		dwMapDataSize = 512 * 1024 * 2;		// data size
		dwMapDataMask = dwMapDataSize - 1;	// size mask
		break;
	case MEM_NCE1:
		pbyMapData = *pMapping->ppbyNCE1;
		dwMapDataSize = *pMapping->pdwNCE1Size; // ROM size is always in nibbles
		dwMapDataMask = dwMapDataSize - 1;	// size mask
		break;
	case MEM_NCE2:
		pbyMapData = *pMapping->ppbyNCE2;
		dwMapDataSize = *pMapping->pdwNCE2Size * 1024 * 2;
		dwMapDataMask = dwMapDataSize - 1;	// size mask
		break;
	case MEM_CE1:
		pbyMapData = *pMapping->ppbyCE1;
		dwMapDataSize = *pMapping->pdwCE1Size * 1024 * 2;
		dwMapDataMask = dwMapDataSize - 1;	// size mask
		break;
	case MEM_CE2:
		pbyMapData = *pMapping->ppbyCE2;
		dwMapDataSize = *pMapping->pdwCE2Size * 1024 * 2;
		dwMapDataMask = dwMapDataSize - 1;	// size mask
		break;
	case MEM_NCE3:
		pbyMapData = *pMapping->ppbyNCE3;
		dwMapDataSize = *pMapping->pdwNCE3Size * 1024 * 2;
		dwMapDataMask = dwMapDataSize - 1;	// size mask
		break;
	default: _ASSERT(FALSE);
		pbyMapData = NULL;
		dwMapDataSize = 0;
		dwMapDataMask = 0;
		bSucc = FALSE;
	}
	return bSucc;
}

enum MEM_MAPPING GetMemMapType(VOID)
{
	return eMapType;
}

BOOL GetMemAvail(enum MEM_MAPPING eType)
{
	switch (eType)
	{
	case MEM_MMU:  return TRUE;
	case MEM_NCE1: return *pMapping->ppbyNCE1 != NULL;
	case MEM_NCE2: return *pMapping->ppbyNCE2 != NULL;
	case MEM_CE1:  return *pMapping->ppbyCE1  != NULL;
	case MEM_CE2:  return *pMapping->ppbyCE2  != NULL;
	case MEM_NCE3: return *pMapping->ppbyNCE3 != NULL;
	default: _ASSERT(FALSE);
	}
	return FALSE;
}

DWORD GetMemDataSize(VOID)
{
	return dwMapDataSize;
}

DWORD GetMemDataMask(VOID)
{
	return dwMapDataMask;
}

BYTE GetMemNib(DWORD *p)
{
	BYTE byVal;

	if (pbyMapData == NULL)
	{
		Npeek(&byVal, *p, 1);
	}
	else
	{
		byVal = pbyMapData[*p];
	}
	*p = (*p + 1) & dwMapDataMask;
	return byVal;
}

VOID GetMemPeek(BYTE *a, DWORD d, UINT s)
{
	if (pbyMapData == NULL)
	{
		Npeek(a, d, s);
	}
	else
	{
		for (; s > 0; --s, ++d)
		{
			*a++ = pbyMapData[d & dwMapDataMask];
		}
	}
	return;
}
