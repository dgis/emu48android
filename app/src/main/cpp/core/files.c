/*
 *   files.c
 *
 *   This file is part of Emu48
 *
 *   Copyright (C) 1995 Sebastien Carlier
 *
 */
#include "pch.h"
#include "Emu48.h"
#include "ops.h"
#include "io.h"								// I/O register definitions
#include "kml.h"
#include "i28f160.h"						// flash support
#include "debugger.h"
#include "lodepng.h"

#pragma intrinsic(abs,labs)

TCHAR  szEmuDirectory[MAX_PATH];
TCHAR  szRomDirectory[MAX_PATH];
TCHAR  szCurrentDirectory[MAX_PATH];
TCHAR  szCurrentKml[MAX_PATH];
TCHAR  szBackupKml[MAX_PATH];
TCHAR  szCurrentFilename[MAX_PATH];
TCHAR  szBackupFilename[MAX_PATH];
TCHAR  szBufferFilename[MAX_PATH];
TCHAR  szPort2Filename[MAX_PATH];

BOOL   bDocumentAvail = FALSE;				// document not available

BYTE   cCurrentRomType = 0;					// Model -> hardware
UINT   nCurrentClass = 0;					// Class -> derivate

LPBYTE Port0 = NULL;
LPBYTE Port1 = NULL;
LPBYTE Port2 = NULL;

LPBYTE pbyRom = NULL;
BOOL   bRomWriteable = TRUE;				// flag if ROM writeable
DWORD  dwRomSize = 0;
LPBYTE pbyRomDirtyPage = NULL;
DWORD  dwRomDirtyPageSize = 0;
WORD   wRomCrc = 0;							// fingerprint of patched ROM

LPBYTE pbyPort2 = NULL;
BOOL   bPort2Writeable = FALSE;
BOOL   bPort2IsShared = FALSE;
DWORD  dwPort2Size = 0;						// size of mapped port2
DWORD  dwPort2Mask = 0;
WORD   wPort2Crc = 0;						// fingerprint of port2

BOOL   bBackup = FALSE;

static HANDLE hRomFile = NULL;
static HANDLE hRomMap = NULL;
static HANDLE hPort2File = NULL;
static HANDLE hPort2Map = NULL;

// document signatures
static BYTE pbySignatureA[16] = "Emu38 Document\xFE";
static BYTE pbySignatureB[16] = "Emu39 Document\xFE";
static BYTE pbySignatureE[16] = "Emu48 Document\xFE";
static BYTE pbySignatureW[16] = "Win48 Document\xFE";
static BYTE pbySignatureV[16] = "Emu49 Document\xFE";
static HANDLE hCurrentFile = NULL;

static CHIPSET BackupChipset;
static LPBYTE  BackupPort0;
static LPBYTE  BackupPort1;
static LPBYTE  BackupPort2;
static BOOL    bRomPacked;

//################
//#
//#    Window Position Tools
//#
//################

VOID SetWindowLocation(HWND hWnd,INT nPosX,INT nPosY)
{
	WINDOWPLACEMENT wndpl;
	RECT *pRc = &wndpl.rcNormalPosition;

	wndpl.length = sizeof(wndpl);
	GetWindowPlacement(hWnd,&wndpl);
	pRc->right = pRc->right - pRc->left + nPosX;
	pRc->bottom = pRc->bottom - pRc->top + nPosY;
	pRc->left = nPosX;
	pRc->top = nPosY;
	SetWindowPlacement(hWnd,&wndpl);
	return;
}



//################
//#
//#    Filename Title Helper Tool
//#
//################

DWORD GetCutPathName(LPCTSTR szFileName, LPTSTR szBuffer, DWORD dwBufferLength, INT nCutLength)
{
	TCHAR  cPath[_MAX_PATH];				// full filename
	TCHAR  cDrive[_MAX_DRIVE];
	TCHAR  cDir[_MAX_DIR];
	TCHAR  cFname[_MAX_FNAME];
	TCHAR  cExt[_MAX_EXT];

	_ASSERT(nCutLength >= 0);				// 0 = only drive and name

	// split original filename into parts
	_tsplitpath(szFileName,cDrive,cDir,cFname,cExt);

	if (*cDir != 0)							// contain directory part
	{
		LPTSTR lpFilePart;					// address of file name in path
		INT    nNameLen,nPathLen,nMaxPathLen;

		GetFullPathName(szFileName,ARRAYSIZEOF(cPath),cPath,&lpFilePart);
		_tsplitpath(cPath,cDrive,cDir,cFname,cExt);

		// calculate size of drive/name and path
		nNameLen = lstrlen(cDrive) + lstrlen(cFname) + lstrlen(cExt);
		nPathLen = lstrlen(cDir);

		// maximum length for path
		nMaxPathLen = nCutLength - nNameLen;

		if (nPathLen > nMaxPathLen)			// have to cut path
		{
			TCHAR cDirTemp[_MAX_DIR] = _T("");
			LPTSTR szPtr;

			// UNC name
			if (cDir[0] == _T('\\') && cDir[1] == _T('\\'))
			{
				// skip server
				if ((szPtr = _tcschr(cDir + 2,_T('\\'))) != NULL)
				{
					// skip share
					if ((szPtr = _tcschr(szPtr + 1,_T('\\'))) != NULL)
					{
						INT nLength = (INT) (szPtr - cDir);

						*szPtr = 0;			// set EOS behind share

						// enough room for \\server\\share and "\...\"
						if (nLength + 5 <= nMaxPathLen)
						{
							lstrcpyn(cDirTemp,cDir,ARRAYSIZEOF(cDirTemp));
							nMaxPathLen -= nLength;
						}

					}
				}
			}

			lstrcat(cDirTemp,_T("\\..."));
			nMaxPathLen -= 5;				// need 6 chars for additional "\..." + "\"
			if (nMaxPathLen < 0) nMaxPathLen = 0;

			// get earliest possible '\' character
			szPtr = &cDir[nPathLen - nMaxPathLen];
			szPtr = _tcschr(szPtr,_T('\\'));
			// not found
			if (szPtr == NULL) szPtr = _T("");

			lstrcat(cDirTemp,szPtr);		// copy path with preample to dir buffer
			lstrcpyn(cDir,cDirTemp,ARRAYSIZEOF(cDir));
		}
	}

	_tmakepath(cPath,cDrive,cDir,cFname,cExt);
	lstrcpyn(szBuffer,cPath,dwBufferLength);
	return lstrlen(szBuffer);
}

VOID SetWindowPathTitle(LPCTSTR szFileName)
{
	TCHAR cPath[MAX_PATH];
	RECT  rectClient;

	if (*szFileName != 0)					// set new title
	{
		_ASSERT(hWnd != NULL);
		VERIFY(GetClientRect(hWnd,&rectClient));
		GetCutPathName(szFileName,cPath,ARRAYSIZEOF(cPath),rectClient.right/11);
		SetWindowTitle(cPath);
	}
	return;
}



//################
//#
//#    BEEP Patch check
//#
//################

BOOL CheckForBeepPatch(VOID)
{
	typedef struct beeppatch
	{
		const DWORD dwAddress;				// patch address
		const BYTE  byPattern[4];			// patch pattern
	} BEEPPATCH, *PBEEPPATCH;

	// known beep patches
	const BEEPPATCH s38[] =	{ { 0x017D0, { 0x8, 0x1, 0xB, 0x1 } } };
	const BEEPPATCH s39[] = { { 0x017BC, { 0x8, 0x1, 0xB, 0x1 } } };
	const BEEPPATCH s48[] = { { 0x017A6, { 0x8, 0x1, 0xB, 0x1 } } };
	const BEEPPATCH s49[] = { { 0x4157A, { 0x8, 0x1, 0xB, 0x1 } },		// 1.18/1.19-5/1.19-6
							  { 0x41609, { 0x8, 0x1, 0xB, 0x1 } } };	// 1.24/2.01/2.09

	const BEEPPATCH *psData;
	UINT nDataItems;
	BOOL bMatch;

	switch (cCurrentRomType)
	{
	case '6':
	case 'A':								// HP38G
		psData = s38;
		nDataItems = ARRAYSIZEOF(s38);
		break;
	case 'E':								// HP39/40G
		psData = s39;
		nDataItems = ARRAYSIZEOF(s39);
		break;
	case 'S':								// HP48SX
	case 'G':								// HP48GX
		psData = s48;
		nDataItems = ARRAYSIZEOF(s48);
		break;
	case 'X':								// HP49G
		psData = s49;
		nDataItems = ARRAYSIZEOF(s49);
		break;
	default:
		psData = NULL;
		nDataItems = 0;
	}

	// check if one data set match
	for (bMatch = FALSE; !bMatch && nDataItems > 0; --nDataItems)
	{
		_ASSERT(pbyRom != NULL && psData != NULL);

		// pattern matching?
		bMatch =  (psData->dwAddress + ARRAYSIZEOF(psData->byPattern) < dwRomSize)
			   && (memcmp(&pbyRom[psData->dwAddress],psData->byPattern,ARRAYSIZEOF(psData->byPattern))) == 0;
		++psData;							// next data set
	}
	return bMatch;
}



//################
//#
//#    Patch
//#
//################

static __inline BYTE Asc2Nib(BYTE c)
{
	if (c<'0') return 0;
	if (c<='9') return c-'0';
	if (c<'A') return 0;
	if (c<='F') return c-'A'+10;
	if (c<'a') return 0;
	if (c<='f') return c-'a'+10;
	return 0;
}

// functions to restore ROM patches
typedef struct tnode
{
	BOOL   bPatch;							// TRUE = ROM address patched
	DWORD  dwAddress;						// patch address
	BYTE   byROM;							// original ROM value
	BYTE   byPatch;							// patched ROM value
	struct tnode *prev;						// previous node
	struct tnode *next;						// next node
} TREENODE, *PTREENODE;

static TREENODE *nodePatch = NULL;

static BOOL PatchNibble(DWORD dwAddress, BYTE byPatch)
{
	PTREENODE p;

	_ASSERT(pbyRom);						// ROM defined
	if ((p = (PTREENODE) malloc(sizeof(TREENODE))) == NULL)
		return TRUE;

	p->bPatch = TRUE;						// address patched
	p->dwAddress = dwAddress;				// save current values
	p->byROM = pbyRom[dwAddress];
	p->byPatch = byPatch;
	p->prev = NULL;
	p->next = nodePatch;					// save node

	if (nodePatch) nodePatch->prev = p;		// add as previous element
	nodePatch = p;

	pbyRom[dwAddress] = byPatch;			// patch ROM
	return FALSE;
}

static VOID RestorePatches(VOID)
{
	TREENODE *p;

	_ASSERT(pbyRom);						// ROM defined
	while (nodePatch != NULL)
	{
		// restore original data
		pbyRom[nodePatch->dwAddress] = nodePatch->byROM;

		p = nodePatch->next;				// save pointer to next node
		free(nodePatch);					// free node
		nodePatch = p;						// new node
	}
	return;
}

VOID UpdatePatches(BOOL bPatch)
{
	TREENODE *p = nodePatch;

	_ASSERT(pbyRom);						// ROM defined
	if (bPatch)								// patch ROM
	{
		if (p)								// something in patch list
		{
			// goto last element in list
			for (; p->next != NULL; p = p->next) {}

			do
			{
				if (!p->bPatch)				// patch only if not patched
				{
					// use original data for patch restore
					p->byROM = pbyRom[p->dwAddress];

					// restore patch data
					pbyRom[p->dwAddress] = p->byPatch;
					p->bPatch = TRUE;		// address patched
				}
				else
				{
					_ASSERT(FALSE);			// call ROM patch on a patched ROM
				}

				p = p->prev;
			}
			while (p != NULL);
		}
	}
	else									// restore ROM
	{
		for (; p != NULL; p = p->next)
		{
			// restore original data
			pbyRom[p->dwAddress] = p->byROM;
			p->bPatch = FALSE;				// address not patched
		}
	}
	return;
}

