/*
 *   stack.c
 *
 *   This file is part of Emu48
 *
 *   Copyright (C) 2005 Christoph Gießelink
 *
 */
#include "pch.h"
#include "Emu48.h"
#include "io.h"

#define fnRadix		51						// fraction mark
#define fnApprox	105						// exact / approx. mode (HP49G)

#define DOINT		0x02614					// Precision Integer (HP49G)
#define DOREAL		0x02933					// Real
#define DOCMP		0x02977					// Complex
#define DOCSTR		0x02A2C					// String

BOOL bDetectClpObject = TRUE;				// try to detect clipboard object
BOOL bLocaleDecimalPoint = FALSE;			// use decimal point character from calculator

//################
//#
//#    Low level subroutines
//#
//################

//
// check if cGroup character is thousands separator
//
static BOOL CheckThousandGroup(LPCTSTR cp,TCHAR cGroup)
{
	UINT uLastPos;
	UINT i;

	// get decimal point
	CONST TCHAR cDecimalPoint = cGroup ^ (_T('.') ^ _T(','));

	BOOL bFound = FALSE;					// 1st separator not found
	BOOL bPosOK = TRUE;

	for (i = 0; bPosOK && cp[i] != cDecimalPoint && cp[i] != 0; ++i)
	{
		if (cp[i] == cGroup)				// found separator
		{
			if (bFound)
			{
				// verify separator position
				bPosOK = (uLastPos + 4 == i);
			}

			uLastPos = i;					// last position of separator
			bFound = TRUE;					// separator found
		}
	}

	// check last grouping
	return bPosOK && bFound && (uLastPos + 4 == i);
}

//
// get decimal point from clipboard
//
static TCHAR GetClpbrdDecimalPoint(LPCTSTR cp)
{
	TCHAR cDec = 0;							// default for invalid decimal point detection

	TCHAR cLast = 0;						// last decimal point
	UINT uPoint = 0;						// no. of points
	UINT uComma = 0;						// no. of commas

	LPCTSTR p;
	for (p = cp; *p; ++p)					// count '.' and ',' characters
	{
		if (*p == _T('.'))
		{
			cLast = *p;						// last occurance
			++uPoint;
		}
		if (*p == _T(','))
		{
			cLast = *p;						// last occurance
			++uComma;
		}
	}

	if (uComma == 0 && uPoint == 0)			// none of both
	{
		cDec = _T('.');
	}
	else if (uComma == 1 && uPoint == 0)	// one single ','
	{
		cDec = _T(',');
	}
	else if (uComma == 0 && uPoint == 1)	// one single '.'
	{
		cDec = _T('.');
	}
	else if (uComma == 1 && uPoint == 1)	// one single ',' and '.'
	{
		// change from ',' to '.' or vice versa
		const TCHAR cFirst = cLast ^ (_T('.') ^ _T(','));

		if (CheckThousandGroup(cp,cFirst))	// check if 1st character is grouped
		{
			cDec = cLast;
		}
	}
	// multiple grouped ',' and single or none '.'
	else if (uComma > 1 && uPoint <= 1 && CheckThousandGroup(cp,_T(',')))
	{
		cDec = _T('.');
	}
	// multiple grouped '.' and single or none ','
	else if (uComma <= 1 && uPoint > 1 && CheckThousandGroup(cp,_T('.')))
	{
		cDec = _T(',');
	}
	return cDec;
}

static LPTSTR Trim(LPCTSTR cp)
{
	LPCTSTR pcWs = _T(" \t\n\r");			// valid whitespace characters

	LPTSTR pc;
	DWORD  dwFirst,dwLast;

	dwLast = lstrlen(cp);					// last position in string (EOS)

	// trim leading and tailing whitespace characters
	dwFirst = (DWORD) _tcsspn(cp,pcWs);		// position of 1st non whitespace character

	// search for position behind last non whitespace character 
	while (dwLast > dwFirst && _tcschr(pcWs,cp[dwLast-1]) != NULL)
		--dwLast;

	dwLast = 1 + dwLast - dwFirst;			// calculate buffer length

	if ((pc = (LPTSTR) malloc(dwLast * sizeof(*pc))) != NULL)
	{
		lstrcpyn(pc,&cp[dwFirst],dwLast);	// copy relevant data + EOS
	}
	return pc;
}

