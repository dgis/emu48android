/*
 *   disrpl.c
 *
 *   This file is part of Emu48
 *
 *   Copyright (C) 2008 Christoph Gießelink
 *
 */
#include "pch.h"
#include "Emu48.h"
#include "disrpl.h"

DWORD dwRplPlatform = RPL_P3;				// current RPL platform

//################
//#
//#    String Writing Helper Functions
//#
//################

#define ALLOCSIZE 256						// string allocation size

typedef struct
{
	DWORD  dwSize;							// size of buffer
	LPTSTR szBuffer;						// buffer
	DWORD  dwPos;							// position inside buffer
} String;

static VOID PutSn(String *str, LPCTSTR szVal, DWORD dwLen)
{
	if (str != NULL)						// with output
	{
		if (str->dwSize == 0)				// no buffer allocated
		{
			str->dwSize = ALLOCSIZE;		// buffer size
			VERIFY(str->szBuffer = (LPTSTR) malloc(str->dwSize * sizeof(TCHAR)));
			str->dwPos = 0;
		}

		_ASSERT(str->szBuffer != NULL);

		if (dwLen > 0)						// actual string length
		{
			// string buffer to small
			if (str->dwPos + dwLen > str->dwSize)
			{
				DWORD dwMinSize;
				dwMinSize = dwLen + str->dwSize - str->dwPos;
				dwMinSize = (dwMinSize + ALLOCSIZE - 1) / ALLOCSIZE;
				dwMinSize *= ALLOCSIZE;

				str->dwSize += dwMinSize;	// new buffer size
				VERIFY(str->szBuffer = (LPTSTR) realloc(str->szBuffer,str->dwSize * sizeof(TCHAR)));
			}

			CopyMemory(&str->szBuffer[str->dwPos],szVal,dwLen * sizeof(TCHAR));
			str->dwPos += dwLen;
		}
	}
	return;
}

static VOID PutC(String *str, TCHAR cVal)
{
	PutSn(str,&cVal,1);
	return;
}

static VOID PutS(String *str, LPCTSTR szVal)
{
	PutSn(str,szVal,lstrlen(szVal));
	return;
}

static VOID __cdecl PutFS(String *str, LPCTSTR lpFormat, ...)
{
	if (str != NULL)						// with output
	{
		TCHAR cOutput[1024];
		va_list arglist;

		va_start(arglist,lpFormat);
		wvsprintf(cOutput,lpFormat,arglist);
		PutS(str,cOutput);
		va_end(arglist);
	}
	return;
}

static VOID PutCondSpc(String *str)
{
	if (str != NULL)						// with output
	{
		// write space when not at beginning and prior character isn't a whitespace
		if (   str->dwPos > 0
			&& _tcschr(_T(" \t\n\r"),str->szBuffer[str->dwPos-1]) == NULL)
			PutC(str,_T(' '));
	}
	return;
}

//################
//#
//#    RPL Object Decoding
//#
//################

static LPCTSTR cHex = _T("0123456789ABCDEF");

// function prototypes
static BOOL (*Getfp(LPCTSTR lpszObject))(DWORD *pdwAddr,String *str,UINT *pnLevel);
static BOOL FetchObj(DWORD *pdwAddr,String *str,UINT *pnLevel);

static DWORD Readx(DWORD *pdwAddr,DWORD n)
{
	DWORD i, t;

	for (i = 0, t = 0; i < n; ++i)
		t |= RplReadNibble(pdwAddr) << (i * 4);

	return t;
}

static BOOL BCDx(BYTE CONST *pbyNum,INT nMantLen,INT nExpLen,String *str)
{
	BYTE  byNib;
	LONG  v,lExp;
	BOOL  bPflag,bExpflag;
	DWORD dwStart;
	INT   i;

	if (str == NULL) return FALSE;			// no output

	dwStart = str->dwPos;					// starting pos inside string

	lExp = 0;
	for (v = 1; nExpLen--; v *= 10)			// fetch exponent
	{
		lExp += (LONG) *pbyNum++ * v;		// calc. exponent
	}

	if (lExp > v / 2) lExp -= v;			// negative exponent

	lExp -= nMantLen - 1;					// set decimal point to end of mantissa

	bPflag = FALSE;							// show no decimal point
	bExpflag = FALSE;						// show no exponent

	// scan mantissa
	for (v = (LONG) nMantLen - 1; v >= 0 || bPflag; v--)
	{
		if (v >= 0L)						// still mantissa digits left
			byNib = *pbyNum++;
		else
			byNib = 0;						// zero for negative exponent

		if (dwStart == str->dwPos)			// still delete zeros at end
		{
			if (byNib == 0 && lExp && v > 0) // delete zeros
			{
				lExp++;						// adjust exponent
				continue;
			}

			// TRUE at x.E
			bExpflag = v + lExp >= nMantLen || lExp < -nMantLen;
			bPflag = !bExpflag && v < -lExp; // decimal point flag at neg. exponent
		}

		// set decimal point
		if ((bExpflag && v == 0) || (!lExp && (dwStart != str->dwPos)))
		{
			PutC(str,_T('.'));				// write decimal point
			if (v < 0)						// no mantissa digits any more
			{
//				PutC(str,_T('0'));			// write heading zero
			}
			bPflag = FALSE;					// finished with negative exponents
		}

		if (v >= 0 || bPflag)
		{
			TCHAR cVal = (TCHAR) byNib + _T('0');
			PutC(str,cVal);					// write character
		}

		lExp++;								// next position
	}

	if (*pbyNum == 9)						// negative number
	{
		PutC(str,_T('-'));					// write sign
	}

	i = (str->dwPos - dwStart) / 2;
	for (v = 0; v < i; v++)					// reverse string
	{
		// swap chars
		TCHAR cNib = str->szBuffer[dwStart+v];
		str->szBuffer[dwStart+v] = str->szBuffer[str->dwPos-v-1];
		str->szBuffer[str->dwPos-v-1] = cNib;
	}

	// write number with exponent
	if (bExpflag)
	{
		PutFS(str,_T("E%d"),lExp-1);		// write exponent
	}
	return FALSE;
}