BOOL PatchRom(LPCTSTR szFilename)
{
	HANDLE hFile = NULL;
	DWORD  dwFileSizeLow = 0;
	DWORD  dwFileSizeHigh = 0;
	DWORD  lBytesRead = 0;
	PSZ    lpStop,lpBuf = NULL;
	DWORD  dwAddress = 0;
	UINT   nPos = 0;
	BOOL   bSucc = TRUE;

	if (pbyRom == NULL) return FALSE;
	SetCurrentDirectory((*szRomDirectory == 0) ? szEmuDirectory : szRomDirectory);
	hFile = CreateFile(szFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	SetCurrentDirectory(szCurrentDirectory);
	if (hFile == INVALID_HANDLE_VALUE) return FALSE;
	dwFileSizeLow = GetFileSize(hFile, &dwFileSizeHigh);
	if (dwFileSizeHigh != 0 || dwFileSizeLow == 0)
	{ // file is too large or empty
		CloseHandle(hFile);
		return FALSE;
	}
	lpBuf = (PSZ) malloc(dwFileSizeLow+1);
	if (lpBuf == NULL)
	{
		CloseHandle(hFile);
		return FALSE;
	}
	ReadFile(hFile, lpBuf, dwFileSizeLow, &lBytesRead, NULL);
	CloseHandle(hFile);
	lpBuf[dwFileSizeLow] = 0;
	nPos = 0;
	while (lpBuf[nPos])
	{
		// skip whitespace characters
		nPos += (UINT) strspn(&lpBuf[nPos]," \t\n\r");

		if (lpBuf[nPos] == ';')				// comment?
		{
			do
			{
				nPos++;
				if (lpBuf[nPos] == '\n')
				{
					nPos++;
					break;
				}
			} while (lpBuf[nPos]);
			continue;
		}
		dwAddress = strtoul(&lpBuf[nPos], &lpStop, 16);
		nPos = (UINT) (lpStop - lpBuf);		// position of lpStop

		if (*lpStop != 0)					// data behind address
		{
			if (*lpStop != ':')				// invalid syntax
			{
				// skip to end of line
				while (lpBuf[nPos] != '\n' && lpBuf[nPos] != 0)
				{
					++nPos;
				}
				bSucc = FALSE;
				continue;
			}

			while (lpBuf[++nPos])
			{
				if (isxdigit(lpBuf[nPos]) == FALSE) break;
				if (dwAddress < dwRomSize)	// patch ROM
				{
					// patch ROM and save original nibble
					PatchNibble(dwAddress, Asc2Nib(lpBuf[nPos]));
				}
				++dwAddress;
			}
		}
	}
	_ASSERT(nPos <= dwFileSizeLow);			// buffer overflow?
	free(lpBuf);
	return bSucc;
}



//################
//#
//#    ROM
//#
//################

BOOL CrcRom(WORD *pwChk)					// calculate fingerprint of ROM
{
	DWORD *pdwData,dwSize;
	DWORD dwChk = 0;

	if (pbyRom == NULL) return TRUE;		// ROM CRC isn't available

	_ASSERT(pbyRom);						// view on ROM
	pdwData = (DWORD *) pbyRom;

	_ASSERT((dwRomSize % sizeof(*pdwData)) == 0);
	dwSize = dwRomSize / sizeof(*pdwData);	// file size in DWORD's

	// use checksum, because it's faster
	while (dwSize-- > 0)
	{
		DWORD dwData = *pdwData++;
		if ((dwData & 0xF0F0F0F0) != 0)		// data packed?
			return FALSE;
		dwChk += dwData;
	}

	*pwChk = (WORD) ((dwChk >> 16) + (dwChk & 0xFFFF));
	return TRUE;
}

BOOL MapRom(LPCTSTR szFilename)
{
	DWORD dwSize,dwFileSize,dwRead;

	// open ROM for writing
	BOOL bRomRW = (cCurrentRomType == 'X' || cCurrentRomType == 'Q') ? bRomWriteable : FALSE;   // CdB for HP: add apples

	if (pbyRom != NULL)
	{
		return FALSE;
	}
	SetCurrentDirectory((*szRomDirectory == 0) ? szEmuDirectory : szRomDirectory);
	if (bRomRW)								// ROM writeable
	{
		hRomFile = CreateFile(szFilename,
							  GENERIC_READ|GENERIC_WRITE,
							  FILE_SHARE_READ,
							  NULL,
							  OPEN_EXISTING,
							  FILE_FLAG_SEQUENTIAL_SCAN,
							  NULL);
		if (hRomFile == INVALID_HANDLE_VALUE)
		{
			bRomRW = FALSE;					// ROM not writeable
			hRomFile = CreateFile(szFilename,
								  GENERIC_READ,
								  FILE_SHARE_READ|FILE_SHARE_WRITE,
								  NULL,
								  OPEN_EXISTING,
								  FILE_FLAG_SEQUENTIAL_SCAN,
								  NULL);
		}
	}
	else									// writing ROM disabled
	{
		hRomFile = CreateFile(szFilename,
							  GENERIC_READ,
							  FILE_SHARE_READ,
							  NULL,
							  OPEN_EXISTING,
							  FILE_FLAG_SEQUENTIAL_SCAN,
							  NULL);
	}
	SetCurrentDirectory(szCurrentDirectory);
	if (hRomFile == INVALID_HANDLE_VALUE)
	{
		hRomFile = NULL;
		return FALSE;
	}
	dwRomSize = GetFileSize(hRomFile, NULL);

	// read the first 4 bytes
	ReadFile(hRomFile,&dwSize,sizeof(dwSize),&dwRead,NULL);
	if (dwRead < sizeof(dwSize))
	{ // file is too small.
		CloseHandle(hRomFile);
		hRomFile = NULL;
		dwRomSize = 0;
		return FALSE;
	}

	dwFileSize = dwRomSize;					// calculate ROM image buffer size
	bRomPacked = (dwSize & 0xF0F0F0F0) != 0; // ROM image packed
	if (bRomPacked)	dwRomSize *= 2;			// unpacked ROM image has double size

	pbyRom = (LPBYTE) malloc(dwRomSize);
	if (pbyRom == NULL)
	{
		CloseHandle(hRomFile);
		hRomFile = NULL;
		dwRomSize = 0;
		return FALSE;
	}

	*(DWORD *) pbyRom = dwSize;				// save first 4 bytes

	// load rest of file content
	ReadFile(hRomFile,&pbyRom[sizeof(dwSize)],dwFileSize - sizeof(dwSize),&dwRead,NULL);
	_ASSERT(dwFileSize - sizeof(dwSize) == dwRead);

	if (bRomRW)								// ROM is writeable
	{
		// no. of dirty pages
		dwRomDirtyPageSize = dwRomSize / ROMPAGESIZE;

		// alloc dirty page table
		pbyRomDirtyPage = (LPBYTE) calloc(dwRomDirtyPageSize,sizeof(*pbyRomDirtyPage));
		if (pbyRomDirtyPage == NULL)
		{
			free(pbyRom);					// free ROM image
			CloseHandle(hRomFile);
			dwRomDirtyPageSize = 0;
			pbyRom = NULL;
			hRomFile = NULL;
			dwRomSize = 0;
			return FALSE;
		}
	}
	else
	{
		dwRomDirtyPageSize = 0;
		CloseHandle(hRomFile);
		hRomFile = NULL;
	}

	if (bRomPacked)							// packed ROM image
	{
		dwSize = dwRomSize;					// destination start address
		while (dwFileSize > 0)				// unpack source
		{
			BYTE byValue = pbyRom[--dwFileSize];
			pbyRom[--dwSize] = byValue >> 4;
			pbyRom[--dwSize] = byValue & 0xF;
		}
	}
	return TRUE;
}

VOID UnmapRom(VOID)
{
	if (pbyRom == NULL) return;				// ROM not mapped
	RestorePatches();						// restore ROM patches
	if (hRomFile)							// ROM file still open (only in R/W case)
	{
		DWORD i;

		_ASSERT(pbyRomDirtyPage != NULL);

		// scan for every dirty page
		for (i = 0; i < dwRomDirtyPageSize; ++i)
		{
			if (pbyRomDirtyPage[i])			// page dirty
			{
				DWORD dwSize,dwLinPos,dwFilePos,dwWritten;

				dwLinPos = i * ROMPAGESIZE;	// position inside emulator memory

				dwSize = ROMPAGESIZE;		// bytes to write
				while (i+1 < dwRomDirtyPageSize && pbyRomDirtyPage[i+1])
				{
					dwSize += ROMPAGESIZE;	// next page is also dirty
					++i;					// skip next page in outer loop
				}

				dwFilePos = dwLinPos;		// ROM file position

				if (bRomPacked)				// repack data
				{
					LPBYTE pbySrc,pbyDest;
					DWORD  j;

					dwSize /= 2;			// adjust no. of bytes to write
					dwFilePos /= 2;			// linear pos in packed file

					// pack data in page
					pbySrc = pbyDest = &pbyRom[dwLinPos];
					for (j = 0; j < dwSize; j++)
					{
						*pbyDest =  *pbySrc++;
						*pbyDest |= *pbySrc++ << 4;
						pbyDest++;
					}
				}

				SetFilePointer(hRomFile,dwFilePos,NULL,FILE_BEGIN);
				WriteFile(hRomFile,&pbyRom[dwLinPos],dwSize,&dwWritten,NULL);
			}
		}

		free(pbyRomDirtyPage);
		CloseHandle(hRomFile);
		pbyRomDirtyPage = NULL;
		dwRomDirtyPageSize = 0;
		hRomFile = NULL;
	}

	free(pbyRom);							// free ROM image
	pbyRom = NULL;
	dwRomSize = 0;
	wRomCrc = 0;
	return;
}



//################
//#
//#    Port2
//#
//################

BOOL CrcPort2(WORD *pwCrc)					// calculate fingerprint of port2
{
	DWORD dwCount;
	DWORD dwFileSize;

	*pwCrc = 0;

	// port2 CRC isn't available
	if (pbyPort2 == NULL) return TRUE;

	dwFileSize = GetFileSize(hPort2File, &dwCount); // get real filesize
	_ASSERT(dwCount == 0);					// isn't created by MapPort2()

	for (dwCount = 0;dwCount < dwFileSize; ++dwCount)
	{
		if ((pbyPort2[dwCount] & 0xF0) != 0) // data packed?
			return FALSE;

		*pwCrc = (*pwCrc >> 4) ^ (((*pwCrc ^ ((WORD) pbyPort2[dwCount])) & 0xf) * 0x1081);
	}
	return TRUE;
}

BOOL MapPort2(LPCTSTR szFilename)
{
	DWORD dwFileSizeLo,dwFileSizeHi;

	if (pbyPort2 != NULL) return FALSE;
	bPort2Writeable = TRUE;
	dwPort2Size = 0;						// reset size of port2

	SetCurrentDirectory(szEmuDirectory);
	hPort2File = CreateFile(szFilename,
							GENERIC_READ|GENERIC_WRITE,
							bPort2IsShared ? FILE_SHARE_READ : 0,
							NULL,
							OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL,
							NULL);
	if (hPort2File == INVALID_HANDLE_VALUE)
	{
		bPort2Writeable = FALSE;
		hPort2File = CreateFile(szFilename,
								GENERIC_READ,
								bPort2IsShared ? (FILE_SHARE_READ|FILE_SHARE_WRITE) : 0,
								NULL,
								OPEN_EXISTING,
								FILE_ATTRIBUTE_NORMAL,
								NULL);
		if (hPort2File == INVALID_HANDLE_VALUE)
		{
			SetCurrentDirectory(szCurrentDirectory);
			hPort2File = NULL;
			return FALSE;
		}
	}
	SetCurrentDirectory(szCurrentDirectory);
	dwFileSizeLo = GetFileSize(hPort2File, &dwFileSizeHi);

	// size not 32, 128, 256, 512, 1024, 2048 or 4096 KB
	if (   dwFileSizeHi != 0
		|| dwFileSizeLo == 0
		|| (dwFileSizeLo & (dwFileSizeLo - 1)) != 0
		|| (dwFileSizeLo & 0xFF02FFFF) != 0)
	{
		UnmapPort2();
		return FALSE;
	}

	hPort2Map = CreateFileMapping(hPort2File, NULL, bPort2Writeable ? PAGE_READWRITE : PAGE_READONLY,
								  0, dwFileSizeLo, NULL);
	if (hPort2Map == NULL)
	{
		UnmapPort2();
		return FALSE;
	}
	pbyPort2 = (LPBYTE) MapViewOfFile(hPort2Map, bPort2Writeable ? FILE_MAP_WRITE : FILE_MAP_READ, 0, 0, dwFileSizeLo);
	if (pbyPort2 == NULL)
	{
		UnmapPort2();
		return FALSE;
	}

	dwPort2Mask = (dwFileSizeLo - 1) >> 18;	// mask for valid address lines of the BS-FF
	dwPort2Size = dwFileSizeLo / 2048;		// mapping size of port2

	if (CrcPort2(&wPort2Crc) == FALSE)		// calculate fingerprint of port2
	{
		UnmapPort2();						// free memory
		AbortMessage(_T("Packed Port 2 image detected!"));
		return FALSE;
	}
	return TRUE;
}

VOID UnmapPort2(VOID)
{
	if (pbyPort2 != NULL)
	{
		UnmapViewOfFile(pbyPort2);
		pbyPort2 = NULL;
	}
	if (hPort2Map != NULL)
	{
		CloseHandle(hPort2Map);
		hPort2Map = NULL;
	}
	if (hPort2File != NULL)
	{
		CloseHandle(hPort2File);
		hPort2File = NULL;
	}
	dwPort2Size = 0;						// reset size of port2
	dwPort2Mask = 0;
	bPort2Writeable = FALSE;
	wPort2Crc = 0;
	return;
}



//################
//#
//#    Documents
//#
//################

static BOOL IsDataPacked(VOID *pMem, DWORD dwSize)
{
	DWORD *pdwMem = (DWORD *) pMem;

	_ASSERT((dwSize % sizeof(DWORD)) == 0);
	if ((dwSize % sizeof(DWORD)) != 0) return TRUE;

	for (dwSize /= sizeof(DWORD); dwSize-- > 0;)
	{
		if ((*pdwMem++ & 0xF0F0F0F0) != 0)
			return TRUE;
	}
	return FALSE;
}

VOID ResetDocument(VOID)
{
	DisableDebugger();
	if (szCurrentKml[0])
	{
		KillKML();
	}
	if (hCurrentFile)
	{
		CloseHandle(hCurrentFile);
		hCurrentFile = NULL;
	}
	szCurrentKml[0] = 0;
	szCurrentFilename[0]=0;
	if (Port0) { free(Port0); Port0 = NULL; }
	if (Port1) { free(Port1); Port1 = NULL; }
	if (Port2) { free(Port2); Port2 = NULL; } else UnmapPort2();
	ZeroMemory(&Chipset,sizeof(Chipset));
	ZeroMemory(&RMap,sizeof(RMap));			// delete MMU mappings
	ZeroMemory(&WMap,sizeof(WMap));
	bDocumentAvail = FALSE;					// document not available
	return;
}

BOOL NewDocument(VOID)
{
	SaveBackup();
	ResetDocument();

	if (!DisplayChooseKml(0)) goto restore;
	if (!InitKML(szCurrentKml,FALSE)) goto restore;
	Chipset.type = cCurrentRomType;
	CrcRom(&Chipset.wRomCrc);				// save fingerprint of loaded ROM

	if (Chipset.type == '6' || Chipset.type == 'A')	// HP38G
	{
		Chipset.Port0Size = (Chipset.type == 'A') ? 32 : 64;
		Chipset.Port1Size = 0;
		Chipset.Port2Size = 0;

		Chipset.cards_status = 0x0;
	}
	if (Chipset.type == 'E' || Chipset.type == 'P')				// HP39/40G/HP39G+   // CdB for HP: add apples
	{
		Chipset.Port0Size = 128;
		Chipset.Port1Size = 0;
		Chipset.Port2Size = 128;

		Chipset.cards_status = 0xF;

		bPort2Writeable = TRUE;				// port2 is writeable
	}
	if (Chipset.type == 'S')				// HP48SX
	{
		Chipset.Port0Size = 32;
		Chipset.Port1Size = 128;
		Chipset.Port2Size = 0;

		Chipset.cards_status = 0x5;

		// use 2nd command line argument if defined
		MapPort2((nArgc < 3) ? szPort2Filename : ppArgv[2]);
	}
	if (Chipset.type == 'G')				// HP48GX
	{
		Chipset.Port0Size = 128;
		Chipset.Port1Size = 128;
		Chipset.Port2Size = 0;

		Chipset.cards_status = 0xA;

		// use 2nd command line argument if defined
		MapPort2((nArgc < 3) ? szPort2Filename : ppArgv[2]);
	}
	if (Chipset.type == 'X' || Chipset.type == '2' || Chipset.type == 'Q')				// HP49G/HP48gII/HP49g+/50g   // CdB for HP: add apples
	{
		Chipset.Port0Size = 256;
		Chipset.Port1Size = 128;
		Chipset.Port2Size = 128;

		Chipset.cards_status = 0xF;
		bPort2Writeable = TRUE;				// port2 is writeable

		FlashInit();						// init flash structure
	}

	if (Chipset.type == 'Q')				// HP49g+/50g   // CdB for HP: add apples
	{
		Chipset.d0size = 16;
	}
	Chipset.IORam[LPE] = RST;				// set ReSeT bit at power on reset

	// allocate port memory
	if (Chipset.Port0Size)
	{
		Port0 = (LPBYTE) calloc(Chipset.Port0Size*2048,sizeof(*Port0));
		_ASSERT(Port0 != NULL);
	}
	if (Chipset.Port1Size)
	{
		Port1 = (LPBYTE) calloc(Chipset.Port1Size*2048,sizeof(*Port1));
		_ASSERT(Port1 != NULL);
	}
	if (Chipset.Port2Size)
	{
		Port2 = (LPBYTE) calloc(Chipset.Port2Size*2048,sizeof(*Port1));
		_ASSERT(Port2 != NULL);
	}
	LoadBreakpointList(NULL);				// clear debugger breakpoint list
	RomSwitch(0);							// boot ROM view of HP49G and map memory
	bDocumentAvail = TRUE;					// document available
	return TRUE;
restore:
	RestoreBackup();
	ResetBackup();

	// HP48SX/GX
	if (Chipset.type == 'S' || Chipset.type == 'G')
	{
		// use 2nd command line argument if defined
		MapPort2((nArgc < 3) ? szPort2Filename : ppArgv[2]);
	}
	if (pbyRom)
	{
		Map(0x00,0xFF);
	}
	return FALSE;
}

BOOL OpenDocument(LPCTSTR szFilename)
{
	#define CHECKAREA(s,e) (offsetof(CHIPSET,e)-offsetof(CHIPSET,s)+sizeof(((CHIPSET *)NULL)->e))

	HANDLE  hFile = INVALID_HANDLE_VALUE;
	DWORD   lBytesRead,lSizeofChipset;
	BYTE    pbyFileSignature[16];
	LPBYTE  pbySig;
	UINT    ctBytesCompared;
	UINT    nLength;

	// Open file
	if (lstrcmpi(szCurrentFilename,szFilename) == 0)
	{
		if (YesNoMessage(_T("Do you want to reload this document?")) == IDNO)
			return TRUE;
	}

	SaveBackup();
	ResetDocument();

	hFile = CreateFile(szFilename, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		AbortMessage(_T("This file is missing or already loaded in another instance of Emu48."));
		goto restore;
	}

	// Read and Compare signature
	ReadFile(hFile, pbyFileSignature, 16, &lBytesRead, NULL);
	switch (pbyFileSignature[0])
	{
	case 'E':
		pbySig = (pbyFileSignature[3] == '3')
			   ? ((pbyFileSignature[4] == '8') ? pbySignatureA : pbySignatureB)
			   : ((pbyFileSignature[4] == '8') ? pbySignatureE : pbySignatureV);
		for (ctBytesCompared=0; ctBytesCompared<14; ctBytesCompared++)
		{
			if (pbyFileSignature[ctBytesCompared]!=pbySig[ctBytesCompared])
			{
				AbortMessage(_T("This file is not a valid Emu48 document."));
				goto restore;
			}
		}
		break;
	case 'W':
		for (ctBytesCompared=0; ctBytesCompared<14; ctBytesCompared++)
		{
			if (pbyFileSignature[ctBytesCompared]!=pbySignatureW[ctBytesCompared])
			{
				AbortMessage(_T("This file is not a valid Win48 document."));
				goto restore;
			}
		}
		break;
	default:
		AbortMessage(_T("This file is not a valid document."));
		goto restore;
	}

	switch (pbyFileSignature[14])
	{
	case 0xFE: // Win48 2.1 / Emu4x 0.99.x format
		// read length of KML script name
		ReadFile(hFile,&nLength,sizeof(nLength),&lBytesRead,NULL);
		// KML script name too long for file buffer
		if (nLength >= ARRAYSIZEOF(szCurrentKml)) goto read_err;
		#if defined _UNICODE
		{
			LPSTR szTmp = (LPSTR) malloc(nLength);
			if (szTmp == NULL)
			{
				AbortMessage(_T("Memory Allocation Failure."));
				goto restore;
			}
			ReadFile(hFile, szTmp, nLength, &lBytesRead, NULL);
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szTmp, lBytesRead,
								szCurrentKml, ARRAYSIZEOF(szCurrentKml));
			free(szTmp);
		}
		#else
		{
			ReadFile(hFile, szCurrentKml, nLength, &lBytesRead, NULL);
		}
		#endif
		if (nLength != lBytesRead) goto read_err;
		szCurrentKml[nLength] = 0;
		break;
	case 0xFF: // Win48 2.05 format
		break;
	default:
		AbortMessage(_T("This file is for an unknown version of Emu48."));
		goto restore;
	}

	// read chipset size inside file
	ReadFile(hFile, &lSizeofChipset, sizeof(lSizeofChipset), &lBytesRead, NULL);
	if (lBytesRead != sizeof(lSizeofChipset)) goto read_err;
	if (lSizeofChipset <= sizeof(Chipset))	// actual or older chipset version
	{
		// read chipset content
		ZeroMemory(&Chipset,sizeof(Chipset));	// init chipset
		ReadFile(hFile, &Chipset, lSizeofChipset, &lBytesRead, NULL);
	}
	else									// newer chipset version
	{
		// read my used chipset content
		ReadFile(hFile, &Chipset, sizeof(Chipset), &lBytesRead, NULL);

		// skip rest of chipset
		SetFilePointer(hFile, lSizeofChipset-sizeof(Chipset), NULL, FILE_CURRENT);
		lSizeofChipset = sizeof(Chipset);
	}
	if (lBytesRead != lSizeofChipset) goto read_err;

	if (!isModelValid(Chipset.type))		// check for valid model in emulator state file
	{
		AbortMessage(_T("Emulator state file with invalid calculator model."));
		goto restore;
	}

	SetWindowLocation(hWnd,Chipset.nPosX,Chipset.nPosY);

	while (TRUE)
	{
		if (szCurrentKml[0])				// KML file name
		{
			BOOL bOK;

			bOK = InitKML(szCurrentKml,FALSE);
			bOK = bOK && (cCurrentRomType == Chipset.type);
			if (bOK) break;

			KillKML();
		}
		if (!DisplayChooseKml(Chipset.type))
			goto restore;
	}
	// reload old button state
	ReloadButtons(Chipset.Keyboard_Row,ARRAYSIZEOF(Chipset.Keyboard_Row));

	FlashInit();							// init flash structure

	if (Chipset.Port0Size)
	{
		Port0 = (LPBYTE) malloc(Chipset.Port0Size*2048);
		if (Port0 == NULL)
		{
			AbortMessage(_T("Memory Allocation Failure."));
			goto restore;
		}

		ReadFile(hFile, Port0, Chipset.Port0Size*2048, &lBytesRead, NULL);
		if (lBytesRead != Chipset.Port0Size*2048) goto read_err;

		if (IsDataPacked(Port0,Chipset.Port0Size*2048)) goto read_err;
	}

	if (Chipset.Port1Size)
	{
		Port1 = (LPBYTE) malloc(Chipset.Port1Size*2048);
		if (Port1 == NULL)
		{
			AbortMessage(_T("Memory Allocation Failure."));
			goto restore;
		}

		ReadFile(hFile, Port1, Chipset.Port1Size*2048, &lBytesRead, NULL);
		if (lBytesRead != Chipset.Port1Size*2048) goto read_err;

		if (IsDataPacked(Port1,Chipset.Port1Size*2048)) goto read_err;
	}

	// HP48SX/GX
	if (cCurrentRomType=='S' || cCurrentRomType=='G')
	{
		MapPort2((nArgc < 3) ? szPort2Filename : ppArgv[2]);
		// port2 changed and card detection enabled
		if (    Chipset.wPort2Crc != wPort2Crc
			&& (Chipset.IORam[CARDCTL] & ECDT) != 0 && (Chipset.IORam[TIMER2_CTRL] & RUN) != 0
		   )
		{
			Chipset.HST |= MP;				// set Module Pulled
			IOBit(SRQ2,NINT,FALSE);			// set NINT to low
			Chipset.SoftInt = TRUE;			// set interrupt
			bInterrupt = TRUE;
		}
	}
	else									// HP38G, HP39/40G, HP49G
	{
		if (Chipset.Port2Size)
		{
			Port2 = (LPBYTE) malloc(Chipset.Port2Size*2048);
			if (Port2 == NULL)
			{
				AbortMessage(_T("Memory Allocation Failure."));
				goto restore;
			}

			ReadFile(hFile, Port2, Chipset.Port2Size*2048, &lBytesRead, NULL);
			if (lBytesRead != Chipset.Port2Size*2048) goto read_err;

			if (IsDataPacked(Port2,Chipset.Port2Size*2048)) goto read_err;

			bPort2Writeable = TRUE;
			Chipset.cards_status = 0xF;
		}
	}

	RomSwitch(Chipset.Bank_FF);				// reload ROM view of HP49G and map memory

	if (Chipset.wRomCrc != wRomCrc)			// ROM changed
	{
		CpuReset();
		Chipset.Shutdn = FALSE;				// automatic restart
	}

	// check CPU main registers
	if (IsDataPacked(Chipset.A,CHECKAREA(A,R4))) goto read_err;

	LoadBreakpointList(hFile);				// load debugger breakpoint list

	lstrcpy(szCurrentFilename, szFilename);
	_ASSERT(hCurrentFile == NULL);
	hCurrentFile = hFile;
	#if defined _USRDLL						// DLL version
		// notify main proc about current document file
		if (pEmuDocumentNotify) pEmuDocumentNotify(szCurrentFilename);
	#endif
	SetWindowPathTitle(szCurrentFilename);	// update window title line
	bDocumentAvail = TRUE;					// document available
	return TRUE;

read_err:
	AbortMessage(_T("This file must be truncated, and cannot be loaded."));
restore:
	if (INVALID_HANDLE_VALUE != hFile)		// close if valid handle
		CloseHandle(hFile);
	RestoreBackup();
	ResetBackup();

	// HP48SX/GX
	if (cCurrentRomType=='S' || cCurrentRomType=='G')
	{
		// use 2nd command line argument if defined
		MapPort2((nArgc < 3) ? szPort2Filename : ppArgv[2]);
	}
	return FALSE;
	#undef CHECKAREA
}

