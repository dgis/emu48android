/*
 *   symbfile.c
 *
 *   This file is part of Emu48
 *
 *   Copyright (C) 2008 Christoph Gießelink
 *
 */
#include "pch.h"
#include "Emu48.h"

//################
//#
//#    Saturn Object File Reading
//#
//################

#define RECORD_BLOCK	256					// block size
#define OS_RESOLVED		0x8000				// resolved symbol
#define OS_RELOCATABLE	0x4000				// relocatable symbol

#define SAT_ID			"Saturn3"			// saturn block header
#define SYMB_ID			"Symb"				// symbol block header

#define HASHENTRIES		199					// size of hash table

typedef struct _REFDATA
{
	LPTSTR lpszName;						// symbol name
	DWORD  dwAddr;							// resolved address
	struct _REFDATA* pNext;
} REFDATA, *PREFDATA;

static PREFDATA ppsBase[HASHENTRIES];		// base of symbol references (initialized with NULL)

static __inline DWORD GetHash(DWORD dwVal)
{
	return dwVal % HASHENTRIES;				// hash function
}

static DWORD GetBigEndian(LPBYTE pbyData, INT nSize)
{
	DWORD dwVal = 0;

	while (nSize-- > 0)
	{
		dwVal <<= 8;
		dwVal += *pbyData++;
	}
	return dwVal;
}

//
// check if entry table is empty
//
BOOL RplTableEmpty(VOID)
{
	DWORD i;

	BOOL bEmpty = TRUE;

	// check if hash table is empty
	for (i = 0; bEmpty && i < ARRAYSIZEOF(ppsBase); ++i)
	{
		bEmpty = (ppsBase[i] == NULL);		// check if empty
	}
	return bEmpty;
}