static BOOL BINx(DWORD *pdwAddr,INT nBinLen,String *str)
{
	LPBYTE pbyNumber;
	INT    i;

	VERIFY(pbyNumber = (LPBYTE) malloc(nBinLen));

	for (i = 0; i < nBinLen; ++i)			// read data
		pbyNumber[i] = RplReadNibble(pdwAddr);

	// strip leading zeros
	for (i = nBinLen - 1; pbyNumber[i] == 0 && i > 0; --i) { }

	for (; i >= 0; --i)						// write rest of bin
	{
		PutC(str,cHex[pbyNumber[i]]);
	}

	free(pbyNumber);
	return FALSE;
}

static BOOL ASCIC(DWORD *pdwAddr,String *str)
{
	DWORD dwLength;

	dwLength = Readx(pdwAddr,2);			// fetch string length in bytes

	for (; dwLength > 0; --dwLength)
	{
		PutC(str,(TCHAR) Readx(pdwAddr,2));	// write data byte
	}
	return FALSE;
}

static BOOL ASCIX(DWORD *pdwAddr,String *str)
{
	BOOL bErr = ASCIC(pdwAddr,str);			// decode a ASCIC
	Readx(pdwAddr,2);						// skip length information at end
	return bErr;
}

// simple HEX output of object
static BOOL DoHexStream(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	DWORD dwObjStart,dwLength;

	dwObjStart = *pdwAddr;					// remember start position
	dwLength = Readx(pdwAddr,5);			// fetch code length in nibbles
	if (dwLength < 5)						// illegal object length
	{
		*pdwAddr = dwObjStart;				// continue decoding behind prolog
		return TRUE;
	}

	dwLength -= 5;							// no. of DOCODE nibbles
	PutFS(str,_T("%X"),dwLength);			// write length information
	if (dwLength > 0)						// have data
	{
		PutC(str,_T(' '));
	}

	for (;dwLength > 0; --dwLength)
	{
		PutC(str,cHex[RplReadNibble(pdwAddr)]); // write digit
	}
	return FALSE;
	UNREFERENCED_PARAMETER(pnLevel);
}

// SEMI stream helper function
static BOOL DoSemiStream(DWORD *pdwAddr,String *str,UINT *pnLevel,LPCTSTR lpszSemi)
{
	DWORD dwObjStart;
	BOOL  bErr;

	UINT nActLevel = *pnLevel;				// save actual nesting level

	(*pnLevel)++;							// next level

	dwObjStart = *pdwAddr;					// remember start position

	do
	{
		// eval object
		bErr = FetchObj(pdwAddr,str,pnLevel);
	}
	while (!bErr && *pnLevel != nActLevel);

	if (bErr)								// decoding error
	{
		*pdwAddr = dwObjStart;				// continue decoding behind prolog
	}
	else
	{
		// no blank because of prior SEMI
		PutS(str,lpszSemi);					// write semi replacement character
	}
	return bErr;
}

// DOINT stream helper function
static BOOL DoIntStream(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	LPBYTE pbyData;
	DWORD  dwObjStart,dwLength,i;

	dwObjStart = *pdwAddr;					// remember start position
	dwLength = Readx(pdwAddr,5);			// fetch zint length in nibbles
	if (dwLength < 5)						// illegal object length
	{
		*pdwAddr = dwObjStart;				// continue decoding behind prolog
		return TRUE;
	}

	dwLength -= 5;							// object length

	VERIFY(pbyData = (LPBYTE) malloc(dwLength));

	for (i = 0; i < dwLength; ++i)			// read data
		pbyData[i] = RplReadNibble(pdwAddr);

	if (dwLength <= 1)						// special implementation for zero
	{
		_ASSERT(dwLength == 0 || (dwLength == 1 && pbyData[0] == 0));
		PutC(str,_T('0'));
	}
	else
	{
		if (pbyData[--dwLength] == 9)		// negative number
			PutC(str,_T('-'));

		while (dwLength > 0)				// write rest of zint
		{
			// use only decimal part for translation
			PutC(str,cHex[pbyData[--dwLength]]);
		}
	}

	free(pbyData);
	return FALSE;
	UNREFERENCED_PARAMETER(pnLevel);
}


//################
//#
//#    RPL Object Primitives
//#
//################

static BOOL DoBint(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	PutS(str,_T("# "));
	return BINx(pdwAddr,5,str);				// BIN5
	UNREFERENCED_PARAMETER(pnLevel);
}

static BOOL DoReal(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	BYTE  byNumber[16];
	DWORD i;

	PutS(str,_T("% "));

	// get real object content
	for (i = 0; i < ARRAYSIZEOF(byNumber); ++i)
		byNumber[i] = RplReadNibble(pdwAddr);

	return BCDx(byNumber,12,3,str);
	UNREFERENCED_PARAMETER(pnLevel);
}

static BOOL DoERel(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	BYTE byNumber[21];
	DWORD i;

	PutS(str,_T("%% "));

	// get extended real object content
	for (i = 0; i < ARRAYSIZEOF(byNumber); ++i)
		byNumber[i] = RplReadNibble(pdwAddr);

	return BCDx(byNumber,15,5,str);
	UNREFERENCED_PARAMETER(pnLevel);
}

static BOOL DoCmp(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	BYTE byNumber[16];
	DWORD i;

	PutS(str,_T("C% "));

	// get real part of complex object content
	for (i = 0; i < ARRAYSIZEOF(byNumber); ++i)
		byNumber[i] = RplReadNibble(pdwAddr);

	BCDx(byNumber,12,3,str);

	PutC(str,_T(' '));

	// get imaginary part of complex object content
	for (i = 0; i < ARRAYSIZEOF(byNumber); ++i)
		byNumber[i] = RplReadNibble(pdwAddr);

	return BCDx(byNumber,12,3,str);
	UNREFERENCED_PARAMETER(pnLevel);
}

static BOOL DoECmp(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	BYTE byNumber[21];
	DWORD i;

	PutS(str,_T("C%% "));

	// get real part of extended complex object content
	for (i = 0; i < ARRAYSIZEOF(byNumber); ++i)
		byNumber[i] = RplReadNibble(pdwAddr);

	BCDx(byNumber,15,5,str);

	PutC(str,_T(' '));

	// get imaginary part of extended complex object content
	for (i = 0; i < ARRAYSIZEOF(byNumber); ++i)
		byNumber[i] = RplReadNibble(pdwAddr);

	return BCDx(byNumber,15,5,str);
	UNREFERENCED_PARAMETER(pnLevel);
}