BOOL SaveDocument(VOID)
{
	DWORD           lBytesWritten;
	DWORD           lSizeofChipset;
	UINT            nLength;
	WINDOWPLACEMENT wndpl;

	if (hCurrentFile == NULL) return FALSE;

	_ASSERT(hWnd);							// window open
	wndpl.length = sizeof(wndpl);			// update saved window position
	GetWindowPlacement(hWnd, &wndpl);
	Chipset.nPosX = (SWORD) wndpl.rcNormalPosition.left;
	Chipset.nPosY = (SWORD) wndpl.rcNormalPosition.top;

	SetFilePointer(hCurrentFile,0,NULL,FILE_BEGIN);

	if (!WriteFile(hCurrentFile, pbySignatureE, sizeof(pbySignatureE), &lBytesWritten, NULL))
	{
		AbortMessage(_T("Could not write into file !"));
		return FALSE;
	}

	CrcRom(&Chipset.wRomCrc);				// save fingerprint of ROM
	CrcPort2(&Chipset.wPort2Crc);			// save fingerprint of port2

	nLength = lstrlen(szCurrentKml);
	WriteFile(hCurrentFile, &nLength, sizeof(nLength), &lBytesWritten, NULL);
	#if defined _UNICODE
	{
		LPSTR szTmp = (LPSTR) malloc(nLength);
		if (szTmp != NULL)
		{
			WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK,
								szCurrentKml, nLength,
								szTmp, nLength, NULL, NULL);
			WriteFile(hCurrentFile, szTmp, nLength, &lBytesWritten, NULL);
			free(szTmp);
		}
	}
	#else
	{
		WriteFile(hCurrentFile, szCurrentKml, nLength, &lBytesWritten, NULL);
	}
	#endif
	lSizeofChipset = sizeof(CHIPSET);
	WriteFile(hCurrentFile, &lSizeofChipset, sizeof(lSizeofChipset), &lBytesWritten, NULL);
	WriteFile(hCurrentFile, &Chipset, lSizeofChipset, &lBytesWritten, NULL);
	if (Chipset.Port0Size) WriteFile(hCurrentFile, Port0, Chipset.Port0Size*2048, &lBytesWritten, NULL);
	if (Chipset.Port1Size) WriteFile(hCurrentFile, Port1, Chipset.Port1Size*2048, &lBytesWritten, NULL);
	if (Chipset.Port2Size) WriteFile(hCurrentFile, Port2, Chipset.Port2Size*2048, &lBytesWritten, NULL);
	SaveBreakpointList(hCurrentFile);		// save debugger breakpoint list
	SetEndOfFile(hCurrentFile);				// cut the rest
	return TRUE;
}