static INT RPL_GetZInt(BYTE CONST *pbyNum,INT nIntLen,LPTSTR cp,INT nSize)
{
	INT i = 0;								// character counter

	_ASSERT(nSize > 0);						// target buffer size

	if (nIntLen > 1)						// has sign nibble
	{
		--nIntLen;							// remove sign from digit length

		// check for valid sign
		_ASSERT(pbyNum[nIntLen] == 0 || pbyNum[nIntLen] == 9);
		if (pbyNum[nIntLen] == 9)			// negative number
		{
			*cp++ = _T('-');				// add sign
			--nSize;						// dec dest buffer size
			++i;							// wrote one character
		}
	}

	if (nIntLen >= nSize) return 0;			// dest buffer overflow
	i += nIntLen;							// adjust character counter

	while (nIntLen-- > 0)					// write all digits
	{
		// check for valid digit
		_ASSERT(pbyNum[nIntLen] >= 0 && pbyNum[nIntLen] <= 9);
		*cp++ = _T('0') + pbyNum[nIntLen];	// and write
	}
	*cp = 0;								// set EOS
	return i;
}

static __inline INT SetZInt(LPCTSTR cp,LPBYTE pbyNum,INT nSize)
{
	BYTE bySign;
	INT  nStrLen,nNumSize;

	_ASSERT(nSize > 0);						// target buffer size

	nStrLen = lstrlen(cp);					// source string length

	if (   nStrLen == 0						// empty string
		// precisition integer contain only these numbers
		|| _tcsspn(cp,_T("0123456789+-")) != (SIZE_T) nStrLen)
		return 0;

	bySign = (*cp != _T('-')) ? 0 : 9;		// set sign nibble
	if (*cp == _T('-') || *cp == _T('+'))	// skip sign character
	{
		++cp;
		--nStrLen;
	}

	if (nStrLen == 1 && *cp == _T('0'))		// special code for zero
	{
		*pbyNum = 0;						// zero data
		return 1;							// finish
	}

	// nStrLen = no. of digits without sign
	if (nStrLen >= nSize)					// destination buffer too small
		return 0;

	nNumSize = nStrLen + 1;					// no. of written data

	while (--nStrLen >= 0)					// eval all digits
	{
		TCHAR c = cp[nStrLen];

		// only '0' .. '9' are valid here
		if (!((c >= _T('0')) || (c <= _T('9'))))
			return 0;

		c -= _T('0');
		*pbyNum++ = (BYTE) c;
	}
	*pbyNum = bySign;						// add sign

	return nNumSize;
}

static INT RPL_SetZInt(LPCTSTR cp,LPBYTE pbyNum,INT nSize)
{
	LPTSTR pszData;

	INT s = 0;

	if ((pszData = Trim(cp)) != NULL)		// create a trimmed working copy of the string
	{
		s = SetZInt(pszData,pbyNum,nSize);
		free(pszData);
	}
	return s;
}

static INT RPL_GetBcd(BYTE CONST *pbyNum,INT nMantLen,INT nExpLen,CONST TCHAR cDec,LPTSTR cp,INT nSize)
{
	BYTE byNib;
	LONG v,lExp;
	BOOL bPflag,bExpflag;
	INT  i;

	lExp = 0;
	for (v = 1; nExpLen--; v *= 10)			// fetch exponent
	{
		lExp += (LONG) *pbyNum++ * v;		// calc. exponent
	}

	if (lExp > v / 2) lExp -= v;			// negative exponent

	lExp -= nMantLen - 1;					// set decimal point to end of mantissa

	i = 0;									// first character
	bPflag = FALSE;							// show no decimal point
	bExpflag = FALSE;						// show no exponent

	// scan mantissa
	for (v = (LONG) nMantLen - 1; v >= 0 || bPflag; v--)
	{
		if (v >= 0L)						// still mantissa digits left
			byNib = *pbyNum++;
		else
			byNib = 0;						// zero for negativ exponent

		if (!i)								// still delete zeros at end
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
		if ((bExpflag && v == 0) || (!lExp && i))
		{
			if (i >= nSize) return 0;		// dest buffer overflow
			cp[i++] = cDec;					// write decimal point
			if (v < 0)						// no mantissa digits any more
			{
				if (i >= nSize) return 0;	// dest buffer overflow
				cp[i++] = _T('0');			// write heading zero
			}
			bPflag = FALSE;					// finished with negativ exponents
		}

		if (v >= 0 || bPflag)
		{
			if (i >= nSize) return 0;		// dest buffer overflow
			cp[i++] = (TCHAR) byNib + _T('0'); // write character
		}

		lExp++;								// next position
	}

	if (*pbyNum == 9)						// negative number
	{
		if (i >= nSize) return 0;			// dest buffer overflow
		cp[i++] = _T('-');					// write sign
	}

	if (i >= nSize) return 0;				// dest buffer overflow
	cp[i] = 0;								// set EOS

	for (v = 0; v < (i / 2); v++)			// reverse string
	{
		TCHAR cNib = cp[v];					// swap chars
		cp[v] = cp[i-v-1];
		cp[i-v-1] = cNib;
	}

	// write number with exponent
	if (bExpflag)
	{
		if (i + 5 >= nSize) return 0;		// dest buffer overflow
		i += wsprintf(&cp[i],_T("E%d"),lExp-1);
	}
	return i;
}