static BOOL DoChar(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	DWORD dwChar = Readx(pdwAddr,2);		// character code

	PutS(str,_T("CHR "));

	if (dwChar == ' ')						// special handling for space
	{
		PutS(str,_T("\" \""));				// write space
	}
	else
	{
		if (dwChar >= 0x80)					// non ASCII character
		{
			// print as escape sequence
			PutC(str,_T('\\'));				// escape sequence prefix
			PutC(str,cHex[dwChar >> 4]);	// print higher nibble
			dwChar = cHex[dwChar & 0xF];	// lower nibble
		}
		else
		{
			if (dwChar < ' ')				// control character
			{
				PutC(str,_T('^'));			// prefix
				dwChar += '@';				// convert to control char
			}
		}

		PutC(str,(TCHAR) dwChar);			// print character
	}
	return FALSE;
	UNREFERENCED_PARAMETER(pnLevel);
}

static BOOL DoArry(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	BOOL (*pObjHandler)(DWORD *,String *,UINT *pnLevel);
	LPCTSTR lpszName;
	DWORD i,dwObjStart,dwLength,dwObject,dwDims,dwDimI;

	dwObjStart = *pdwAddr;					// remember start position
	dwLength = Readx(pdwAddr,5);			// fetch object length in nibbles

	// look for object handler
	dwObject = Readx(pdwAddr,5);			// fetch object
	lpszName = RplGetName(dwObject);		// name of object
	if (lpszName == NULL)					// unknown type
	{
		*pdwAddr = dwObjStart;				// continue decoding behind prolog
		return TRUE;
	}
	pObjHandler = Getfp(lpszName);			// search for object handler
	if (pObjHandler == NULL)				// unknown object handler
	{
		*pdwAddr = dwObjStart;				// continue decoding behind prolog
		return TRUE;
	}

	dwDims = Readx(pdwAddr,5);				// number of array dimensions
	dwDimI = Readx(pdwAddr,5);				// 1st dimension

	PutS(str,_T("ARRY "));

	if (dwDims > 1)							// more than 1 dimension
	{
		DWORD dwDimJ;

		PutFS(str,_T("%u "),dwDimI);		// 1st dimension

		// check other dimensions
		for (--dwDims; dwDims > 0; --dwDims)
		{
			dwDimJ = Readx(pdwAddr,5);		// other dimensions
			PutFS(str,_T("%u "),dwDimJ);	// write other dimension
			dwDimI *= dwDimJ;				// no. of elements
		}
	}

	PutC(str,_T('['));

	for (i = 0; i < dwDimI; ++i)
	{
		PutC(str,_T(' '));
		pObjHandler(pdwAddr,str,pnLevel);	// decode object
	}

	PutS(str,_T(" ]"));

	// this assert also fail on illegal objects
	_ASSERT(dwObjStart + dwLength == *pdwAddr);
	return FALSE;
}

static BOOL DoLnkArry(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	BOOL (*pObjHandler)(DWORD *,String *,UINT *pnLevel);
	LPCTSTR lpszName;
	DWORD i,dwAddr,dwLength,dwObject,dwDims,dwDimI,dwObjAddr;

	dwAddr = *pdwAddr;						// address of array start
	dwLength = Readx(&dwAddr,5);			// fetch object length in nibbles

	// look for object handler
	dwObject = Readx(&dwAddr,5);			// fetch object
	lpszName = RplGetName(dwObject);		// name of object
	if (lpszName == NULL)					// unknown type
	{
		return TRUE;						// continue decoding behind prolog
	}
	pObjHandler = Getfp(lpszName);			// search for object handler
	if (pObjHandler == NULL)				// unknown object handler
	{
		return TRUE;						// continue decoding behind prolog
	}

	dwDims = Readx(&dwAddr,5);				// number of array dimensions
	dwDimI = Readx(&dwAddr,5);				// 1st dimension

	PutS(str,_T("LNKARRY "));

	if (dwDims > 1)							// more than 1 dimension
	{
		DWORD dwDimJ;

		PutFS(str,_T("%u "),dwDimI);		// 1st dimension

		// check other dimensions
		for (--dwDims; dwDims > 0; --dwDims)
		{
			dwDimJ = Readx(&dwAddr,5);		// other dimensions
			PutFS(str,_T("%u "),dwDimJ);	// write other dimension
			dwDimI *= dwDimJ;				// no. of elements
		}
	}

	PutC(str,_T('['));

	for (i = 0; i < dwDimI; ++i)
	{
		PutC(str,_T(' '));

		dwObjAddr = dwAddr;					// actual address
		dwObject = Readx(&dwAddr,5);		// relative link to object
		if (dwObject == 0)					// empty link
		{
			PutC(str,_T('_'));
		}
		else
		{
			Readx(&dwObjAddr,dwObject);		// absolute address
			pObjHandler(&dwObjAddr,str,pnLevel); // decode object
		}
	}

	PutS(str,_T(" ]"));

	Readx(pdwAddr,dwLength);				// goto end of link array
	return FALSE;
}

static BOOL DoCStr(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	DWORD i,dwObjStart,dwLength,dwC;

	dwObjStart = *pdwAddr;					// remember start position
	dwLength = Readx(pdwAddr,5);			// fetch object length in nibbles
	if (dwLength < 5)						// illegal object length
	{
		*pdwAddr = dwObjStart;				// continue decoding behind prolog
		return TRUE;
	}
	dwLength = (dwLength - 5) / 2;			// string length

	PutS(str,_T("$ \""));

	for (i = 0; i < dwLength; ++i)
	{
		dwC = Readx(pdwAddr,2);				// character code

		do
		{
			if (dwC == '\t')				// tab
			{
				PutS(str,_T("\\t"));
				break;
			}
			if (dwC == '\n')				// newline
			{
				PutS(str,_T("\\n"));
				break;
			}
			if (dwC == '\r')				// return
			{
				PutS(str,_T("\\r"));
				break;
			}
			if (dwC < ' ' || dwC >= 0x80)	// non ASCII character
			{
				PutC(str,_T('\\'));			// escape sequence prefix
				PutC(str,cHex[dwC >> 4]);	// print higher nibble
				PutC(str,cHex[dwC & 0xF]);	// print lower nibble
				break;
			}

			if (dwC == '"' || dwC == '\\')	// double " or \ symbol
				PutC(str,(TCHAR) dwC);

			PutC(str,(TCHAR) dwC);			// print character
		}
		while (FALSE);
	}

	PutC(str,_T('"'));
	return FALSE;
	UNREFERENCED_PARAMETER(pnLevel);
}