BOOL SaveDocumentAs(LPCTSTR szFilename)
{
	HANDLE hFile;

	if (hCurrentFile)						// already file in use
	{
		CloseHandle(hCurrentFile);			// close it, even it's same, so data always will be saved
		hCurrentFile = NULL;
	}
	hFile = CreateFile(szFilename, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)		// error, couldn't create a new file
	{
		AbortMessage(_T("This file must be currently used by another instance of Emu48."));
		return FALSE;
	}
	lstrcpy(szCurrentFilename, szFilename);	// save new file name
	hCurrentFile = hFile;					// and the corresponding handle
	#if defined _USRDLL						// DLL version
		// notify main proc about current document file
		if (pEmuDocumentNotify) pEmuDocumentNotify(szCurrentFilename);
	#endif
	SetWindowPathTitle(szCurrentFilename);	// update window title line
	return SaveDocument();					// save current content
}



//################
//#
//#    Backup
//#
//################

BOOL SaveBackup(VOID)
{
	WINDOWPLACEMENT wndpl;

	if (!bDocumentAvail) return FALSE;

	_ASSERT(nState != SM_RUN);				// emulation engine is running

	// save window position
	_ASSERT(hWnd);							// window open
	wndpl.length = sizeof(wndpl);			// update saved window position
	GetWindowPlacement(hWnd, &wndpl);
	Chipset.nPosX = (SWORD) wndpl.rcNormalPosition.left;
	Chipset.nPosY = (SWORD) wndpl.rcNormalPosition.top;

	lstrcpy(szBackupFilename, szCurrentFilename);
	lstrcpy(szBackupKml, szCurrentKml);
	if (BackupPort0) { free(BackupPort0); BackupPort0 = NULL; }
	if (BackupPort1) { free(BackupPort1); BackupPort1 = NULL; }
	if (BackupPort2) { free(BackupPort2); BackupPort2 = NULL; }
	CopyMemory(&BackupChipset, &Chipset, sizeof(Chipset));
	if (Port0 && Chipset.Port0Size)
	{
		BackupPort0 = (LPBYTE) malloc(Chipset.Port0Size*2048);
		CopyMemory(BackupPort0,Port0,Chipset.Port0Size*2048);
	}
	if (Port1 && Chipset.Port1Size)
	{
		BackupPort1 = (LPBYTE) malloc(Chipset.Port1Size*2048);
		CopyMemory(BackupPort1,Port1,Chipset.Port1Size*2048);
	}
	if (Port2 && Chipset.Port2Size)			// internal port2
	{
		BackupPort2 = (LPBYTE) malloc(Chipset.Port2Size*2048);
		CopyMemory(BackupPort2,Port2,Chipset.Port2Size*2048);
	}
	CreateBackupBreakpointList();
	bBackup = TRUE;
	return TRUE;
}