static __inline INT SetBcd(LPCTSTR cp,INT nMantLen,INT nExpLen,CONST TCHAR cDec,LPBYTE pbyNum,INT nSize)
{
	TCHAR cVc[] = _T(",.0123456789eE+-");

	BYTE byNum[80];
	INT  i,nIp,nDp,nMaxExp;
	LONG lExp;

	// get thousand separator
	const TCHAR cThousand = cDec ^ (_T('.') ^ _T(','));

	if (   nMantLen + nExpLen >= nSize		// destination buffer too small
		|| !*cp								// empty string
		|| _tcsspn(cp,cVc) != (SIZE_T) lstrlen(cp) // real doesn't contain only these numbers
		|| (SIZE_T) lstrlen(cp) >= ARRAYSIZEOF(byNum)) // ignore too long reals
		return 0;

	byNum[0] = (*cp != _T('-')) ? 0 : 9;	// set sign nibble
	if (*cp == _T('-') || *cp == _T('+'))	// skip sign nibble
		cp++;

	// only '.', '0' .. '9' are valid here
	if (!((*cp == cDec) || (*cp >= _T('0')) || (*cp <= _T('9'))))
		return 0;

	nIp = 0;								// length of integer part
	if (*cp != cDec)						// no decimal point
	{
		// count integer part
		for (; (*cp >= _T('0') && *cp <= _T('9')) || *cp == cThousand; ++cp)
		{
			if (*cp != cThousand)			// not thousand separator
			{
				byNum[++nIp] = *cp - _T('0');
			}
		}
		if (!nIp) return 0;
	}

	// only '.', 'E', 'e' or end are valid here
	if (!(!*cp || (*cp == cDec) || (*cp == _T('E')) || (*cp == _T('e'))))
		return 0;

	nDp = 0;								// length of decimal part
	if (*cp == cDec)						// decimal point
	{
		cp++;								// skip '.'

		// count decimal part
		while (*cp >= _T('0') && *cp <= _T('9'))
			byNum[nIp + ++nDp] = *cp++ - _T('0');
	}

	// count number of heading zeros in mantissa
	for (i = 0; byNum[i+1] == 0 && i + 1 < nIp + nDp; ++i) { }

	if (i > 0)								// have to normalize
	{
		INT j;

		nIp -= i;							// for later ajust of exponent
		for (j = 1; j <= nIp + nDp; ++j)	// normalize mantissa
			byNum[j] = byNum[j + i];
	}

	if (byNum[1] == 0)						// number is 0
	{
		ZeroMemory(pbyNum,nMantLen + nExpLen + 1);
		return nMantLen + nExpLen + 1;
	}

	for (i = nIp + nDp; i < nMantLen;)		// fill rest of mantissa with 0
		byNum[++i] = 0;

	// must be 'E', 'e' or end
	if (!(!*cp || (*cp == _T('E')) || (*cp == _T('e'))))
		return 0;

	lExp = 0;
	if (*cp == _T('E') || *cp == _T('e'))
	{
		cp++;								// skip 'E'

		i = FALSE;							// positive exponent
		if (*cp == _T('-') || *cp == _T('+'))
		{
			i = (*cp++ == _T('-'));			// adjust exponent sign
		}

		// exponent symbol must be followed by number
		if (*cp < _T('0') || *cp > _T('9')) return 0;

		while (*cp >= _T('0') && *cp <= _T('9'))
			lExp = lExp * 10 + *cp++ - _T('0');

		if (i) lExp = -lExp;
	}

	if (*cp != 0) return 0;

	// adjust exponent value with exponent from normalized mantissa
	lExp += nIp - 1;

	// calculate max. posive exponent
	for (nMaxExp = 5, i = 1; i < nExpLen; ++i)
		nMaxExp *= 10;

	// check range of exponent
	if ((lExp < 0 && -lExp >= nMaxExp) || (lExp >= nMaxExp))
		return 0;

	if (lExp < 0) lExp += 2 * nMaxExp;		// adjust negative offset

	for (i = nExpLen; i > 0; --i)			// convert number into digits
	{
		byNum[nMantLen + i] = (BYTE) (lExp % 10);
		lExp /= 10;
	}

	// copy to target in reversed order
	for (i = nMantLen + nExpLen; i >= 0; --i)
		*pbyNum++ = byNum[i];

	return nMantLen + nExpLen + 1;
}