static BOOL DoHxs(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	DWORD i,dwObjStart,dwLength;
	BOOL  bRemove;
	BYTE  byVal;

	dwObjStart = *pdwAddr;					// remember start position
	dwLength = Readx(pdwAddr,5);			// fetch object length in nibbles
	if (dwLength < 5)						// illegal object length
	{
		*pdwAddr = dwObjStart;				// continue decoding behind prolog
		return TRUE;
	}

	PutS(str,_T("HXS "));

	dwLength -= 5;							// no. of HXS
	PutFS(str,_T("%X"),dwLength);			// write length information
	if (dwLength > 0)						// have data
	{
		PutC(str,_T(' '));
	}

	bRemove = TRUE;							// remove leading zeros

	for (i = 0; i < dwLength; ++i)
	{
		byVal = RplReadNibble(pdwAddr);

		// remove leading zeros
		if (byVal == 0 && bRemove && i + 1 < dwLength)
			continue;

		PutC(str,cHex[byVal]);				// write digit
		bRemove = FALSE;					// got non zero digit
	}
	return FALSE;
	UNREFERENCED_PARAMETER(pnLevel);
}

static BOOL DoList(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	PutC(str,_T('{'));
	return DoSemiStream(pdwAddr,str,pnLevel,_T("}"));
}

static BOOL DoTag(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	PutS(str,_T("TAG "));
	ASCIC(pdwAddr,str);						// name
	PutC(str,_T(' '));
	return FetchObj(pdwAddr,str,pnLevel);	// object
}

static BOOL DoCol(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	BOOL bErr = FALSE;

	PutS(str,_T("::"));

	if (*pnLevel > 0)						// nested objects
	{
		bErr = DoSemiStream(pdwAddr,str,pnLevel,_T(";"));
	}
	return bErr;
}

static BOOL DoCode(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	PutS(str,_T("CODE "));
	return DoHexStream(pdwAddr,str,pnLevel);
}

static BOOL DoIdnt(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	PutS(str,_T("ID "));
	return ASCIC(pdwAddr,str);
	UNREFERENCED_PARAMETER(pnLevel);
}

static BOOL DoLam(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	PutS(str,_T("LAM "));
	return ASCIC(pdwAddr,str);
	UNREFERENCED_PARAMETER(pnLevel);
}

static BOOL DoRomp(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	PutS(str,_T("ROMPTR "));
	BINx(pdwAddr,3,str);					// BIN3
	PutC(str,_T(' '));
	BINx(pdwAddr,3,str);					// BIN3
	return FALSE;
	UNREFERENCED_PARAMETER(pnLevel);
}

static BOOL Semi(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	if (*pnLevel > 0)						// nested objects
	{
		(*pnLevel)--;						// signaling end, caller handles the semi
	}
	else
	{
		PutC(str,_T(';'));					// write standard semi
	}
	return FALSE;
	UNREFERENCED_PARAMETER(pdwAddr);
}

static BOOL DoRrp(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	DWORD dwOffset,dwObjStart,dwBufferPos,dwAddr;
	BOOL bErr = FALSE;

	dwObjStart = *pdwAddr;					// remember start position of RRP
	dwBufferPos = str->dwPos;				// insert address of text

	PutS(str,_T("DIR"));
	Readx(pdwAddr,3);						// skip attachments field

	dwAddr = *pdwAddr;						// actual address
	dwOffset = Readx(pdwAddr,5);			// offset-1

	if (dwOffset != 0)						// directory not empty
	{
		DWORD dwOffsetAddr;

		Readx(&dwAddr,dwOffset);			// name-1
		if (dwAddr < *pdwAddr)				// address wrap around
		{
			*pdwAddr = dwObjStart;			// continue decoding behind prolog
			str->dwPos = dwBufferPos;		// throw out all DIR output
			return TRUE;
		}

		*pdwAddr = 0;						// unknown end

		do
		{
			PutC(str,_T('\n'));
			PutS(str,_T("VARNAME "));

			// operation is safe because checked for address wrapping before
			dwAddr -= 5;					// address of next offset
			dwOffsetAddr = dwAddr;

			dwOffset = Readx(&dwAddr,5);	// read next offset
			bErr = ASCIX(&dwAddr,str);		// read name

			PutC(str,_T('\n'));
			(*pnLevel)++;					// treat object as whole
			// eval object
			bErr = bErr || FetchObj(&dwAddr,str,pnLevel);
			(*pnLevel)--;

			if (*pdwAddr == 0)				// first name
			{
				*pdwAddr = dwAddr;			// end of dir
			}

			dwAddr = dwOffsetAddr;			// address of next offset

			// illegal offset
			if (dwOffset >= dwAddr - dwObjStart)
			{
				*pdwAddr = dwObjStart;		// continue decoding behind illegal RRP prolog
				str->dwPos = dwBufferPos;	// throw out all DIR output
				return TRUE;
			}

			// operation is safe because checked for address wrapping before
			dwAddr -= dwOffset;				// address of next name
		}
		while (!bErr && dwOffset != 0);
	}

	PutS(str,_T("\nENDDIR"));
	return bErr;
}

// type specific handler
static BOOL DoExt(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	DWORD dwObjStart,dwBufferPos;
	BOOL bErr;

	dwObjStart = *pdwAddr;					// remember start position
	dwBufferPos = str->dwPos;				// insert address of text

	if (dwRplPlatform <= RPL_P2)			// DOEXT
	{
		PutS(str,_T("EXT0 "));
		bErr = DoHexStream(pdwAddr,str,pnLevel);
	}
	else									// Unit
	{
		PutS(str,_T("UNIT "));
		bErr = DoSemiStream(pdwAddr,str,pnLevel,_T(";"));
	}

	if (bErr)								// decoding error
	{
		*pdwAddr = dwObjStart;				// continue decoding behind prolog
		str->dwPos = dwBufferPos;			// throw out all output
	}
	return bErr;
}