BOOL RestoreBackup(VOID)
{
	BOOL bDbgOpen;

	if (!bBackup) return FALSE;

	bDbgOpen = (nDbgState != DBG_OFF);		// debugger window open?
	ResetDocument();
	// need chipset for contrast setting in InitKML()
	Chipset.contrast = BackupChipset.contrast;
	if (!InitKML(szBackupKml,TRUE))
	{
		InitKML(szCurrentKml,TRUE);
		return FALSE;
	}
	lstrcpy(szCurrentKml, szBackupKml);
	lstrcpy(szCurrentFilename, szBackupFilename);
	if (szCurrentFilename[0])
	{
		hCurrentFile = CreateFile(szCurrentFilename, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (hCurrentFile == INVALID_HANDLE_VALUE)
		{
			hCurrentFile = NULL;
			szCurrentFilename[0] = 0;
		}
	}
	CopyMemory(&Chipset, &BackupChipset, sizeof(Chipset));
	if (BackupPort0 && Chipset.Port0Size)
	{
		Port0 = (LPBYTE) malloc(Chipset.Port0Size*2048);
		CopyMemory(Port0,BackupPort0,Chipset.Port0Size*2048);
	}
	if (BackupPort1 && Chipset.Port1Size)
	{
		Port1 = (LPBYTE) malloc(Chipset.Port1Size*2048);
		CopyMemory(Port1,BackupPort1,Chipset.Port1Size*2048);
	}
	if (BackupPort2 && Chipset.Port2Size)	// internal port2
	{
		Port2 = (LPBYTE) malloc(Chipset.Port2Size*2048);
		CopyMemory(Port2,BackupPort2,Chipset.Port2Size*2048);
	}
	// map port2
	else
	{
		if (cCurrentRomType=='S' || cCurrentRomType=='G') // HP48SX/GX
		{
			// use 2nd command line argument if defined
			MapPort2((nArgc < 3) ? szPort2Filename : ppArgv[2]);
		}
	}
	Map(0x00,0xFF);							// update memory mapping
	SetWindowPathTitle(szCurrentFilename);	// update window title line
	SetWindowLocation(hWnd,Chipset.nPosX,Chipset.nPosY);
	RestoreBackupBreakpointList();			// restore the debugger breakpoint list
	if (bDbgOpen) OnToolDebug();			// reopen the debugger
	bDocumentAvail = TRUE;					// document available
	return TRUE;
}

BOOL ResetBackup(VOID)
{
	if (!bBackup) return FALSE;
	szBackupFilename[0] = 0;
	szBackupKml[0] = 0;
	if (BackupPort0) { free(BackupPort0); BackupPort0 = NULL; }
	if (BackupPort1) { free(BackupPort1); BackupPort1 = NULL; }
	if (BackupPort2) { free(BackupPort2); BackupPort2 = NULL; }
	ZeroMemory(&BackupChipset,sizeof(BackupChipset));
	bBackup = FALSE;
	return TRUE;
}



//################
//#
//#    Open File Common Dialog Boxes
//#
//################

static VOID InitializeOFN(LPOPENFILENAME ofn)
{
	ZeroMemory((LPVOID)ofn, sizeof(OPENFILENAME));
	ofn->lStructSize = sizeof(OPENFILENAME);
	ofn->hwndOwner = hWnd;
	ofn->Flags = OFN_EXPLORER|OFN_HIDEREADONLY;
	return;
}

BOOL GetOpenFilename(VOID)
{
	TCHAR szBuffer[ARRAYSIZEOF(szBufferFilename)];
	OPENFILENAME ofn;

	InitializeOFN(&ofn);
	ofn.lpstrFilter =
		_T("Emu48 Files (*.e38;*.e39;*.e48;*.e49)\0")
		  _T("*.e38;*.e39;*.e48;*.e49\0")
		_T("HP-38 Files (*.e38)\0*.e38\0")
		_T("HP-39 Files (*.e39)\0*.e39\0")
		_T("HP-48 Files (*.e48)\0*.e48\0")
		_T("HP-49 Files (*.e49)\0*.e49\0")
		_T("Win48 Files (*.w48)\0*.w48\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = szBuffer;
	ofn.lpstrFile[0] = 0;
	ofn.nMaxFile = ARRAYSIZEOF(szBuffer);
	ofn.Flags |= OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
	if (GetOpenFileName(&ofn) == FALSE) return FALSE;
	_ASSERT(ARRAYSIZEOF(szBufferFilename) == ofn.nMaxFile);
	lstrcpy(szBufferFilename, ofn.lpstrFile);
	return TRUE;
}

BOOL GetSaveAsFilename(VOID)
{
	TCHAR szBuffer[ARRAYSIZEOF(szBufferFilename)];
	OPENFILENAME ofn;

	InitializeOFN(&ofn);
	ofn.lpstrFilter =
		_T("HP-38 Files (*.e38)\0*.e38\0")
		_T("HP-39 Files (*.e39)\0*.e39\0")
		_T("HP-48 Files (*.e48)\0*.e48\0")
		_T("HP-49 Files (*.e49)\0*.e49\0");
	ofn.lpstrDefExt = _T("e48");			// HP48SX/GX
	ofn.nFilterIndex = 3;
	if (cCurrentRomType=='6' || cCurrentRomType=='A') // HP38G
	{
		ofn.lpstrDefExt = _T("e38");
		ofn.nFilterIndex = 1;
	}
	if (cCurrentRomType=='E' || cCurrentRomType=='P')				// HP39/40   // CdB for HP: add apples
	{
		ofn.lpstrDefExt = _T("e39");
		ofn.nFilterIndex = 2;
	}
	if (cCurrentRomType=='X' || cCurrentRomType=='2' || cCurrentRomType=='Q')				// HP49G/HP48gII/HP49g+/HP50g   // CdB for HP: add apples
	{
		ofn.lpstrDefExt = _T("e49");
		ofn.nFilterIndex = 4;
	}
	ofn.lpstrFile = szBuffer;
	ofn.lpstrFile[0] = 0;
	ofn.nMaxFile = ARRAYSIZEOF(szBuffer);
	ofn.Flags |= OFN_CREATEPROMPT|OFN_OVERWRITEPROMPT;
	if (GetSaveFileName(&ofn) == FALSE) return FALSE;
	_ASSERT(ARRAYSIZEOF(szBufferFilename) == ofn.nMaxFile);
	lstrcpy(szBufferFilename, ofn.lpstrFile);
	return TRUE;
}

BOOL GetLoadObjectFilename(LPCTSTR lpstrFilter,LPCTSTR lpstrDefExt)
{
	TCHAR szBuffer[ARRAYSIZEOF(szBufferFilename)];
	OPENFILENAME ofn;

	InitializeOFN(&ofn);
	ofn.lpstrFilter = lpstrFilter;
	ofn.lpstrDefExt = lpstrDefExt;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = szBuffer;
	ofn.lpstrFile[0] = 0;
	ofn.nMaxFile = ARRAYSIZEOF(szBuffer);
	ofn.Flags |= OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
	if (GetOpenFileName(&ofn) == FALSE) return FALSE;
	_ASSERT(ARRAYSIZEOF(szBufferFilename) == ofn.nMaxFile);
	lstrcpy(szBufferFilename, ofn.lpstrFile);
	return TRUE;
}

BOOL GetSaveObjectFilename(LPCTSTR lpstrFilter,LPCTSTR lpstrDefExt)
{
	TCHAR szBuffer[ARRAYSIZEOF(szBufferFilename)];
	OPENFILENAME ofn;

	InitializeOFN(&ofn);
	ofn.lpstrFilter = lpstrFilter;
	ofn.lpstrDefExt = NULL;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = szBuffer;
	ofn.lpstrFile[0] = 0;
	ofn.nMaxFile = ARRAYSIZEOF(szBuffer);
	ofn.Flags |= OFN_CREATEPROMPT|OFN_OVERWRITEPROMPT;
	if (GetSaveFileName(&ofn) == FALSE) return FALSE;
	_ASSERT(ARRAYSIZEOF(szBufferFilename) == ofn.nMaxFile);
	lstrcpy(szBufferFilename, ofn.lpstrFile);
	if (ofn.nFileExtension == 0)			// given filename has no extension
	{
		// actual name length
		UINT nLength = lstrlen(szBufferFilename);

		// destination buffer has room for the default extension
		if (nLength + 1 + lstrlen(lpstrDefExt) < ARRAYSIZEOF(szBufferFilename))
		{
			// add default extension
			szBufferFilename[nLength++] = _T('.');
			lstrcpy(&szBufferFilename[nLength], lpstrDefExt);
		}
	}
	return TRUE;
}



//################
//#
//#    Load and Save HP48 Objects
//#
//################

WORD WriteStack(UINT nStkLevel,LPBYTE lpBuf,DWORD dwSize)	// separated from LoadObject()
{
	BOOL  bBinary;
	DWORD dwAddress, i;

	bBinary =  ((lpBuf[dwSize+0]=='H')
	        &&  (lpBuf[dwSize+1]=='P')
	        &&  (lpBuf[dwSize+2]=='H')
	        &&  (lpBuf[dwSize+3]=='P')
	        &&  (lpBuf[dwSize+4]=='4')
			&&  (lpBuf[dwSize+5]==((cCurrentRomType=='X' || cCurrentRomType=='2' || cCurrentRomType=='Q') ? '9' : '8'))  // CdB for HP: add apples
	        &&  (lpBuf[dwSize+6]=='-'));

	for (dwAddress = 0, i = 0; i < dwSize; i++)
	{
		BYTE byTwoNibs = lpBuf[i+dwSize];
		lpBuf[dwAddress++] = (BYTE)(byTwoNibs&0xF);
		lpBuf[dwAddress++] = (BYTE)(byTwoNibs>>4);
	}

	dwSize = dwAddress;						// unpacked buffer size

	if (bBinary == TRUE)
	{ // load as binary
		dwSize = RPL_ObjectSize(lpBuf+16,dwSize-16);
		if (dwSize == BAD_OB) return S_ERR_OBJECT;
		dwAddress = RPL_CreateTemp(dwSize,TRUE);
		if (dwAddress == 0) return S_ERR_BINARY;
		Nwrite(lpBuf+16,dwAddress,dwSize);
	}
	else
	{ // load as string
		dwAddress = RPL_CreateTemp(dwSize+10,TRUE);
		if (dwAddress == 0) return S_ERR_ASCII;
		Write5(dwAddress,0x02A2C);			// String
		Write5(dwAddress+5,dwSize+5);		// length of String
		Nwrite(lpBuf,dwAddress+10,dwSize);	// data
	}
	RPL_Push(nStkLevel,dwAddress);
	return S_ERR_NO;
}

BOOL LoadObject(LPCTSTR szFilename)			// separated stack writing part
{
	HANDLE hFile;
	DWORD  dwFileSizeLow;
	DWORD  dwFileSizeHigh;
	LPBYTE lpBuf;
	WORD   wError;

	hFile = CreateFile(szFilename,
					   GENERIC_READ,
					   FILE_SHARE_READ,
					   NULL,
					   OPEN_EXISTING,
					   FILE_FLAG_SEQUENTIAL_SCAN,
					   NULL);
	if (hFile == INVALID_HANDLE_VALUE) return FALSE;
	dwFileSizeLow = GetFileSize(hFile, &dwFileSizeHigh);
	if (dwFileSizeHigh != 0)
	{ // file is too large.
		CloseHandle(hFile);
		return FALSE;
	}
	lpBuf = (LPBYTE) malloc(dwFileSizeLow*2);
	if (lpBuf == NULL)
	{
		CloseHandle(hFile);
		return FALSE;
	}
	ReadFile(hFile, lpBuf+dwFileSizeLow, dwFileSizeLow, &dwFileSizeHigh, NULL);
	CloseHandle(hFile);

	wError = WriteStack(1,lpBuf,dwFileSizeLow);

	if (wError == S_ERR_OBJECT)
		AbortMessage(_T("This isn't a valid binary file."));

	if (wError == S_ERR_BINARY)
		AbortMessage(_T("The calculator does not have enough\nfree memory to load this binary file."));

	if (wError == S_ERR_ASCII)
		AbortMessage(_T("The calculator does not have enough\nfree memory to load this text file."));

	free(lpBuf);
	return (wError == S_ERR_NO);
}

BOOL SaveObject(LPCTSTR szFilename)			// separated stack reading part
{
	HANDLE hFile;
	LPBYTE pbyHeader;
	DWORD  lBytesWritten;
	DWORD  dwAddress;
	DWORD  dwLength;

	dwAddress = RPL_Pick(1);
	if (dwAddress == 0)
	{
		AbortMessage(_T("Too Few Arguments."));
		return FALSE;
	}
	dwLength = (RPL_SkipOb(dwAddress) - dwAddress + 1) / 2;

	hFile = CreateFile(szFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		AbortMessage(_T("Cannot open file."));
		return FALSE;
	}

	pbyHeader = ((cCurrentRomType=='X' || cCurrentRomType=='2' || cCurrentRomType=='Q'))
			  ? (LPBYTE) BINARYHEADER49
			  : (LPBYTE) BINARYHEADER48;

			  WriteFile(hFile, pbyHeader, 8, &lBytesWritten, NULL);

	while (dwLength--)
	{
		BYTE byByte = Read2(dwAddress);
		WriteFile(hFile, &byByte, 1, &lBytesWritten, NULL);
		dwAddress += 2;
	}
	CloseHandle(hFile);
	return TRUE;
}



//################
//#
//#    Load Icon
//#
//################

BOOL LoadIconFromFile(LPCTSTR szFilename)
{
	HANDLE hIcon;

	SetCurrentDirectory(szEmuDirectory);
	// not necessary to destroy because icon is shared
	hIcon = LoadImage(NULL, szFilename, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE|LR_LOADFROMFILE|LR_SHARED);
	SetCurrentDirectory(szCurrentDirectory);

	if (hIcon)
	{
		SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM) hIcon);
		SendMessage(hWnd, WM_SETICON, ICON_BIG,   (LPARAM) hIcon);
	}
	return hIcon != NULL;
}

VOID LoadIconDefault(VOID)
{
	// use window class icon
	SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM) NULL);
	SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM) NULL);
	return;
}



//################
//#
//#    Load Bitmap
//#
//################

#define WIDTHBYTES(bits) ((((bits) + 31) / 32) * 4)

typedef struct _BmpFile
{
	DWORD  dwPos;							// actual reading pos
	DWORD  dwFileSize;						// file size
	LPBYTE pbyFile;							// buffer
} BMPFILE, FAR *LPBMPFILE, *PBMPFILE;

static __inline WORD DibNumColors(BITMAPINFOHEADER CONST *lpbi)
{
	if (lpbi->biClrUsed != 0) return (WORD) lpbi->biClrUsed;

	/* a 24 bitcount DIB has no color table */
	return (lpbi->biBitCount <= 8) ? (1 << lpbi->biBitCount) : 0;
}