//
// load entry table
//
BOOL RplLoadTable(LPCTSTR lpszFilename)
{
	BYTE   byPage[RECORD_BLOCK];			// record page size
	HANDLE hFile;
	DWORD  dwFileLength,dwCodeLength,dwNoSymbols,dwNoReferences;
	DWORD  dwFilePos,dwBytesRead,dwSymb,dwPageIndex,dwResolvedSymb;
	BOOL   bSymbol,bSucc;

	bSucc = FALSE;

	hFile = CreateFile(lpszFilename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		dwResolvedSymb = 0;					// no resolved symbols added
		bSymbol = TRUE;						// next set is a symbol

		// read first page
		ReadFile(hFile,byPage,sizeof(byPage),&dwBytesRead,NULL);
		if (dwBytesRead == sizeof(byPage) && memcmp(byPage,SAT_ID,7) == 0)
		{
			// file length in bytes
			dwFileLength = GetBigEndian(byPage+7,sizeof(WORD)) * sizeof(byPage);

			// code area in nibbles
			dwCodeLength = GetBigEndian(byPage+9,sizeof(DWORD));

			// no. of symbols & references
			dwNoSymbols = GetBigEndian(byPage+13,sizeof(WORD));

			// no. of references
			dwNoReferences = GetBigEndian(byPage+15,sizeof(WORD));

			// convert code area length into no. of pages
			dwPageIndex = (dwCodeLength + (2 * sizeof(byPage) - 1)) / (2 * sizeof(byPage));

			// calculate no. of code pages
			dwFilePos = dwPageIndex * sizeof(byPage);

			// jump to begin of symbols by skipping no. of code pages
			bSucc = SetFilePointer(hFile,dwFilePos,NULL,FILE_CURRENT) != INVALID_SET_FILE_POINTER;

			dwFilePos += sizeof(byPage);	// actual file position
		}

		// read all symbol pages
		for (dwPageIndex = 256, dwSymb = 0; bSucc && dwSymb < dwNoSymbols; dwPageIndex += 42)
		{
			if (dwPageIndex >= 256)			// read complete page
			{
				// read new symbol page
				ReadFile(hFile,byPage,sizeof(byPage),&dwBytesRead,NULL);
				dwFilePos += dwBytesRead;	// update file position
				if (   dwFilePos > dwFileLength
					|| dwBytesRead != sizeof(byPage)
					|| memcmp(byPage,SYMB_ID,4) != 0)
				{
					bSucc = FALSE;
					break;
				}

				dwPageIndex = 4;			// begin of new symbol
			}

			if (bSymbol)					// this is the 42 byte symbol set
			{
				WORD wSymbolType = (WORD) GetBigEndian(byPage+dwPageIndex+36,sizeof(WORD));

				// check if it's a resolved or relocatable symbol
				bSymbol = (wSymbolType & OS_RESOLVED) != 0;

				if (bSymbol) ++dwResolvedSymb; // added resolved symbol

				if (wSymbolType == OS_RESOLVED) // resolved symbol type
				{
					TCHAR szSymbolName[36+1],*pcPtr;
					PREFDATA pData;
					DWORD dwHash;

					#if defined _UNICODE
					{
						MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,(LPCSTR)byPage+dwPageIndex,36,
											szSymbolName,ARRAYSIZEOF(szSymbolName));
						szSymbolName[36] = 0; // set EOS
					}
					#else
					{
						lstrcpyn(szSymbolName,(LPCSTR)byPage+dwPageIndex,ARRAYSIZEOF(szSymbolName));
					}
					#endif

					// cut symbol name at first space character
					if ((pcPtr = _tcschr(szSymbolName,_T(' '))) != NULL)
						*pcPtr = 0;			// set EOS

					// allocate symbol memory
					VERIFY(pData = (PREFDATA) malloc(sizeof(*pData)));
					pData->lpszName = DuplicateString(szSymbolName);
					pData->dwAddr = GetBigEndian(byPage+dwPageIndex+38,sizeof(DWORD));

					// add to hash table
					dwHash = GetHash(pData->dwAddr);
					pData->pNext = ppsBase[dwHash];
					ppsBase[dwHash] = pData;
				}

				++dwSymb;					// got symbol
			}
			else							// 42 byte fill reference
			{
				bSymbol = TRUE;				// nothing to do, next is a symbol set
			}
		}

		bSucc = bSucc && (dwFilePos <= dwFileLength)
					  && (dwNoSymbols == (dwResolvedSymb + dwNoReferences));

		CloseHandle(hFile);
	}

	if (!bSucc) RplDeleteTable();			// delete current table
	return bSucc;
}

//
// delete entry table
//
VOID RplDeleteTable(VOID)
{
	PREFDATA pData;
	DWORD    i;

	// clear hash entries
	for (i = 0; i < ARRAYSIZEOF(ppsBase); ++i)
	{
		while (ppsBase[i] != NULL)			// walk through all datasets
		{
			pData = ppsBase[i]->pNext;
			free(ppsBase[i]->lpszName);
			free(ppsBase[i]);
			ppsBase[i] = pData;
		}
	}
	return;
}

//
// return name for given entry address
//
LPCTSTR RplGetName(DWORD dwAddr)
{
	PREFDATA pData = ppsBase[GetHash(dwAddr)];

	// walk through all datasets of hash entry
	for (; pData != NULL; pData = pData->pNext)
	{
		if (pData->dwAddr == dwAddr)		// found address
			return pData->lpszName;			// return symbol name
	}
	return NULL;
}

//
// return entry address for given name
//
BOOL RplGetAddr(LPCTSTR lpszName, DWORD *pdwAddr)
{
	PREFDATA pData;
	DWORD    i;

	// check for every dataset in hash table
	for (i = 0; i < ARRAYSIZEOF(ppsBase); ++i)
	{
		// walk through all datasets of hash entry
		for (pData = ppsBase[i]; pData != NULL; pData = pData->pNext)
		{
			// found symbol name
			if (lstrcmp(lpszName,pData->lpszName) == 0)
			{
				*pdwAddr = pData->dwAddr;	// return address
				return FALSE;				// found
			}
		}
	}
	return TRUE;							// not found
}