static INT RPL_SetBcd(LPCTSTR cp,INT nMantLen,INT nExpLen,CONST TCHAR cDec,LPBYTE pbyNum,INT nSize)
{
	LPTSTR pszData;

	INT s = 0;

	if ((pszData = Trim(cp)) != NULL)		// create a trimmed working copy of the string
	{
		s = SetBcd(pszData,nMantLen,nExpLen,cDec,pbyNum,nSize);
		free(pszData);
	}
	return s;
}

static INT RPL_GetComplex(BYTE CONST *pbyNum,INT nMantLen,INT nExpLen,CONST TCHAR cDec,LPTSTR cp,INT nSize)
{
	INT   nLen,nPos;
	TCHAR cSep;

	cSep = (cDec == _T('.'))				// current separator
		 ? _T(',')							// decimal point '.' -> ',' separator
		 : _T(';');							// decimal comma ',' -> ';' separator

	nPos = 0;								// write buffer position

	if (nSize < 5) return 0;				// target buffer to small
	nSize -= 4;								// reserved room for (,)\0

	cp[nPos++] = _T('(');					// start of complex number

	// real part
	nLen = RPL_GetBcd(pbyNum,nMantLen,nExpLen,cDec,&cp[1],nSize);
	if (nLen == 0) return 0;				// target buffer to small

	_ASSERT(nLen <= nSize);

	nPos += nLen;							// actual buffer postion
	nSize -= nLen;							// remainder target buffer size

	cp[nPos++] = cSep;						// write of complex number seperator

	// imaginary part
	nLen = RPL_GetBcd(&pbyNum[16],nMantLen,nExpLen,cDec,&cp[nPos],nSize);
	if (nLen == 0) return 0;				// target buffer to small

	nPos += nLen;							// actual buffer postion

	cp[nPos++] = _T(')');					// end of complex number
	cp[nPos] = 0;							// EOS

	_ASSERT(lstrlen(cp) == nPos);

	return nPos;
}

static INT RPL_SetComplex(LPCTSTR cp,INT nMantLen,INT nExpLen,CONST TCHAR cDec,LPBYTE pbyNum,INT nSize)
{
	LPTSTR pcSep,pszData;
	INT    nLen;
	TCHAR  cSep;

	nLen = 0;								// read data length

	cSep = (cDec == _T('.'))				// current separator
		 ? _T(',')							// decimal point '.' -> ',' separator
		 : _T(';');							// decimal comma ',' -> ';' separator

	if ((pszData = Trim(cp)) != NULL)		// create a trimmed working copy of the string
	{
		INT nStrLength = lstrlen(pszData);	// string length

		// complex number with brackets around
		if (   nStrLength > 0
			&& pszData[0]              == _T('(')
			&& pszData[nStrLength - 1] == _T(')'))
		{
			pszData[--nStrLength] = 0;		// replace ')' with EOS

			// search for number separator
			if ((pcSep = _tcschr(pszData+1,cSep)) != NULL)
			{
				INT nLen1st;

				*pcSep = 0;					// set EOS for 1st substring

				// decode 1st substring
				nLen1st = RPL_SetBcd(pszData+1,nMantLen,nExpLen,cDec,&pbyNum[0],nSize);
				if (nLen1st > 0)
				{
					// decode 2nd substring
					nLen = RPL_SetBcd(pcSep+1,nMantLen,nExpLen,cDec,&pbyNum[nMantLen+nExpLen+1],nSize-nLen1st);
					if (nLen > 0)
					{
						nLen += nLen1st;	// complete Bcd length
					}
				}
			}
		}
		free(pszData);
	}
	return nLen;
}