static HPALETTE CreateBIPalette(BITMAPINFOHEADER CONST *lpbi)
{
	LOGPALETTE* pPal;
	HPALETTE    hpal = NULL;
	WORD        wNumColors;
	BYTE        red;
	BYTE        green;
	BYTE        blue;
	UINT        i;
	RGBQUAD*    pRgb;

	if (!lpbi)
		return NULL;

	if (lpbi->biSize != sizeof(BITMAPINFOHEADER))
		return NULL;

	// Get a pointer to the color table and the number of colors in it
	pRgb = (RGBQUAD FAR *)((LPBYTE)lpbi + (WORD)lpbi->biSize);
	wNumColors = DibNumColors(lpbi);

	if (wNumColors)
	{
		// Allocate for the logical palette structure
		pPal = (PLOGPALETTE) malloc(sizeof(LOGPALETTE) + wNumColors * sizeof(PALETTEENTRY));
		if (!pPal) return NULL;

		pPal->palVersion    = 0x300;
		pPal->palNumEntries = wNumColors;

		// Fill in the palette entries from the DIB color table and
		// create a logical color palette.
		for (i = 0; i < pPal->palNumEntries; i++)
		{
			pPal->palPalEntry[i].peRed   = pRgb[i].rgbRed;
			pPal->palPalEntry[i].peGreen = pRgb[i].rgbGreen;
			pPal->palPalEntry[i].peBlue  = pRgb[i].rgbBlue;
			pPal->palPalEntry[i].peFlags = 0;
		}
		hpal = CreatePalette(pPal);
		free(pPal);
	}
	else
	{
		// create halftone palette for 16, 24 and 32 bitcount bitmaps

		// 16, 24 and 32 bitcount DIB's have no color table entries so, set the
		// number of to the maximum value (256).
		wNumColors = 256;
		pPal = (PLOGPALETTE) malloc(sizeof(LOGPALETTE) + wNumColors * sizeof(PALETTEENTRY));
		if (!pPal) return NULL;

		pPal->palVersion    = 0x300;
		pPal->palNumEntries = wNumColors;

		red = green = blue = 0;

		// Generate 256 (= 8*8*4) RGB combinations to fill the palette
		// entries.
		for (i = 0; i < pPal->palNumEntries; i++)
		{
			pPal->palPalEntry[i].peRed   = red;
			pPal->palPalEntry[i].peGreen = green;
			pPal->palPalEntry[i].peBlue  = blue;
			pPal->palPalEntry[i].peFlags = 0;

			if (!(red += 32))
				if (!(green += 32))
					blue += 64;
		}
		hpal = CreatePalette(pPal);
		free(pPal);
	}
	return hpal;
}

static HBITMAP DecodeBmp(LPBMPFILE pBmp,BOOL bPalette)
{
	LPBITMAPFILEHEADER pBmfh;
	LPBITMAPINFO pBmi;
	HBITMAP hBitmap;
	DWORD   dwFileSize;

	hBitmap = NULL;

	// size of bitmap header information
	dwFileSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	if (pBmp->dwFileSize < dwFileSize) return NULL;

	// check for bitmap
	pBmfh = (LPBITMAPFILEHEADER) pBmp->pbyFile;
	if (pBmfh->bfType != 0x4D42) return NULL; // "BM"

	pBmi = (LPBITMAPINFO) (pBmp->pbyFile + sizeof(BITMAPFILEHEADER));

	// size with color table
	if (pBmi->bmiHeader.biCompression == BI_BITFIELDS)
	{
		dwFileSize += 3 * sizeof(DWORD);
	}
	else
	{
		dwFileSize += DibNumColors(&pBmi->bmiHeader) * sizeof(RGBQUAD);
	}
	if (dwFileSize != pBmfh->bfOffBits) return NULL;

	// size with bitmap data
	if (pBmi->bmiHeader.biCompression != BI_RGB)
	{
		dwFileSize += pBmi->bmiHeader.biSizeImage;
	}
	else
	{
		dwFileSize += WIDTHBYTES(pBmi->bmiHeader.biWidth * pBmi->bmiHeader.biBitCount)
			        * labs(pBmi->bmiHeader.biHeight);
	}
	if (pBmp->dwFileSize < dwFileSize) return NULL;

	VERIFY(hBitmap = CreateDIBitmap(
		hWindowDC,
		&pBmi->bmiHeader,
		CBM_INIT,
		pBmp->pbyFile + pBmfh->bfOffBits,
		pBmi, DIB_RGB_COLORS));
	if (hBitmap == NULL) return NULL;

	if (bPalette && hPalette == NULL)
	{
		hPalette = CreateBIPalette(&pBmi->bmiHeader);
		// save old palette
		hOldPalette = SelectPalette(hWindowDC, hPalette, FALSE);
		RealizePalette(hWindowDC);
	}
	return hBitmap;
}

static BOOL ReadGifByte(LPBMPFILE pGif, INT *n)
{
	// outside GIF file
	if (pGif->dwPos >= pGif->dwFileSize)
		return TRUE;

	*n = pGif->pbyFile[pGif->dwPos++];
	return FALSE;
}

static BOOL ReadGifWord(LPBMPFILE pGif, INT *n)
{
	// outside GIF file
	if (pGif->dwPos + 1 >= pGif->dwFileSize)
		return TRUE;

	*n = pGif->pbyFile[pGif->dwPos++];
	*n |= (pGif->pbyFile[pGif->dwPos++] << 8);
	return FALSE;
}