static BOOL DoSymb(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	DWORD dwObjStart,dwBufferPos;
	BOOL bErr;

	dwObjStart = *pdwAddr;					// remember start position
	dwBufferPos = str->dwPos;				// insert address of text

	PutS(str,_T("SYMBOL "));
	if ((bErr = DoSemiStream(pdwAddr,str,pnLevel,_T(";"))))
	{
		*pdwAddr = dwObjStart;				// continue decoding behind prolog
		str->dwPos = dwBufferPos;			// throw out all output
	}
	return bErr;
}

static BOOL DoGrob(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	DWORD dwObjStart,dwBufferPos;
	BOOL bErr;

	dwObjStart = *pdwAddr;					// remember start position
	dwBufferPos = str->dwPos;				// insert address of text

	PutS(str,_T("GROB "));
	if ((bErr = DoHexStream(pdwAddr,str,pnLevel)))
	{
		*pdwAddr = dwObjStart;				// continue decoding behind prolog
		str->dwPos = dwBufferPos;			// throw out all output
	}
	return bErr;
}

static BOOL DoLib(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	DWORD dwObjStart,dwBufferPos;
	BOOL bErr;

	dwObjStart = *pdwAddr;					// remember start position
	dwBufferPos = str->dwPos;				// insert address of text

	PutS(str,_T("LIB "));
	if ((bErr = DoHexStream(pdwAddr,str,pnLevel)))
	{
		*pdwAddr = dwObjStart;				// continue decoding behind prolog
		str->dwPos = dwBufferPos;			// throw out all output
	}
	return bErr;
}

static BOOL DoBak(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	DWORD dwObjStart,dwBufferPos;
	BOOL bErr;

	dwObjStart = *pdwAddr;					// remember start position
	dwBufferPos = str->dwPos;				// insert address of text

	PutS(str,_T("BAK "));
	if ((bErr = DoHexStream(pdwAddr,str,pnLevel)))
	{
		*pdwAddr = dwObjStart;				// continue decoding behind prolog
		str->dwPos = dwBufferPos;			// throw out all output
	}
	return bErr;
}

static BOOL DoExt0(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	DWORD dwObjStart,dwBufferPos;
	BOOL bErr;

	dwObjStart = *pdwAddr;					// remember start position
	dwBufferPos = str->dwPos;				// insert address of text

	PutS(str,_T("LIBDAT "));
	if ((bErr = DoHexStream(pdwAddr,str,pnLevel)))
	{
		*pdwAddr = dwObjStart;				// continue decoding behind prolog
		str->dwPos = dwBufferPos;			// throw out all output
	}
	return bErr;
}

static BOOL DoExt1(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	DWORD dwObjStart,dwBufferPos;
	BOOL bErr;

	dwObjStart = *pdwAddr;					// remember start position
	dwBufferPos = str->dwPos;				// insert address of text

	if (dwRplPlatform <= RPL_P3)			// DOEXT1
	{
		PutS(str,_T("EXT1 "));
		bErr = DoHexStream(pdwAddr,str,pnLevel);
	}
	else									// DOACPTR
	{
		PutS(str,_T("ACPTR "));
		bErr = BINx(pdwAddr,5,str);			// BIN5
		PutC(str,_T(' '));
		bErr = bErr || BINx(pdwAddr,5,str);	// BIN5
	}

	if (bErr)								// decoding error
	{
		*pdwAddr = dwObjStart;				// continue decoding behind prolog
		str->dwPos = dwBufferPos;			// throw out all output
	}
	return bErr;
}

static BOOL DoExt2(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	DWORD dwObjStart,dwBufferPos;
	BOOL bErr;

	dwObjStart = *pdwAddr;					// remember start position
	dwBufferPos = str->dwPos;				// insert address of text

	PutS(str,_T("EXT2 "));
	if ((bErr = DoHexStream(pdwAddr,str,pnLevel)))
	{
		*pdwAddr = dwObjStart;				// continue decoding behind prolog
		str->dwPos = dwBufferPos;			// throw out all output
	}
	return bErr;
}

static BOOL DoExt3(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	DWORD dwObjStart,dwBufferPos;
	BOOL bErr;

	dwObjStart = *pdwAddr;					// remember start position
	dwBufferPos = str->dwPos;				// insert address of text

	PutS(str,_T("EXT3 "));
	if ((bErr = DoHexStream(pdwAddr,str,pnLevel)))
	{
		*pdwAddr = dwObjStart;				// continue decoding behind prolog
		str->dwPos = dwBufferPos;			// throw out all output
	}
	return bErr;
}

static BOOL DoExt4(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	DWORD dwObjStart,dwBufferPos;
	BOOL bErr;

	dwObjStart = *pdwAddr;					// remember start position
	dwBufferPos = str->dwPos;				// insert address of text

	PutS(str,_T("EXT4 "));
	if ((bErr = DoHexStream(pdwAddr,str,pnLevel)))
	{
		*pdwAddr = dwObjStart;				// continue decoding behind prolog
		str->dwPos = dwBufferPos;			// throw out all output
	}
	return bErr;
}

static BOOL DoZint(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	DWORD dwObjStart,dwBufferPos;
	BOOL bErr;

	dwObjStart = *pdwAddr;					// remember start position
	dwBufferPos = str->dwPos;				// insert address of text

	PutS(str,_T("ZINT "));
	if ((bErr = DoIntStream(pdwAddr,str,pnLevel)))
	{
		*pdwAddr = dwObjStart;				// continue decoding behind prolog
		str->dwPos = dwBufferPos;			// throw out all output
	}
	return bErr;
}