//################
//#
//#    Object subroutines
//#
//################

static TCHAR GetRadix(VOID)
{
	if (bLocaleDecimalPoint)				// use Windows Locale decimal point character
	{
		TCHAR cDecimal[2];

		// get locale decimal point with zero terminator
		GetLocaleInfo(LOCALE_USER_DEFAULT,LOCALE_SDECIMAL,cDecimal,2);
		return cDecimal[0];
	}

	return RPL_GetSystemFlag(fnRadix) ? _T(',') : _T('.');
}

static INT DoInt(DWORD dwAddress,LPTSTR cp,INT nSize)
{
	LPBYTE lpbyData;
	INT    nLength,nIntLen;

	nIntLen = Read5(dwAddress) - 5;			// no. of digits
	if (nIntLen <= 0) return 0;				// error in calculator object

	nLength = 0;
	if ((lpbyData = (LPBYTE) malloc(nIntLen)))
	{
		// get precisition integer object content and decode it
		Npeek(lpbyData,dwAddress+5,nIntLen);
		nLength = RPL_GetZInt(lpbyData,nIntLen,cp,nSize);
		free(lpbyData);
	}
	return nLength;
}

static INT DoReal(DWORD dwAddress,LPTSTR cp,INT nSize)
{
	BYTE byNumber[16];

	// get real object content and decode it
	Npeek(byNumber,dwAddress,ARRAYSIZEOF(byNumber));
	return RPL_GetBcd(byNumber,12,3,GetRadix(),cp,nSize);
}

static INT DoComplex(DWORD dwAddress,LPTSTR cp,INT nSize)
{
	BYTE byNumber[32];

	// get complex object content and decode it
	Npeek(byNumber,dwAddress,ARRAYSIZEOF(byNumber));
	return RPL_GetComplex(byNumber,12,3,GetRadix(),cp,nSize);
}


//################
//#
//#    Stack routines
//#
//################

//
// ID_STACK_COPY
//
LRESULT OnStackCopy(VOID)					// copy data from stack
{
	TCHAR  cBuffer[128];
	HANDLE hClipObj;
	LPBYTE lpbyData;
	DWORD  dwAddress,dwObject,dwSize;
	UINT   uClipboardFormat;

	_ASSERT(nState == SM_RUN);				// emulator must be in RUN state
	if (WaitForSleepState())				// wait for cpu SHUTDN then sleep state
	{
		InfoMessage(_T("The emulator is busy."));
		return 0;
	}

	_ASSERT(nState == SM_SLEEP);

	if ((dwAddress = RPL_Pick(1)) == 0)		// pick address of level1 object
	{
		MessageBeep(MB_OK);					// error beep
		goto error;
	}

	switch (dwObject = Read5(dwAddress))	// select object
	{
	case DOINT:  // Precision Integer (HP49G)
	case DOREAL: // real object
	case DOCMP:  // complex object
		dwAddress += 5;						// object content

		switch (dwObject)
		{
		case DOINT: // Precision Integer (HP49G)
			// get precision integer object content and decode it
			dwSize = DoInt(dwAddress,cBuffer,ARRAYSIZEOF(cBuffer));
			break;
		case DOREAL: // real object
			// get real object content and decode it
			dwSize = DoReal(dwAddress,cBuffer,ARRAYSIZEOF(cBuffer));
			break;
		case DOCMP: // complex object
			// get complex object content and decode it
			dwSize = DoComplex(dwAddress,cBuffer,ARRAYSIZEOF(cBuffer));
			break;
		}

		// calculate buffer size
		dwSize = (dwSize + 1) * sizeof(*cBuffer);

		// memory allocation for clipboard data
		if ((hClipObj = GlobalAlloc(GMEM_MOVEABLE,dwSize)) == NULL)
			goto error;

		if ((lpbyData = (LPBYTE) GlobalLock(hClipObj)))
		{
			// copy data to memory
			CopyMemory(lpbyData,cBuffer,dwSize);
			GlobalUnlock(hClipObj);			// unlock memory
		}

		#if defined _UNICODE
			uClipboardFormat = CF_UNICODETEXT;
		#else
			uClipboardFormat = CF_TEXT;
		#endif
		break;
	case DOCSTR: // string
		dwAddress += 5;						// address of string length
		dwSize = (Read5(dwAddress) - 5) / 2; // length of string

		// memory allocation for clipboard data
		if ((hClipObj = GlobalAlloc(GMEM_MOVEABLE,dwSize + 1)) == NULL)
			goto error;

		if ((lpbyData = (LPBYTE) GlobalLock(hClipObj))) // lock memory
		{
			// copy data into clipboard buffer
			for (dwAddress += 5;dwSize-- > 0;dwAddress += 2,++lpbyData)
				*lpbyData = Read2(dwAddress);
			*lpbyData = 0;					// set EOS

			GlobalUnlock(hClipObj);			// unlock memory
		}
		uClipboardFormat = CF_TEXT;			// always text
		break;
	default:
		MessageBeep(MB_OK);					// error beep
		goto error;
	}

	if (OpenClipboard(hWnd))
	{
		if (EmptyClipboard())
			SetClipboardData(uClipboardFormat,hClipObj);
		else
			GlobalFree(hClipObj);
		CloseClipboard();
	}
	else										// clipboard open failed
	{
		GlobalFree(hClipObj);
	}

error:
	SwitchToState(SM_RUN);
	return 0;
}