static HBITMAP DecodeGif(LPBMPFILE pBmp,DWORD *pdwTransparentColor,BOOL bPalette)
{
	// this implementation base on the GIF image file
	// decoder engine of Free42 (c) by Thomas Okken

	HBITMAP hBitmap;

	typedef struct cmap
	{
		WORD    biBitCount;					// bits used in color map
		DWORD   biClrUsed;					// no of colors in color map
		RGBQUAD bmiColors[256];				// color map
	} CMAP;

	BOOL    bHasGlobalCmap;
	CMAP    sGlb;							// data of global color map

	INT     nWidth,nHeight,nInfo,nBackground,nZero;
	LONG    lBytesPerLine;

	LPBYTE  pbyPixels;

	BITMAPINFO bmi;							// global bitmap info

	BOOL bDecoding = TRUE;

	hBitmap = NULL;

	pBmp->dwPos = 6;						// position behind GIF header

	/* Bits 6..4 of info contain one less than the "color resolution",
	 * defined as the number of significant bits per RGB component in
	 * the source image's color palette. If the source image (from
	 * which the GIF was generated) was 24-bit true color, the color
	 * resolution is 8, so the value in bits 6..4 is 7. If the source
	 * image had an EGA color cube (2x2x2), the color resolution would
	 * be 2, etc.
	 * Bit 3 of info must be zero in GIF87a; in GIF89a, if it is set,
	 * it indicates that the global colormap is sorted, the most
	 * important entries being first. In PseudoColor environments this
	 * can be used to make sure to get the most important colors from
	 * the X server first, to optimize the image's appearance in the
	 * event that not all the colors from the colormap can actually be
	 * obtained at the same time.
	 * The 'zero' field is always 0 in GIF87a; in GIF89a, it indicates
	 * the pixel aspect ratio, as (PAR + 15) : 64. If PAR is zero,
	 * this means no aspect ratio information is given, PAR = 1 means
	 * 1:4 (narrow), PAR = 49 means 1:1 (square), PAR = 255 means
	 * slightly over 4:1 (wide), etc.
	 */

	if (   ReadGifWord(pBmp,&nWidth)
		|| ReadGifWord(pBmp,&nHeight)
		|| ReadGifByte(pBmp,&nInfo)
		|| ReadGifByte(pBmp,&nBackground)
		|| ReadGifByte(pBmp,&nZero)
		|| nZero != 0)
		goto quit;

	ZeroMemory(&bmi,sizeof(bmi));			// init bitmap info
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = nWidth;
	bmi.bmiHeader.biHeight = nHeight;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 24;			// create a true color DIB
	bmi.bmiHeader.biCompression = BI_RGB;

	ZeroMemory(&sGlb,sizeof(sGlb));			// init global color map
	bHasGlobalCmap = (nInfo & 0x80) != 0;

	sGlb.biBitCount = (nInfo & 7) + 1;		// bits used in global color map
	sGlb.biClrUsed = (1 << sGlb.biBitCount); // no of colors in global color map

	// color table should not exceed 256 colors
	_ASSERT(sGlb.biClrUsed <= ARRAYSIZEOF(sGlb.bmiColors));

	if (bHasGlobalCmap)						// global color map
	{
		DWORD i;

		for (i = 0; i < sGlb.biClrUsed; ++i)
		{
			int r, g, b;

			if (ReadGifByte(pBmp,&r) || ReadGifByte(pBmp,&g) || ReadGifByte(pBmp,&b))
				goto quit;

			sGlb.bmiColors[i].rgbRed   = r;
			sGlb.bmiColors[i].rgbGreen = g;
			sGlb.bmiColors[i].rgbBlue  = b;
		}
	}
	else									// no color map
	{
		DWORD i;

		for (i = 0; i < sGlb.biClrUsed; ++i)
		{
			BYTE k = (BYTE) ((i * sGlb.biClrUsed) / (sGlb.biClrUsed - 1));

			sGlb.bmiColors[i].rgbRed   = k;
			sGlb.bmiColors[i].rgbGreen = k;
			sGlb.bmiColors[i].rgbBlue  = k;
		}
	}

	// bitmap dimensions
	lBytesPerLine = WIDTHBYTES(bmi.bmiHeader.biWidth * bmi.bmiHeader.biBitCount);
	bmi.bmiHeader.biSizeImage = lBytesPerLine * bmi.bmiHeader.biHeight;

	// create top-down DIB
	bmi.bmiHeader.biHeight = -bmi.bmiHeader.biHeight;

	// allocate buffer for pixels
	VERIFY(hBitmap = CreateDIBSection(hWindowDC,
									  &bmi,
									  DIB_RGB_COLORS,
									  (VOID **)&pbyPixels,
									  NULL,
									  0));
	if (hBitmap == NULL) goto quit;

	// fill pixel buffer with background color
	for (nHeight = 0; nHeight < labs(bmi.bmiHeader.biHeight); ++nHeight)
	{
		LPBYTE pbyLine = pbyPixels + nHeight * lBytesPerLine;

		for (nWidth = 0; nWidth < bmi.bmiHeader.biWidth; ++nWidth)
		{
			*pbyLine++ = sGlb.bmiColors[nBackground].rgbBlue;
			*pbyLine++ = sGlb.bmiColors[nBackground].rgbGreen;
			*pbyLine++ = sGlb.bmiColors[nBackground].rgbRed;
		}

		_ASSERT((DWORD) (pbyLine - pbyPixels) <= bmi.bmiHeader.biSizeImage);
	}

	while (bDecoding)
	{
		INT nBlockType;

		if (ReadGifByte(pBmp,&nBlockType)) goto quit;

		switch (nBlockType)
		{
		case ',': // image
			{
				CMAP sAct;					// data of actual color map

				INT  nLeft,nTop,nWidth,nHeight;
				INT  i,nInfo;

				BOOL bInterlaced;
				INT h,v;
				INT nCodesize;				// LZW codesize in bits
				INT nBytecount;

				SHORT prefix_table[4096];
				SHORT code_table[4096];

				INT  nMaxcode;
				INT  nClearCode;
				INT  nEndCode;

				INT  nCurrCodesize;
				INT  nCurrCode;
				INT  nOldCode;
				INT  nBitsNeeded;
				BOOL bEndCodeSeen;

				// read image dimensions
				if (   ReadGifWord(pBmp,&nLeft)
					|| ReadGifWord(pBmp,&nTop)
					|| ReadGifWord(pBmp,&nWidth)
					|| ReadGifWord(pBmp,&nHeight)
					|| ReadGifByte(pBmp,&nInfo))
					goto quit;

				if (   nTop + nHeight > labs(bmi.bmiHeader.biHeight)
					|| nLeft + nWidth > bmi.bmiHeader.biWidth)
					goto quit;

				/* Bit 3 of info must be zero in GIF87a; in GIF89a, if it
				 * is set, it indicates that the local colormap is sorted,
				 * the most important entries being first. In PseudoColor
				 * environments this can be used to make sure to get the
				 * most important colors from the X server first, to
				 * optimize the image's appearance in the event that not
				 * all the colors from the colormap can actually be
				 * obtained at the same time.
				 */

				if ((nInfo & 0x80) == 0)	// using global color map
				{
					sAct = sGlb;
				}
				else						// using local color map
				{
					DWORD i;

					sAct.biBitCount = (nInfo & 7) + 1;	// bits used in color map
					sAct.biClrUsed = (1 << sAct.biBitCount); // no of colors in color map

					for (i = 0; i < sAct.biClrUsed; ++i)
					{
						int r, g, b;

						if (ReadGifByte(pBmp,&r) || ReadGifByte(pBmp,&g) || ReadGifByte(pBmp,&b))
							goto quit;

						sAct.bmiColors[i].rgbRed   = r;
						sAct.bmiColors[i].rgbGreen = g;
						sAct.bmiColors[i].rgbBlue  = b;
					}
				}

				// interlaced image
				bInterlaced = (nInfo & 0x40) != 0;

				h = 0;
				v = 0;
				if (   ReadGifByte(pBmp,&nCodesize)
					|| ReadGifByte(pBmp,&nBytecount))
					goto quit;

				nMaxcode = (1 << nCodesize);

				// preset LZW table
				for (i = 0; i < nMaxcode + 2; ++i)
				{
					prefix_table[i] = -1;
					code_table[i] = i;
				}
				nClearCode = nMaxcode++;
				nEndCode = nMaxcode++;

				nCurrCodesize = nCodesize + 1;
				nCurrCode = 0;
				nOldCode = -1;
				nBitsNeeded = nCurrCodesize;
				bEndCodeSeen = FALSE;

				while (nBytecount != 0)
				{
					for (i = 0; i < nBytecount; ++i)
					{
						INT nCurrByte;
						INT nBitsAvailable;

						if (ReadGifByte(pBmp,&nCurrByte))
							goto quit;

						if (bEndCodeSeen) continue;

						nBitsAvailable = 8;
						while (nBitsAvailable != 0)
						{
							INT nBitsCopied = (nBitsNeeded < nBitsAvailable)
											? nBitsNeeded
											: nBitsAvailable;

							INT nBits = nCurrByte >> (8 - nBitsAvailable);

							nBits &= 0xFF >> (8 - nBitsCopied);
							nCurrCode |= nBits << (nCurrCodesize - nBitsNeeded);
							nBitsAvailable -= nBitsCopied;
							nBitsNeeded -= nBitsCopied;

							if (nBitsNeeded == 0)
							{
								BYTE byExpanded[4096];
								INT  nExplen;

								do
								{
									if (nCurrCode == nEndCode)
									{
										bEndCodeSeen = TRUE;
										break;
									}

									if (nCurrCode == nClearCode)
									{
										nMaxcode = (1 << nCodesize) + 2;
										nCurrCodesize = nCodesize + 1;
										nOldCode = -1;
										break;
									}

									if (nCurrCode < nMaxcode)
									{
										if (nMaxcode < 4096 && nOldCode != -1)
										{
											INT c = nCurrCode;
											while (prefix_table[c] != -1)
												c = prefix_table[c];
											c = code_table[c];
											prefix_table[nMaxcode] = nOldCode;
											code_table[nMaxcode] = c;
											nMaxcode++;
											if (nMaxcode == (1 << nCurrCodesize) && nCurrCodesize < 12)
												nCurrCodesize++;
										}
									}
									else
									{
										INT c;

										if (nCurrCode > nMaxcode || nOldCode == -1) goto quit;

										_ASSERT(nCurrCode == nMaxcode);

										/* Once maxcode == 4096, we can't get here
										 * any more, because we refuse to raise
										 * nCurrCodeSize above 12 -- so we can
										 * never read a bigger code than 4095.
										 */

										c = nOldCode;
										while (prefix_table[c] != -1)
											c = prefix_table[c];
										c = code_table[c];
										prefix_table[nMaxcode] = nOldCode;
										code_table[nMaxcode] = c;
										nMaxcode++;

										if (nMaxcode == (1 << nCurrCodesize) && nCurrCodesize < 12)
											nCurrCodesize++;
									}
									nOldCode = nCurrCode;

									// output nCurrCode!
									nExplen = 0;
									while (nCurrCode != -1)
									{
										_ASSERT(nExplen < ARRAYSIZEOF(byExpanded));
										byExpanded[nExplen++] = (BYTE) code_table[nCurrCode];
										nCurrCode = prefix_table[nCurrCode];
									}
									_ASSERT(nExplen > 0);

									while (--nExplen >= 0)
									{
										// get color map index
										BYTE byColIndex = byExpanded[nExplen];

										LPBYTE pbyRgbr = pbyPixels + (lBytesPerLine * (nTop + v) + 3 * (nLeft + h));

										_ASSERT(pbyRgbr + 2 < pbyPixels + bmi.bmiHeader.biSizeImage);
										_ASSERT(byColIndex < sAct.biClrUsed);

										*pbyRgbr++ = sAct.bmiColors[byColIndex].rgbBlue;
										*pbyRgbr++ = sAct.bmiColors[byColIndex].rgbGreen;
										*pbyRgbr   = sAct.bmiColors[byColIndex].rgbRed;

										if (++h == nWidth)
										{
											h = 0;
											if (bInterlaced)
											{
												switch (v & 7)
												{
												case 0:
													v += 8;
													if (v < nHeight)
														break;
													/* Some GIF en/decoders go
													 * straight from the '0'
													 * pass to the '4' pass
													 * without checking the
													 * height, and blow up on
													 * 2/3/4 pixel high
													 * interlaced images.
													 */
													if (nHeight > 4)
														v = 4;
													else
														if (nHeight > 2)
															v = 2;
														else
															if (nHeight > 1)
																v = 1;
															else
																bEndCodeSeen = TRUE;
													break;
												case 4:
													v += 8;
													if (v >= nHeight)
														v = 2;
													break;
												case 2:
												case 6:
													v += 4;
													if (v >= nHeight)
														v = 1;
													break;
												case 1:
												case 3:
												case 5:
												case 7:
													v += 2;
													if (v >= nHeight)
														bEndCodeSeen = TRUE;
													break;
												}
												if (bEndCodeSeen)
													break; // while (--nExplen >= 0)
											}
											else // non interlaced
											{
												if (++v == nHeight)
												{
													bEndCodeSeen = TRUE;
													break; // while (--nExplen >= 0)
												}
											}
										}
									}
								}
								while (FALSE);

								nCurrCode = 0;
								nBitsNeeded = nCurrCodesize;
							}
						}
					}

					if (ReadGifByte(pBmp,&nBytecount))
						goto quit;
				}
			}
			break;

		case '!': // extension block
			{
				INT i,nFunctionCode,nByteCount,nDummy;

				if (ReadGifByte(pBmp,&nFunctionCode)) goto quit;
				if (ReadGifByte(pBmp,&nByteCount)) goto quit;

				// Graphic Control Label & correct Block Size
				if (nFunctionCode == 0xF9 && nByteCount == 0x04)
				{
					INT nPackedFields,nColorIndex;

					// packed fields
					if (ReadGifByte(pBmp,&nPackedFields)) goto quit;

					// delay time
					if (ReadGifWord(pBmp,&nDummy)) goto quit;

					// transparent color index
					if (ReadGifByte(pBmp,&nColorIndex)) goto quit;

					// transparent color flag set
					if ((nPackedFields & 0x1) != 0)
					{
						if (pdwTransparentColor != NULL)
						{
							*pdwTransparentColor = RGB(sGlb.bmiColors[nColorIndex].rgbRed,
													   sGlb.bmiColors[nColorIndex].rgbGreen,
													   sGlb.bmiColors[nColorIndex].rgbBlue);
						}
					}

					// block terminator (0 byte)
					if (!(!ReadGifByte(pBmp,&nDummy) && nDummy == 0)) goto quit;
				}
				else // other function
				{
					while (nByteCount != 0)
					{
						for (i = 0; i < nByteCount; ++i)
						{
							if (ReadGifByte(pBmp,&nDummy)) goto quit;
						}

						if (ReadGifByte(pBmp,&nByteCount)) goto quit;
					}
				}
			}
			break;

		case ';': // terminator
			bDecoding = FALSE;
			break;

		default: goto quit;
		}
	}

	_ASSERT(bDecoding == FALSE);			// decoding successful

	// normal decoding exit
	if (bPalette && hPalette == NULL)
	{
		hPalette = CreateBIPalette((PBITMAPINFOHEADER) &bmi);
		// save old palette
		hOldPalette = SelectPalette(hWindowDC, hPalette, FALSE);
		RealizePalette(hWindowDC);
	}

quit:
	if (hBitmap != NULL && bDecoding)		// creation failed
	{
		DeleteObject(hBitmap);				// delete bitmap
		hBitmap = NULL;
	}
	return hBitmap;
}

static HBITMAP DecodePng(LPBMPFILE pBmp,BOOL bPalette)
{
	// this implementation use the PNG image file decoder
	// engine of Copyright (c) 2005-2018 Lode Vandevenne

	HBITMAP hBitmap;

	UINT    uError,uWidth,uHeight;
	INT     nWidth,nHeight;
	LONG    lBytesPerLine;

	LPBYTE  pbyImage;						// PNG RGB image data
	LPBYTE  pbySrc;							// source buffer pointer
	LPBYTE  pbyPixels;						// BMP buffer

	BITMAPINFO bmi;

	hBitmap = NULL;
	pbyImage = NULL;

	// decode PNG image
	uError = lodepng_decode_memory(&pbyImage,&uWidth,&uHeight,pBmp->pbyFile,pBmp->dwFileSize,LCT_RGB,8);
	if (uError) goto quit;

	ZeroMemory(&bmi,sizeof(bmi));			// init bitmap info
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = (LONG) uWidth;
	bmi.bmiHeader.biHeight = (LONG) uHeight;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 24;			// create a true color DIB
	bmi.bmiHeader.biCompression = BI_RGB;

	// bitmap dimensions
	lBytesPerLine = WIDTHBYTES(bmi.bmiHeader.biWidth * bmi.bmiHeader.biBitCount);
	bmi.bmiHeader.biSizeImage = lBytesPerLine * bmi.bmiHeader.biHeight;

	// allocate buffer for pixels
	VERIFY(hBitmap = CreateDIBSection(hWindowDC,
									  &bmi,
									  DIB_RGB_COLORS,
									  (VOID **)&pbyPixels,
									  NULL,
									  0));
	if (hBitmap == NULL) goto quit;

	pbySrc = pbyImage;						// init source loop pointer

	// fill bottom up DIB pixel buffer with color information
	for (nHeight = bmi.bmiHeader.biHeight - 1; nHeight >= 0; --nHeight)
	{
		LPBYTE pbyLine = pbyPixels + nHeight * lBytesPerLine;

		for (nWidth = 0; nWidth < bmi.bmiHeader.biWidth; ++nWidth)
		{
			*pbyLine++ = pbySrc[2];			// blue
			*pbyLine++ = pbySrc[1];			// green
			*pbyLine++ = pbySrc[0];			// red
			pbySrc += 3;
		}

		_ASSERT((DWORD) (pbyLine - pbyPixels) <= bmi.bmiHeader.biSizeImage);
	}

	if (bPalette && hPalette == NULL)
	{
		hPalette = CreateBIPalette((PBITMAPINFOHEADER) &bmi);
		// save old palette
		hOldPalette = SelectPalette(hWindowDC, hPalette, FALSE);
		RealizePalette(hWindowDC);
	}

quit:
	if (pbyImage != NULL)					// buffer for PNG image allocated
	{
		free(pbyImage);						// free PNG image data
	}

	if (hBitmap != NULL && uError != 0)		// creation failed
	{
		DeleteObject(hBitmap);				// delete bitmap
		hBitmap = NULL;
	}
	return hBitmap;
}