static BOOL DoLngReal(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	DWORD dwObjStart,dwBufferPos,dwExpLen,dwAddr;
	BOOL bErr;

	dwObjStart = *pdwAddr;					// remember start position
	dwBufferPos = str->dwPos;				// insert address of text

	bErr = DoIntStream(pdwAddr,str,pnLevel);
	PutC(str,_T('.'));

	dwAddr = *pdwAddr;						// copy of address for Readx()
	dwExpLen = Readx(&dwAddr,5);			// length of exponent
	if (dwExpLen > 6)						// non zero exponent
	{
		PutC(str,_T('E'));
		bErr = bErr || DoIntStream(pdwAddr,str,pnLevel);
	}
	else									// no exponent
	{
		Readx(pdwAddr,dwExpLen);			// skip exponent
	}

	if (bErr)								// decoding error
	{
		*pdwAddr = dwObjStart;				// continue decoding behind prolog
		str->dwPos = dwBufferPos;			// throw out all output
	}
	return bErr ;
}

static BOOL DoLngCmp(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	DWORD dwObjStart,dwBufferPos;
	BOOL bErr;

	dwObjStart = *pdwAddr;					// remember start position
	dwBufferPos = str->dwPos;				// insert address of text

	PutC(str,_T('('));
	bErr = DoLngReal(pdwAddr,str,pnLevel);
	PutC(str,_T(','));
	bErr = bErr || DoLngReal(pdwAddr,str,pnLevel);
	PutC(str,_T(')'));

	if (bErr)								// decoding error
	{
		*pdwAddr = dwObjStart;				// continue decoding behind prolog
		str->dwPos = dwBufferPos;			// throw out all output
	}
	return bErr;
}

static BOOL DoFlashPtr(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	DWORD dwObjStart,dwBufferPos;
	BOOL bErr;

	dwObjStart = *pdwAddr;					// remember start position
	dwBufferPos = str->dwPos;				// insert address of text

	PutS(str,_T("FLASHPTR "));
	bErr = BINx(pdwAddr,3,str);				// BIN3
	PutC(str,_T(' '));
	bErr = bErr || BINx(pdwAddr,4,str);		// BIN4

	if (bErr)								// decoding error
	{
		*pdwAddr = dwObjStart;				// continue decoding behind prolog
		str->dwPos = dwBufferPos;			// throw out all output
	}
	return bErr;
	UNREFERENCED_PARAMETER(pnLevel);
}

static BOOL DoAplet(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	DWORD dwObjStart,dwBufferPos;
	BOOL bErr;

	dwObjStart = *pdwAddr;					// remember start position
	dwBufferPos = str->dwPos;				// insert address of text

	PutS(str,_T("APLET "));
	if ((bErr = DoHexStream(pdwAddr,str,pnLevel)))
	{
		*pdwAddr = dwObjStart;				// continue decoding behind prolog
		str->dwPos = dwBufferPos;			// throw out all output
	}
	return bErr;
}

static BOOL DoMinifont(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	DWORD dwObjStart,dwBufferPos;
	BOOL bErr;

	dwObjStart = *pdwAddr;					// remember start position
	dwBufferPos = str->dwPos;				// insert address of text

	PutS(str,_T("MINIFONT "));
	if ((bErr = DoHexStream(pdwAddr,str,pnLevel)))
	{
		*pdwAddr = dwObjStart;				// continue decoding behind prolog
		str->dwPos = dwBufferPos;			// throw out all output
	}
	return bErr;
}

static BOOL DoMatrix(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	DWORD dwObjStart,dwBufferPos;
	BOOL bErr;

	dwObjStart = *pdwAddr;					// remember start position
	dwBufferPos = str->dwPos;				// insert address of text

	PutS(str,_T("MATRIX"));
	if ((bErr = DoSemiStream(pdwAddr,str,pnLevel,_T(";"))))
	{
		*pdwAddr = dwObjStart;				// continue decoding behind prolog
		str->dwPos = dwBufferPos;			// throw out all output
	}
	return bErr;
}

static struct ObjHandler
{
	LPCTSTR lpszName;						// object name
	// decode function
	BOOL (*fp)(DWORD *pdwAddr,String *str,UINT *pnLevel);
	DWORD	dwType;							// calculator type (RPL_P1 = all)
} ObjDecode[] =
{
	_T("DOBINT"),		DoBint,			RPL_P1,		// System Binary
	_T("DOREAL"),		DoReal,			RPL_P1,		// Real
	_T("DOEREL"),		DoERel,			RPL_P1,		// Long Real
	_T("DOCMP"),		DoCmp,			RPL_P1,		// Complex
	_T("DOECMP"),		DoECmp,			RPL_P1,		// Long Complex
	_T("DOCHAR"),		DoChar,			RPL_P1,		// Character
	_T("DOARRY"),		DoArry,			RPL_P1,		// Array
	_T("DOLNKARRY"),	DoLnkArry,		RPL_P1,		// Linked Array
	_T("DOCSTR"),		DoCStr,			RPL_P1,		// String
	_T("DOHSTR"),		DoHxs,			RPL_P1,		// Binary Integer
	_T("DOHXS"),		DoHxs,			RPL_P1,		// Binary Integer
	_T("DOLIST"),		DoList,			RPL_P1,		// List
	_T("DOSYMB"),		DoSymb,			RPL_P1,		// Algebraic
	_T("DOCOL"),		DoCol,			RPL_P1,		// Program
	_T("DOCODE"),		DoCode,			RPL_P1,		// Code
	_T("DOIDNT"),		DoIdnt,			RPL_P1,		// Global Name
	_T("DOLAM"),		DoLam,			RPL_P1,		// Local Name
	_T("DOROMP"),		DoRomp,			RPL_P1,		// XLIB Name
	_T("SEMI"),			Semi,			RPL_P1,		// SEMI
	_T("DORRP"),		DoRrp,			RPL_P2,		// Directory
	_T("DOEXT"),		DoExt,			RPL_P2,		// Reserved or Unit (HP48 and later)
	_T("DOTAG"),		DoTag,			RPL_P3,		// Tagged
	_T("DOGROB"),		DoGrob,			RPL_P3,		// Graphic
	_T("DOLIB"),		DoLib,			RPL_P3,		// Library
	_T("DOBAK"),		DoBak,			RPL_P3,		// Backup
	_T("DOEXT0"),		DoExt0,			RPL_P3,		// Library Data
	_T("DOEXT1"),		DoExt1,			RPL_P3,		// Ext1 or ACcess PoinTeR
	_T("DOACPTR"),		DoExt1,			RPL_P3,		// Ext1 or ACcess PoinTeR
	_T("DOEXT2"),		DoExt2,			RPL_P3,		// Reserved 1, Font (HP49G)
	_T("DOEXT3"),		DoExt3,			RPL_P3,		// Reserved 2
	_T("DOEXT4"),		DoExt4,			RPL_P3,		// Reserved 3
	_T("DOINT"),		DoZint,			RPL_P5,		// Infinite Precision Integers
	_T("DOLNGREAL"),	DoLngReal,		RPL_P5,		// Precision Real
	_T("DOLNGCMP"),		DoLngCmp,		RPL_P5,		// Precision Complex
	_T("DOFLASHP"),		DoFlashPtr,		RPL_P5,		// Flash Pointer
	_T("DOAPLET"),		DoAplet,		RPL_P5,		// Aplet
	_T("DOMINIFONT"),	DoMinifont,		RPL_P5,		// Mini Font
	_T("DOMATRIX"),		DoMatrix,		RPL_P5,		// Symbolic matrix
};