//
// ID_STACK_PASTE
//
LRESULT OnStackPaste(VOID)					// paste data to stack
{
	#if defined _UNICODE
		#define CF_TEXTFORMAT CF_UNICODETEXT
	#else
		#define CF_TEXTFORMAT CF_TEXT
	#endif

	HANDLE hClipObj;

	BOOL bSuccess = FALSE;

	// check if clipboard format is available
	if (!IsClipboardFormatAvailable(CF_TEXTFORMAT))
	{
		MessageBeep(MB_OK);					// error beep
		return 0;
	}

	SuspendDebugger();						// suspend debugger
	bDbgAutoStateCtrl = FALSE;				// disable automatic debugger state control

	// calculator off, turn on
	if (!(Chipset.IORam[BITOFFSET]&DON))
	{
		KeyboardEvent(TRUE,0,0x8000);
		Sleep(dwWakeupDelay);
		KeyboardEvent(FALSE,0,0x8000);

		// wait for sleep mode
		while (Chipset.Shutdn == FALSE) Sleep(0);
	}

	_ASSERT(nState == SM_RUN);				// emulator must be in RUN state
	if (WaitForSleepState())				// wait for cpu SHUTDN then sleep state
	{
		InfoMessage(_T("The emulator is busy."));
		goto cancel;
	}

	_ASSERT(nState == SM_SLEEP);

	if (OpenClipboard(hWnd))
	{
		if ((hClipObj = GetClipboardData(CF_TEXTFORMAT)))
		{
			LPCTSTR lpstrClipdata;
			LPBYTE  lpbyData;

			if ((lpstrClipdata = (LPCTSTR) GlobalLock(hClipObj)))
			{
				TCHAR cDec;
				BYTE  byNumber[128];
				DWORD dwAddress;

				INT s = 0;					// no valid object

				do
				{
					if (bDetectClpObject)	// autodetect clipboard object enabled
					{
					// HP49G or HP49G+ in exact mode
					if (   (cCurrentRomType == 'X' || cCurrentRomType == 'Q')
						&& !RPL_GetSystemFlag(fnApprox))
						{
							// try to convert string to HP49 precision integer
							s = RPL_SetZInt(lpstrClipdata,byNumber,sizeof(byNumber));

							if (s > 0)		// is a real number for exact mode
							{
								// get TEMPOB memory for HP49 precision integer object
								dwAddress = RPL_CreateTemp(s+5+5,TRUE);
								if ((bSuccess = (dwAddress > 0)))
								{
									Write5(dwAddress,DOINT); // prolog
									Write5(dwAddress+5,s+5); // size
									Nwrite(byNumber,dwAddress+10,s); // data

									// push object to stack
									RPL_Push(1,dwAddress);
								}
								break;
							}
						}

						cDec = GetClpbrdDecimalPoint(lpstrClipdata);

						if (cDec)			// valid decimal point
						{
							// try to convert string to real format
							_ASSERT(16 <= ARRAYSIZEOF(byNumber));
							s = RPL_SetBcd(lpstrClipdata,12,3,cDec,byNumber,sizeof(byNumber));
						}

						if (s > 0)			// is a real number
						{
							_ASSERT(s == 16); // length of real number BCD coded

							// get TEMPOB memory for real object
							dwAddress = RPL_CreateTemp(16+5,TRUE);
							if ((bSuccess = (dwAddress > 0)))
							{
								Write5(dwAddress,DOREAL); // prolog
								Nwrite(byNumber,dwAddress+5,s); // data

								// push object to stack
								RPL_Push(1,dwAddress);
							}
							break;
						}

						// search for ';' as separator
						cDec = (_tcschr(lpstrClipdata,_T(';')) != NULL)
							 ? _T(',')							// decimal comma
							 : _T('.');							// decimal point

						// try to convert string to complex format
						_ASSERT(32 <= ARRAYSIZEOF(byNumber));
						s = RPL_SetComplex(lpstrClipdata,12,3,cDec,byNumber,sizeof(byNumber));

						if (s > 0)			// is a real complex
						{
							_ASSERT(s == 32); // length of complex number BCD coded

							// get TEMPOB memory for complex object
							dwAddress = RPL_CreateTemp(16+16+5,TRUE);
							if ((bSuccess = (dwAddress > 0)))
							{
								Write5(dwAddress,DOCMP); // prolog
								Nwrite(byNumber,dwAddress+5,s); // data

								// push object to stack
								RPL_Push(1,dwAddress);
							}
							break;
						}
					}

					// any other format
					{
						DWORD dwSize = lstrlen(lpstrClipdata);
						if ((lpbyData = (LPBYTE) malloc(dwSize * 2)))
						{
							LPBYTE lpbySrc,lpbyDest;
							DWORD  dwLoop;

							#if defined _UNICODE
								// copy data UNICODE -> ASCII
								WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK,
													lpstrClipdata, dwSize,
													(LPSTR) lpbyData+dwSize, dwSize, NULL, NULL);
							#else
								// copy data
								memcpy(lpbyData+dwSize,lpstrClipdata,dwSize);
							#endif

							// unpack data
							lpbySrc = lpbyData+dwSize;
							lpbyDest = lpbyData;
							dwLoop = dwSize;
							while (dwLoop-- > 0)
							{
								BYTE byTwoNibs = *lpbySrc++;
								*lpbyDest++ = (BYTE) (byTwoNibs & 0xF);
								*lpbyDest++ = (BYTE) (byTwoNibs >> 4);
							}

							dwSize *= 2;	// size in nibbles

							// get TEMPOB memory for string object
							dwAddress = RPL_CreateTemp(dwSize+10,TRUE);
							if ((bSuccess = (dwAddress > 0)))
							{
								Write5(dwAddress,DOCSTR); // String
								Write5(dwAddress+5,dwSize+5); // length of String
								Nwrite(lpbyData,dwAddress+10,dwSize); // data

								// push object to stack
								RPL_Push(1,dwAddress);
							}
							free(lpbyData);
						}
					}
				}
				while (FALSE);

				GlobalUnlock(hClipObj);
			}
		}
		CloseClipboard();
	}

	SwitchToState(SM_RUN);					// run state
	while (nState!=nNextState) Sleep(0);
	_ASSERT(nState == SM_RUN);

	if (bSuccess == FALSE)					// data not copied
		goto cancel;

	KeyboardEvent(TRUE,0,0x8000);
	Sleep(dwWakeupDelay);
	KeyboardEvent(FALSE,0,0x8000);

	// wait for sleep mode
	while (Chipset.Shutdn == FALSE) Sleep(0);

cancel:
	bDbgAutoStateCtrl = TRUE;				// enable automatic debugger state control
	ResumeDebugger();
	return 0;
	#undef CF_TEXTFORMAT
}