HBITMAP LoadBitmapFile(LPCTSTR szFilename,BOOL bPalette)
{
	HANDLE  hFile;
	HANDLE  hMap;
	BMPFILE Bmp;
	HBITMAP hBitmap;

	SetCurrentDirectory(szEmuDirectory);
	hFile = CreateFile(szFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	SetCurrentDirectory(szCurrentDirectory);
	if (hFile == INVALID_HANDLE_VALUE) return NULL;
	Bmp.dwFileSize = GetFileSize(hFile, NULL);
	hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (hMap == NULL)
	{
		CloseHandle(hFile);
		return NULL;
	}
	Bmp.pbyFile = (LPBYTE) MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
	if (Bmp.pbyFile == NULL)
	{
		CloseHandle(hMap);
		CloseHandle(hFile);
		return NULL;
	}

	do
	{
		// check for bitmap file header "BM"
		if (Bmp.dwFileSize >= 2 && *(WORD *) Bmp.pbyFile == 0x4D42)
		{
			hBitmap = DecodeBmp(&Bmp,bPalette);
			break;
		}

		// check for GIF file header
		if (   Bmp.dwFileSize >= 6
			&& (memcmp(Bmp.pbyFile,"GIF87a",6) == 0 || memcmp(Bmp.pbyFile,"GIF89a",6) == 0))
		{
			hBitmap = DecodeGif(&Bmp,&dwTColor,bPalette);
			break;
		}

		// check for PNG file header
		if (Bmp.dwFileSize >= 8 && memcmp(Bmp.pbyFile,"\x89PNG\r\n\x1a\n",8) == 0)
		{
			hBitmap = DecodePng(&Bmp,bPalette);
			break;
		}

		// unknown file type
		hBitmap = NULL;
	}
	while (FALSE);

	UnmapViewOfFile(Bmp.pbyFile);
	CloseHandle(hMap);
	CloseHandle(hFile);
	return hBitmap;
}

static BOOL AbsColorCmp(DWORD dwColor1,DWORD dwColor2,DWORD dwTol)
{
	DWORD dwDiff;

	dwDiff =  (DWORD) abs((INT) (dwColor1 & 0xFF) - (INT) (dwColor2 & 0xFF));
	dwColor1 >>= 8;
	dwColor2 >>= 8;
	dwDiff += (DWORD) abs((INT) (dwColor1 & 0xFF) - (INT) (dwColor2 & 0xFF));
	dwColor1 >>= 8;
	dwColor2 >>= 8;
	dwDiff += (DWORD) abs((INT) (dwColor1 & 0xFF) - (INT) (dwColor2 & 0xFF));

	return dwDiff > dwTol;					// FALSE = colors match
}

static BOOL LabColorCmp(DWORD dwColor1,DWORD dwColor2,DWORD dwTol)
{
	DWORD dwDiff;
	INT   nDiffCol;

	nDiffCol = (INT) (dwColor1 & 0xFF) - (INT) (dwColor2 & 0xFF);
	dwDiff = (DWORD) (nDiffCol * nDiffCol);
	dwColor1 >>= 8;
	dwColor2 >>= 8;
	nDiffCol = (INT) (dwColor1 & 0xFF) - (INT) (dwColor2 & 0xFF);
	dwDiff += (DWORD) (nDiffCol * nDiffCol);
	dwColor1 >>= 8;
	dwColor2 >>= 8;
	nDiffCol = (INT) (dwColor1 & 0xFF) - (INT) (dwColor2 & 0xFF);
	dwDiff += (DWORD) (nDiffCol * nDiffCol);
	dwTol *= dwTol;
	
	return dwDiff > dwTol;					// FALSE = colors match
}

static DWORD EncodeColorBits(DWORD dwColorVal,DWORD dwMask)
{
	#define MAXBIT 32
	UINT uLshift = MAXBIT;
	UINT uRshift = 8;
	DWORD dwBitMask = dwMask;

	dwColorVal &= 0xFF;						// the color component using the lowest 8 bit

	// position of highest bit
	while ((dwBitMask & (1<<(MAXBIT-1))) == 0 && uLshift > 0)
	{
		dwBitMask <<= 1;					// next bit
		--uLshift;							// next position
	}

	if (uLshift > 24)						// avoid overflow on 32bit mask
	{
		uLshift -= uRshift;					// normalize left shift
		uRshift = 0;
	}

	return ((dwColorVal << uLshift) >> uRshift) & dwMask;
	#undef MAXBIT
}

HRGN CreateRgnFromBitmap(HBITMAP hBmp,COLORREF color,DWORD dwTol)
{
	#define ADD_RECTS_COUNT  256

	BOOL (*fnColorCmp)(DWORD dwColor1,DWORD dwColor2,DWORD dwTol);

	DWORD dwRed,dwGreen,dwBlue;
	HRGN hRgn;
	LPRGNDATA pRgnData;
	LPBITMAPINFO bi;
	LPBYTE pbyBits;
	LPBYTE pbyColor;
	DWORD dwAlignedWidthBytes;
	DWORD dwBpp;
	DWORD dwRectsCount;
	LONG x,y,xleft;
	BOOL bFoundLeft;
	BOOL bIsMask;

	if (dwTol >= 1000)						// use CIE L*a*b compare
	{
		fnColorCmp = LabColorCmp;
		dwTol -= 1000;						// remove L*a*b compare selector
	}
	else									// use Abs summation compare
	{
		fnColorCmp = AbsColorCmp;
	}

	// allocate memory for extended image information incl. RGBQUAD color table
	bi = (LPBITMAPINFO) calloc(1,sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
	bi->bmiHeader.biSize = sizeof(bi->bmiHeader);
	_ASSERT(bi->bmiHeader.biBitCount == 0); // for query without color table

	// get information about image
	GetDIBits(hWindowDC,hBmp,0,0,NULL,bi,DIB_RGB_COLORS);

	// DWORD aligned bitmap width in BYTES
	dwAlignedWidthBytes = WIDTHBYTES(  bi->bmiHeader.biWidth
									 * bi->bmiHeader.biPlanes
									 * bi->bmiHeader.biBitCount);

	// biSizeImage is empty
	if (bi->bmiHeader.biSizeImage == 0 && bi->bmiHeader.biCompression == BI_RGB)
	{
		bi->bmiHeader.biSizeImage = dwAlignedWidthBytes * bi->bmiHeader.biHeight;
	}

	// allocate memory for image data (colors)
	pbyBits = (LPBYTE) malloc(bi->bmiHeader.biSizeImage);

	// fill bits buffer
	GetDIBits(hWindowDC,hBmp,0,bi->bmiHeader.biHeight,pbyBits,bi,DIB_RGB_COLORS);

	// convert color if current DC is 16-bit/32-bit bitfield coded
	if (bi->bmiHeader.biCompression == BI_BITFIELDS)
	{
		dwRed   = *(LPDWORD) &bi->bmiColors[0];
		dwGreen = *(LPDWORD) &bi->bmiColors[1];
		dwBlue  = *(LPDWORD) &bi->bmiColors[2];
	}
	else // RGB coded
	{
		// convert color if current DC is 16-bit RGB coded
		if (bi->bmiHeader.biBitCount == 16)
		{
			// for 15 bit (5:5:5)
			dwRed   = 0x00007C00;
			dwGreen = 0x000003E0;
			dwBlue  = 0x0000001F;
		}
		else
		{
			// convert COLORREF to RGBQUAD color
			dwRed   = 0x00FF0000;
			dwGreen = 0x0000FF00;
			dwBlue  = 0x000000FF;
		}
	}
	color = EncodeColorBits((color >> 16), dwBlue)
		  | EncodeColorBits((color >>  8), dwGreen)
		  | EncodeColorBits((color >>  0), dwRed);

	dwBpp = bi->bmiHeader.biBitCount >> 3;	// bytes per pixel

	// DIB is bottom up image so we begin with the last scanline
	pbyColor = pbyBits + (bi->bmiHeader.biHeight - 1) * dwAlignedWidthBytes;

	dwRectsCount = bi->bmiHeader.biHeight;	// number of rects in allocated buffer

	bFoundLeft = FALSE;						// set when mask has been found in current scan line

	// allocate memory for region data
	pRgnData = (PRGNDATA) malloc(sizeof(RGNDATAHEADER) + dwRectsCount * sizeof(RECT));

	// fill it by default
	ZeroMemory(&pRgnData->rdh,sizeof(pRgnData->rdh));
	pRgnData->rdh.dwSize = sizeof(pRgnData->rdh);
	pRgnData->rdh.iType	 = RDH_RECTANGLES;
	SetRect(&pRgnData->rdh.rcBound,MAXLONG,MAXLONG,0,0);

	for (y = 0; y < bi->bmiHeader.biHeight; ++y)
	{
		LPBYTE pbyLineStart = pbyColor;

		for (x = 0; x < bi->bmiHeader.biWidth; ++x)
		{
			// get color
			switch (bi->bmiHeader.biBitCount)
			{
			case 8:
				bIsMask = fnColorCmp(*(LPDWORD)(&bi->bmiColors)[*pbyColor],color,dwTol);
				break;
			case 16:
				// it makes no sense to allow a tolerance here
				bIsMask = (*(LPWORD)pbyColor != (WORD) color);
				break;
			case 24:
				bIsMask = fnColorCmp((*(LPDWORD)pbyColor & 0x00ffffff),color,dwTol);
				break;
			case 32:
				bIsMask = fnColorCmp(*(LPDWORD)pbyColor,color,dwTol);
			}
			pbyColor += dwBpp;				// shift pointer to next color

			if (!bFoundLeft && bIsMask)		// non transparent color found
			{
				xleft = x;
				bFoundLeft = TRUE;
			}

			if (bFoundLeft)					// found non transparent color in scanline
			{
				// transparent color or last column
				if (!bIsMask || x + 1 == bi->bmiHeader.biWidth)
				{
					// non transparent color and last column
					if (bIsMask && x + 1 == bi->bmiHeader.biWidth)
						++x;

					// save current RECT
					((LPRECT) pRgnData->Buffer)[pRgnData->rdh.nCount].left = xleft;
					((LPRECT) pRgnData->Buffer)[pRgnData->rdh.nCount].top  = y;
					((LPRECT) pRgnData->Buffer)[pRgnData->rdh.nCount].right = x;
					((LPRECT) pRgnData->Buffer)[pRgnData->rdh.nCount].bottom = y + 1;
					pRgnData->rdh.nCount++;

					if (xleft < pRgnData->rdh.rcBound.left)
						pRgnData->rdh.rcBound.left = xleft;

					if (y < pRgnData->rdh.rcBound.top)
						pRgnData->rdh.rcBound.top = y;

					if (x > pRgnData->rdh.rcBound.right)
						pRgnData->rdh.rcBound.right = x;

					if (y + 1 > pRgnData->rdh.rcBound.bottom)
						pRgnData->rdh.rcBound.bottom = y + 1;

					// if buffer full reallocate it with more room
					if (pRgnData->rdh.nCount >= dwRectsCount)
					{
						dwRectsCount += ADD_RECTS_COUNT;

						pRgnData = (LPRGNDATA) realloc(pRgnData,sizeof(RGNDATAHEADER) + dwRectsCount * sizeof(RECT));
					}

					bFoundLeft = FALSE;
				}
			}
		}

		// previous scanline
		pbyColor = pbyLineStart - dwAlignedWidthBytes;
	}
	// release image data
	free(pbyBits);
	free(bi);

	// create region
	hRgn = ExtCreateRegion(NULL,sizeof(RGNDATAHEADER) + pRgnData->rdh.nCount * sizeof(RECT),pRgnData);

	free(pRgnData);
	return hRgn;
	#undef ADD_RECTS_COUNT
}