static BOOL (*Getfp(LPCTSTR lpszObject))(DWORD *,String *,UINT *)
{
	UINT i;

	for (i = 0; i < ARRAYSIZEOF(ObjDecode); ++i)
	{
		if (lstrcmp(lpszObject,ObjDecode[i].lpszName) == 0)
		{
			// RPL platform type enabled
			if ((ObjDecode[i].dwType & dwRplPlatform) == ObjDecode[i].dwType)
				return ObjDecode[i].fp;		// return object handler
		}
	}
	return NULL;							// not found
}

static BOOL FetchObj(DWORD *pdwAddr,String *str,UINT *pnLevel)
{
	LPCTSTR lpszName;
	DWORD   dwObject;
	BOOL    bErr;

	bErr = FALSE;

	PutCondSpc(str);						// write blank if necessary

	dwObject = Readx(pdwAddr,5);			// fetch object

	lpszName = RplGetName(dwObject);		// get name of object
	if (lpszName != NULL)
	{
		// look for object type
		BOOL (*fp)(DWORD *,String *,UINT *) = Getfp(lpszName);

		if (fp != NULL)						// found an object handler
		{
			bErr = fp(pdwAddr,str,pnLevel);	// call the handler
		}
		else
		{
			PutS(str,lpszName);				// named entry
		}
	}

	if (lpszName == NULL || bErr)			// no name or illegal object behind prolog
	{
		PutFS(str,_T("PTR %X"),dwObject);	// unnamed entry
	}
	return bErr;
}

BYTE (*RplReadNibble)(DWORD *p) = NULL;		// get nibble function pointer

DWORD RplSkipObject(DWORD dwAddr)
{
	UINT nLevel = 1;						// nest DOCOL objects

	_ASSERT(RplReadNibble != NULL);			// get nibble function defined
	FetchObj(&dwAddr,NULL,&nLevel);			// decode object without output
	return dwAddr;
}

LPTSTR RplDecodeObject(DWORD dwAddr, DWORD *pdwNxtAddr)
{
	String	str = { 0 };
	DWORD   dwNxtAddr;
	UINT    nLevel = 0;						// don't nest DOCOL objects

	dwNxtAddr = dwAddr;						// init next address

	_ASSERT(RplReadNibble != NULL);			// get nibble function defined
	FetchObj(&dwNxtAddr,&str,&nLevel);		// decode object

	PutC(&str,0);							// set EOS

	// release unnecessary allocated buffer memory
	VERIFY(str.szBuffer = (LPTSTR) realloc(str.szBuffer,str.dwPos * sizeof(str.szBuffer[0])));

	// return address of next object
	if (pdwNxtAddr != NULL) *pdwNxtAddr = dwNxtAddr;
	return str.szBuffer;
}


//################
//#
//#    RPL Object Viewer
//#
//################

BOOL bRplViewName = TRUE;					// show entry point name
BOOL bRplViewAddr = TRUE;					// show address
BOOL bRplViewBin = TRUE;					// show binary data
BOOL bRplViewAsm = TRUE;					// show ASM code instead of hex data

static VOID PrintHead(DWORD dwStartAddr, DWORD dwEndAddr, String *str)
{
	if (bRplViewAddr)						// show address for object
	{
		PutFS(str,_T("%05X "),dwStartAddr);
	}

	if (bRplViewBin)						// show object binary data
	{
		DWORD dwIndex;

		for (dwIndex = 0; dwIndex < 5; ++dwIndex)
		{
			TCHAR c = _T(' ');

			if (dwStartAddr < dwEndAddr)	// still show hex nibble
			{
				c = cHex[RplReadNibble(&dwStartAddr)];
			}

			PutC(str,c);					// write data content
		}

		PutS(str,_T("  "));
	}
	else									// no binary data
	{
		if (bRplViewAddr)					// missing whitespace
		{
			PutC(str,_T(' '));
		}
	}
	return;
}

static VOID PrintTail(DWORD dwStartAddr, DWORD dwEndAddr, String *str)
{
	if (bRplViewBin)						// show binary data
	{
		DWORD dwActAddr,dwRemain;

		if (dwStartAddr < dwEndAddr)		// remaining data to show
		{
			for (dwActAddr = dwStartAddr; dwActAddr < dwEndAddr;)
			{
				if (bRplViewAddr)			// address is visible
				{
					// spaces instead of show address
					PutS(str,_T("      "));

					// address has 6 digit
					if (dwActAddr >= 0x100000)
						PutC(str,_T(' '));
				}

				dwRemain = dwEndAddr - dwActAddr;
				if (dwRemain > 5) dwRemain = 5;

				for (; dwRemain > 0; --dwRemain)
				{
					// write data content
					PutC(str,cHex[RplReadNibble(&dwActAddr)]);
				}

				PutS(str,_T("\r\n"));
			}
		}
	}
	return;
}

static VOID PrintBlank(String *str)
{
	if (bRplViewAddr)						// address is visible
	{
		PutS(str,_T("     "));				// spaces instead of address
	}
	if (bRplViewBin)						// show binary data
	{
		if (bRplViewAddr)					// address is visible
		{
			PutC(str,_T(' '));				// need separator
		}

		PutS(str,_T("     "));				// spaces instead of binary
	}
	if (bRplViewAddr || bRplViewBin)		// any prefix
	{
		PutS(str,_T("  "));					// need separator
	}
	return;
}

static VOID PrintLevel(DWORD dwLevel, String *str)
{
	for (; dwLevel > 0; --dwLevel)			// level depending blanks
	{
		PutS(str,_T("  "));
	}
	return;
}

#if defined HARDWARE						// compiled inside emulator sources
static DWORD AssemblyOutput(DWORD dwAddr, DWORD dwEndAddr, DWORD dwLevel, String *str)
{
	LPCTSTR lpszName;
	TCHAR   cBuffer[64];
	DWORD   dwNxtAddr;

	PrintLevel(dwLevel,str);				// level depending blanks
	PutS(str,_T("CODE\r\n"));				// code preamble

	PrintTail(dwAddr+5,dwAddr+10,str);		// write code length information

	++dwLevel;

	dwAddr += 10;							// start of assembler code

	for (; dwAddr < dwEndAddr; dwAddr = dwNxtAddr)
	{
		// entry name enabled and known entry?
		if (bRplViewName && (lpszName = RplGetName(dwAddr)) != NULL)
		{
			PrintHead(dwAddr,dwAddr,str);	// write head for assembly label
			PutFS(str,_T("=%s\r\n"),lpszName);
		}

		// disassemble line
		dwNxtAddr = disassemble(dwAddr,cBuffer);

		PrintHead(dwAddr,dwNxtAddr,str);	// write head of assembly line
		PrintLevel(dwLevel,str);			// level depending blanks
		PutS(str,cBuffer);					// write disassembler text
		PutS(str,_T("\r\n"));

		// write additional binary data of opcode
		PrintTail(dwAddr+5,dwNxtAddr,str);
	}

	--dwLevel;

	PrintBlank(str);						// skip address and binary part
	PrintLevel(dwLevel,str);				// level depending blanks
	PutS(str,_T("ENDCODE"));				// code postamble
	return dwAddr;
}
#endif

LPTSTR RplCreateObjView(DWORD dwStartAddr, DWORD dwEndAddr, BOOL bSingleObj)
{
	String	str = { 0 };
	LPCTSTR lpszName;
	LPTSTR  lpszObject;
	DWORD   dwLevel,dwAddr,dwNxtAddr;

	_ASSERT(RplReadNibble != NULL);			// get nibble function defined

	lpszObject = NULL;						// no memory allocated
	dwLevel = 0;							// nesting level

	// decode object
	for (dwAddr = dwStartAddr;dwAddr < dwEndAddr; dwAddr = dwNxtAddr)
	{
		lpszObject = RplDecodeObject(dwAddr,&dwNxtAddr);

		if (dwLevel > 0 && lstrcmp(lpszObject,_T(";")) == 0)
			--dwLevel;

		// entry name enabled and known entry?
		if (bRplViewName && (lpszName = RplGetName(dwAddr)) != NULL)
		{
			if (bRplViewAddr)				// show address for entry
			{
				PutFS(&str,_T("%05X "),dwAddr);
			}
			PutFS(&str,_T("=%s\r\n"),lpszName);
		}

		PrintHead(dwAddr,dwAddr+5,&str);	// write initial head

		do
		{
			// check for special RRP handling
			if (_tcsncmp(lpszObject,_T("DIR\n"),4) == 0)
			{
				LPCTSTR lpszStart,lpszEnd;

				lpszStart = lpszEnd = lpszObject;

				// decode lines
				while (*lpszEnd != 0)
				{
					// separate lines
					if ((lpszEnd = _tcschr(lpszStart,_T('\n'))) == NULL)
					{
						VERIFY(lpszEnd = _tcschr(lpszStart,0));
					}

					if (dwLevel > 0 && _tcsncmp(lpszStart,_T("ENDDIR"),lpszEnd-lpszStart) == 0)
						--dwLevel;

					// level depending blanks
					PrintLevel(dwLevel,&str);

					// write line without LF
					PutSn(&str,lpszStart,(DWORD) (lpszEnd-lpszStart));

					if (_tcsncmp(lpszStart,_T("DIR"),lpszEnd-lpszStart) == 0)
						++dwLevel;

					if (*lpszEnd != 0)		// more data?
					{
						PutS(&str,_T("\r\n")); // send CR LF
						PrintBlank(&str);
						lpszStart = lpszEnd + 1; // next part
					}
				}
				break;
			}

			#if defined HARDWARE			// compiled inside emulator sources
			// check for special CODE handling
			if (bRplViewAsm && _tcsncmp(lpszObject,_T("CODE "),5) == 0)
			{
				// replace object by a disassembler output
				dwAddr = AssemblyOutput(dwAddr,dwNxtAddr,dwLevel,&str);
				break;
			}
			#endif

			// default, show the object
			PrintLevel(dwLevel,&str);		// level depending blanks
			PutS(&str,lpszObject);			// the object
		}
		while (FALSE);
		PutS(&str,_T("\r\n"));

		PrintTail(dwAddr+5,dwNxtAddr,&str);	// write additional binary data

		if (lstrcmp(lpszObject,_T("::")) == 0)
			++dwLevel;

		free(lpszObject);					// free object string
		lpszObject = NULL;

		if (   (bSingleObj && dwLevel == 0)	// single object decoding?
			|| (dwNxtAddr < dwAddr))		// or address wrap around
			break;							// stop decoding
	}

	_ASSERT(lpszObject == NULL);

	// remove CR LF at end
	while (   str.dwPos > 0
		   && (   str.szBuffer[str.dwPos-1] == _T('\r')
		       || str.szBuffer[str.dwPos-1] == _T('\n')
			  )
		  )
	{
		str.dwPos--;
	}

	PutC(&str,0);							// set EOS

	// release unnecessary allocated buffer memory
	VERIFY(str.szBuffer = (LPTSTR) realloc(str.szBuffer,str.dwPos * sizeof(str.szBuffer[0])));
	return str.szBuffer;
}
