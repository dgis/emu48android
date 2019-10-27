/*
 *   display.c
 *
 *   This file is part of Emu48
 *
 *   Copyright (C) 1995 Sebastien Carlier
 *   Copyright (C) 2002 Christoph Gießelink
 *
 */
#include "pch.h"
#include "resource.h"
#include "Emu48.h"
#include "io.h"
#include "kml.h"

// #define DEBUG_DISPLAY					// switch for DISPLAY debug purpose

#define NOCOLORSGRAY	8					// no. of colors in gray scale mode
#define NOCOLORSBW		2					// no. of colors in black and white mode

#define DISPLAY_FREQ	19					// display update 1/frequency (1/64) in ms (gray scale mode)

#define B 0x00000000						// black
#define W 0x00FFFFFF						// white
#define I 0xFFFFFFFF						// ignore

#define LCD_ROW		(36*4)					// max. pixel per line

#define GRAYMASK(c)	(((((c)-1)>>1)<<24) \
					|((((c)-1)>>1)<<16) \
					|((((c)-1)>>1)<<8)  \
					|((((c)-1)>>1)))

#define DIBPIXEL4(d,p)	*((DWORD*)(d)) = ((*((DWORD*)(d)) & dwGrayMask) << 1) | (p); \
						*((LPBYTE*) &(d)) += 4

BOOL   bGrayscale = FALSE;
UINT   nBackgroundX = 0;
UINT   nBackgroundY = 0;
UINT   nBackgroundW = 0;
UINT   nBackgroundH = 0;
UINT   nLcdX = 0;
UINT   nLcdY = 0;
UINT   nLcdZoom = 1;						// memory DC zoom
UINT   nGdiXZoom = 1;						// GDI x zoom
UINT   nGdiYZoom = 1;						// GDI y zoom
HDC    hLcdDC = NULL;
HDC    hMainDC = NULL;
HDC    hAnnunDC = NULL;						// annunciator DC

BYTE (*GetLineCounter)(VOID) = NULL;
VOID (*StartDisplay)(BYTE byInitial) = NULL;
VOID (*StopDisplay)(VOID) = NULL;

static BYTE GetLineCounterGray(VOID);
static BYTE GetLineCounterBW(VOID);
static VOID StartDisplayGray(BYTE byInitial);
static VOID StartDisplayBW(BYTE byInitial);
static VOID StopDisplayGray(VOID);
static VOID StopDisplayBW(VOID);

static LPBYTE pbyLcd;

static HBITMAP hLcdBitmap;
static HBITMAP hMainBitmap;
static HBITMAP hAnnunBitmap;

static DWORD Pattern[16];
static BYTE  Buf[36];

static DWORD dwGrayMask;

static LARGE_INTEGER lLcdRef;				// reference time for VBL counter
static UINT uLcdTimerId = 0;

static BYTE byVblRef = 0;					// VBL stop reference

static DWORD dwKMLColor[64] =				// color table loaded by KML script
{
	W,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
	B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
	I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,
	I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I
};

static struct
{
	BITMAPINFOHEADER Lcd_bmih;
	RGBQUAD bmiColors[NOCOLORSGRAY];
} bmiLcd =
{
	{0x28,0/*x*/,0/*y*/,1,8,BI_RGB,0,0,0,NOCOLORSGRAY,0}
};

static __inline VOID BuildPattern(VOID)
{
	WORD i,j;
	for (i=0; i<16; ++i)
	{
		Pattern[i] = 0;
		for (j=8; j>0; j>>=1)
		{
			Pattern[i] = (Pattern[i] << 8) | ((i&j) != 0);
		}
	}
	return;
}

VOID UpdateContrast(BYTE byContrast)
{
	RGBQUAD c,b;
	INT i,nColors;

	// table for max. 8 colors
	const INT nCAdj[] = { 0, 1, 1, 2, 1, 2, 2, 3 };

	// when display is off use contrast 0
	if ((Chipset.IORam[BITOFFSET] & DON) == 0) byContrast = 0;

	c = *(RGBQUAD*)&dwKMLColor[byContrast];	   // pixel on  color
	b = *(RGBQUAD*)&dwKMLColor[byContrast+32]; // pixel off color

	// if background color is undefined, use color 0 for compatibility
	if (I == *(DWORD*)&b) b = *(RGBQUAD*)&dwKMLColor[0];

	nColors = bGrayscale ? (NOCOLORSGRAY-1) : (NOCOLORSBW-1);

	_ASSERT(nColors <= ARRAYSIZEOF(nCAdj));	// no. of colors must be smaller than entries in the gray color table

	// fill color palette of bitmap
	for (i = 0; i <= nColors; ++i)
	{
		bmiLcd.bmiColors[i] = b;
		bmiLcd.bmiColors[i].rgbRed   += ((INT) c.rgbRed   - (INT) b.rgbRed)   * nCAdj[i] / nCAdj[nColors];
		bmiLcd.bmiColors[i].rgbGreen += ((INT) c.rgbGreen - (INT) b.rgbGreen) * nCAdj[i] / nCAdj[nColors];
		bmiLcd.bmiColors[i].rgbBlue  += ((INT) c.rgbBlue  - (INT) b.rgbBlue)  * nCAdj[i] / nCAdj[nColors];
	}

	// update palette information
	_ASSERT(hLcdDC);
	SetDIBColorTable(hLcdDC,0,ARRAYSIZEOF(bmiLcd.bmiColors),bmiLcd.bmiColors);
	return;
}

VOID SetLcdColor(UINT nId, UINT nRed, UINT nGreen, UINT nBlue)
{
	dwKMLColor[nId&0x3F] = ((nRed&0xFF)<<16)|((nGreen&0xFF)<<8)|(nBlue&0xFF);
	return;
}

VOID SetLcdMode(BOOL bMode)
{
	if ((bGrayscale = bMode))
	{
		// set pixel update mask
		dwGrayMask = GRAYMASK(NOCOLORSGRAY);
		GetLineCounter = GetLineCounterGray;
		StartDisplay = StartDisplayGray;
		StopDisplay = StopDisplayGray;
	}
	else
	{
		// set pixel update mask
		dwGrayMask = GRAYMASK(NOCOLORSBW);
		GetLineCounter = GetLineCounterBW;
		StartDisplay = StartDisplayBW;
		StopDisplay = StopDisplayBW;
	}
	UpdateContrast(Chipset.contrast);
	return;
}

VOID CreateLcdBitmap(VOID)
{
	// create LCD bitmap
	bmiLcd.Lcd_bmih.biWidth = LCD_ROW;
	bmiLcd.Lcd_bmih.biHeight = -SCREENHEIGHT; // CdB for HP: add 64/80 line display for apples
	_ASSERT(hLcdDC == NULL);
	VERIFY(hLcdDC = CreateCompatibleDC(hWindowDC));
	VERIFY(hLcdBitmap = CreateDIBSection(hWindowDC,(BITMAPINFO*)&bmiLcd,DIB_RGB_COLORS,(VOID **)&pbyLcd,NULL,0));
	hLcdBitmap = (HBITMAP) SelectObject(hLcdDC,hLcdBitmap);
	_ASSERT(hPalette != NULL);
	SelectPalette(hLcdDC,hPalette,FALSE);	// set palette for LCD DC
	RealizePalette(hLcdDC);					// realize palette
	BuildPattern();							// build Nibble -> DIB mask pattern
	SetLcdMode(bGrayscale);					// init display update function pointer
	return;
}

VOID DestroyLcdBitmap(VOID)
{
	// set contrast palette to startup colors
	WORD i = 0;   dwKMLColor[i++] = W;
	while (i < 32) dwKMLColor[i++] = B;
	while (i < 64) dwKMLColor[i++] = I;

	GetLineCounter = NULL;
	StartDisplay = NULL;
	StopDisplay = NULL;

	if (hLcdDC != NULL)
	{
		// destroy LCD bitmap
		DeleteObject(SelectObject(hLcdDC,hLcdBitmap));
		DeleteDC(hLcdDC);
		hLcdDC = NULL;
		hLcdBitmap = NULL;
	}
	return;
}

BOOL CreateMainBitmap(LPCTSTR szFilename)
{
	_ASSERT(hWindowDC != NULL);
	VERIFY(hMainDC = CreateCompatibleDC(hWindowDC));
	if (hMainDC == NULL) return FALSE;		// quit if failed
	hMainBitmap = LoadBitmapFile(szFilename,TRUE);
	if (hMainBitmap == NULL)
	{
		DeleteDC(hMainDC);
		hMainDC = NULL;
		return FALSE;
	}
	hMainBitmap = (HBITMAP) SelectObject(hMainDC,hMainBitmap);
	_ASSERT(hPalette != NULL);
	VERIFY(SelectPalette(hMainDC,hPalette,FALSE));
	RealizePalette(hMainDC);
	return TRUE;
}

VOID DestroyMainBitmap(VOID)
{
	if (hMainDC != NULL)
	{
		// destroy Main bitmap
		DeleteObject(SelectObject(hMainDC,hMainBitmap));
		DeleteDC(hMainDC);
		hMainDC = NULL;
		hMainBitmap = NULL;
	}
	return;
}

//
// load annunciator bitmap
//
BOOL CreateAnnunBitmap(LPCTSTR szFilename)
{
	_ASSERT(hWindowDC != NULL);
	VERIFY(hAnnunDC = CreateCompatibleDC(hWindowDC));
	if (hAnnunDC == NULL) return FALSE;		// quit if failed
	hAnnunBitmap = LoadBitmapFile(szFilename,FALSE);
	if (hAnnunBitmap == NULL)
	{
		DeleteDC(hAnnunDC);
		hAnnunDC = NULL;
		return FALSE;
	}
	hAnnunBitmap = (HBITMAP) SelectObject(hAnnunDC,hAnnunBitmap);
	return TRUE;
}

//
// destroy annunciator bitmap
//
VOID DestroyAnnunBitmap(VOID)
{
	if (hAnnunDC != NULL)
	{
		VERIFY(DeleteObject(SelectObject(hAnnunDC,hAnnunBitmap)));
		DeleteDC(hAnnunDC);
		hAnnunDC = NULL;
		hAnnunBitmap = NULL;
	}
	return;
}

//****************
//*
//* LCD functions
//*
//****************

VOID UpdateDisplayPointers(VOID)
{
	EnterCriticalSection(&csLcdLock);
	{
		#if defined DEBUG_DISPLAY
		{
			TCHAR buffer[256];
			wsprintf(buffer,_T("%.5lx: Update Display Pointer\n"),Chipset.pc);
			OutputDebugString(buffer);
		}
		#endif

		// calculate display width
		Chipset.width = (34 + Chipset.loffset + (Chipset.boffset / 4) * 2) & 0xFFFFFFFE;
		Chipset.end1 = Chipset.start1 + MAINSCREENHEIGHT * Chipset.width;
		if (Chipset.end1 < Chipset.start1)
		{
			// calculate first address of main display
			Chipset.start12 = Chipset.end1 - Chipset.width;
			// calculate last address of main display
			Chipset.end1 = Chipset.start1 - Chipset.width;
		}
		else
		{
			Chipset.start12 = Chipset.start1;
		}
		Chipset.end2 = Chipset.start2 + MENUHEIGHT * 34;
	}
	LeaveCriticalSection(&csLcdLock);
	return;
}

VOID UpdateMainDisplay(VOID)
{
	UINT  x, y;
	BYTE *p = pbyLcd+(Chipset.d0size*LCD_ROW);  // CdB for HP: add 64/80 line display for apples
	DWORD d = Chipset.start1;

	#if defined DEBUG_DISPLAY
	{
		TCHAR buffer[256];
		wsprintf(buffer,_T("%.5lx: Update Main Display\n"),Chipset.pc);
		OutputDebugString(buffer);
	}
	#endif

	if (!(Chipset.IORam[BITOFFSET]&DON))
	{
		ZeroMemory(pbyLcd, LCD_ROW * SCREENHEIGHT);
	}
	else
	{
		for (y = 0; y < MAINSCREENHEIGHT; ++y)
		{
			Npeek(Buf,d,36);
			for (x=0; x<36; ++x)		// every 4 pixel
			{
				DIBPIXEL4(p,Pattern[Buf[x]]);
				// check for display buffer overflow
				_ASSERT(p <= pbyLcd + LCD_ROW * SCREENHEIGHT);
			}
			d+=Chipset.width;
		}
	}
	EnterCriticalSection(&csGDILock);		// solving NT GDI problems
	{
	// CdB for HP: add 64/80 line display for apples
		StretchBlt(hWindowDC, nLcdX, nLcdY+Chipset.d0size*nLcdZoom, 131*nLcdZoom*nGdiXZoom, MAINSCREENHEIGHT*nLcdZoom*nGdiYZoom,
			       hLcdDC, Chipset.boffset, Chipset.d0size, 131, MAINSCREENHEIGHT, SRCCOPY);
		GdiFlush();
	}
	LeaveCriticalSection(&csGDILock);
	return;
}

VOID UpdateMenuDisplay(VOID)
{
	UINT  x, y;
	BYTE  *p;
	DWORD d = Chipset.start2;

	#if defined DEBUG_DISPLAY
	{
		TCHAR buffer[256];
		wsprintf(buffer,_T("%.5lx: Update Menu Display\n"),Chipset.pc);
		OutputDebugString(buffer);
	}
	#endif

	if (!(Chipset.IORam[BITOFFSET]&DON)) return;
	if (MENUHEIGHT==0) return;				// menu disabled

	// calculate bitmap offset
	p = pbyLcd + ((Chipset.d0size+MAINSCREENHEIGHT)*LCD_ROW);  // CdB for HP: add 64/80 line display for apples
	for (y = 0; y < MENUHEIGHT; ++y)
	{
		Npeek(Buf,d,34);				// 34 nibbles are viewed
		for (x=0; x<34; ++x)			// every 4 pixel
		{
			DIBPIXEL4(p,Pattern[Buf[x]]);
			// check for display buffer overflow
			_ASSERT(p <= pbyLcd + LCD_ROW * SCREENHEIGHT);
		}
		// adjust pointer to 36 DIBPIXEL drawing calls
		p += (36-34) * sizeof(DWORD);
		d+=34;
	}
	EnterCriticalSection(&csGDILock);		// solving NT GDI problems
	{
		// CdB for HP: add 64/80 line display for apples
		StretchBlt(hWindowDC, 
			   nLcdX, nLcdY+(MAINSCREENHEIGHT+Chipset.d0size)*nLcdZoom*nGdiYZoom,
			   131*nLcdZoom*nGdiXZoom, MENUHEIGHT*nLcdZoom*nGdiYZoom,
			    hLcdDC, 
			   0, (MAINSCREENHEIGHT+Chipset.d0size), 
			   131, MENUHEIGHT, 
			   SRCCOPY);
		GdiFlush();
	}
	LeaveCriticalSection(&csGDILock);
	return;
}

// CdB for HP: add header management
VOID RefreshDisp0()
{
	UINT x, y;
	BYTE *p;
	BYTE* d = Chipset.d0memory;

	#if defined DEBUG_DISPLAY
	{
		TCHAR buffer[256];
		wsprintf(buffer,_T("%.5lx: Update header Display\n"),Chipset.pc);
		OutputDebugString(buffer);
	}
	#endif

	if (!(Chipset.IORam[BITOFFSET]&DON)) return;

	// calculate bitmap offset
	p = pbyLcd;
	for (y = 0; y<Chipset.d0size; ++y)
	{
		memcpy(Buf,d,34);				// 34 nibbles are viewed
		for (x=0; x<36; ++x)			// every 4 pixel
		{
			DIBPIXEL4(p,Pattern[Buf[x]]);
		}
		d+=34;
	}
	EnterCriticalSection(&csGDILock);		// solving NT GDI problems
	{
		StretchBlt(hWindowDC, nLcdX, nLcdY,
			       131*nLcdZoom*nGdiXZoom, Chipset.d0size*nLcdZoom*nGdiYZoom,
			       hLcdDC, Chipset.d0offset, 0, 131, Chipset.d0size, SRCCOPY);
		GdiFlush();
	}
	LeaveCriticalSection(&csGDILock);
	return;
}

VOID WriteToMainDisplay(LPBYTE a, DWORD d, UINT s)
{
	UINT x0, x;
	UINT y0, y;
	DWORD *p;

	INT  lWidth = abs(Chipset.width);		// display width

	if (bGrayscale) return;					// no direct writing in grayscale mode

	#if defined DEBUG_DISPLAY
	{
		TCHAR buffer[256];
		wsprintf(buffer,_T("%.5lx: Write Main Display %x,%u\n"),Chipset.pc,d,s);
		OutputDebugString(buffer);
	}
	#endif

	if (!(Chipset.IORam[BITOFFSET]&DON))	// display off
		return;								// no drawing

	if (MAINSCREENHEIGHT == 0) return;				// menu disabled

	d -= Chipset.start1;					// nibble offset to DISPADDR (start of display)
    d += SCREENHEIGHTREAL * lWidth;         // make positive offset
    y0 = y = abs((INT)d / lWidth - SCREENHEIGHTREAL);            // bitmap row
    x0 = x = (INT)d % lWidth;               // bitmap column
	p = (DWORD*)(pbyLcd + y0*LCD_ROW + x0*sizeof(*p));

	// outside main display area
//	_ASSERT(y0 >= (INT)Chipset.d0size && y0 < (INT)(MAINSCREENHEIGHT+Chipset.d0size));
	if (!(y0 >= (INT)Chipset.d0size && y0 < (INT)(MAINSCREENHEIGHT+Chipset.d0size))) return;

	while (s--)								// loop for nibbles to write
	{
		if (x<36)							// only fill visible area
		{
			*p = Pattern[*a];
		}
		++a;								// next value to write
		++x;								// next x position
		if (((INT) x==lWidth)&&s)			// end of display line
		{
            // end of main display area
            if (y == (INT)MAINSCREENHEIGHT + Chipset.d0size - 1) break;
			x = 0;							// first coloumn
			++y;							// next row

			// recalculate bitmap memory position of new line
			p = (DWORD*) (pbyLcd+y*LCD_ROW);  // CdB for HP: add 64/80 line display for apples
		}
		else
			p++;
	}

    // update window region
    if (y0 != y)                            // changed more than one line
    {
        x0 = 0;                             // no x-position offset
        x = 131;                            // redraw complete lines

        ++y;                                // redraw this line as well
    }
    else
    {
        x0 <<= 2; x <<= 2;                  // x-position in pixel
        _ASSERT(x >= x0);                   // can't draw negative number of pixel
        x -= x0;                            // number of pixels to update

        x0 -= Chipset.boffset;              // adjust x-position with left margin
        if (x0 < 0) x0 = 0;

        if (x0   > 131) x0 = 131;           // cut right borders
        if (x + x0 > 131) x = 131 - x0;

        y = y0 + 1;                         // draw one line
    }

	EnterCriticalSection(&csGDILock);	// solving NT GDI problems
	{
        StretchBlt(hWindowDC, nLcdX + x0*nLcdZoom*nGdiXZoom, nLcdY+y0*nLcdZoom*nGdiYZoom,
            x*nLcdZoom*nGdiXZoom, (y-y0)*nLcdZoom*nGdiYZoom,
            hLcdDC, x0 + Chipset.boffset, y0, x, y-y0, SRCCOPY);  // CdB for HP: add 64/80 line display for apples
		GdiFlush();
	}
	LeaveCriticalSection(&csGDILock);
	return;
}

VOID WriteToMenuDisplay(LPBYTE a, DWORD d, UINT s)
{
	UINT x0, x;
	UINT y0, y;
	DWORD *p;

	if (bGrayscale) return;					// no direct writing in grayscale mode

	#if defined DEBUG_DISPLAY
	{
		TCHAR buffer[256];
		wsprintf(buffer,_T("%.5lx: Write Menu Display %x,%u\n"),Chipset.pc,d,s);
		OutputDebugString(buffer);
	}
	#endif

	if (!(Chipset.IORam[BITOFFSET]&DON)) return;
	if (MENUHEIGHT == 0) return;				// menu disabled

	d -= Chipset.start2;
	y0 = y = (d / 34) + MAINSCREENHEIGHT+Chipset.d0size;         	// bitmap row
	x0 = x = d % 34;
	p = (DWORD*)(pbyLcd + y0*LCD_ROW + x0*sizeof(*p));

	// outside menu display area
//	_ASSERT(y0 >= (INT)(Chipset.d0size+MAINSCREENHEIGHT) && y0 < (INT)(SCREENHEIGHT));
	if (!(y0 >= (UINT)(Chipset.d0size+MAINSCREENHEIGHT) && y0 < (UINT)(SCREENHEIGHT))) return;

	while (s--)								// loop for nibbles to write
	{
		if (x<36)							// only fill visible area
		{
			*p = Pattern[*a];
		}
		a++;								// next value to write
		x++;								// next x position
		if ((x==34)&&s)					// end of display line
		{
			x = 0;							// first coloumn
			y++;							// next row
			if (y == SCREENHEIGHTREAL) break;
			// recalculate bitmap memory position of new line
			p=(DWORD*)(pbyLcd+y*LCD_ROW);  // CdB for HP: add 64/80 ligne display for apples
		} else p++;
	}
	if (y==y0) y++;
	EnterCriticalSection(&csGDILock);		// solving NT GDI problems
	{
		StretchBlt(hWindowDC, nLcdX, nLcdY+y0*nLcdZoom*nGdiYZoom, 
				   (131*nLcdZoom)*nGdiXZoom, (y-y0)*nLcdZoom*nGdiYZoom,
				   hLcdDC, 0, y0, 131, y-y0, SRCCOPY);  // CdB for HP: add 64/80 line display for apples
		GdiFlush();
	}
	LeaveCriticalSection(&csGDILock);
	return;
}

VOID UpdateAnnunciators(VOID)
{
	BYTE c;

	c = (BYTE)(Chipset.IORam[ANNCTRL] | (Chipset.IORam[ANNCTRL+1]<<4));
	// switch annunciators off if timer stopped
	if ((c & AON) == 0 || (Chipset.IORam[TIMER2_CTRL] & RUN) == 0)
		c = 0;

	DrawAnnunciator(1,c&LA1);
	DrawAnnunciator(2,c&LA2);
	DrawAnnunciator(3,c&LA3);
	DrawAnnunciator(4,c&LA4);
	DrawAnnunciator(5,c&LA5);
	DrawAnnunciator(6,c&LA6);
	return;
}

VOID ResizeWindow(VOID)
{
	if (hWnd != NULL)						// if window created
	{
		RECT rectWindow;
		RECT rectClient;

		rectWindow.left   = 0;
		rectWindow.top    = 0;
		rectWindow.right  = nBackgroundW;
		rectWindow.bottom = nBackgroundH;

		AdjustWindowRect(&rectWindow,
			(DWORD) GetWindowLongPtr(hWnd,GWL_STYLE),
			GetMenu(hWnd) != NULL || IsRectEmpty(&rectWindow));
		SetWindowPos(hWnd, bAlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0,
			rectWindow.right  - rectWindow.left,
			rectWindow.bottom - rectWindow.top,
			SWP_NOMOVE);

		// check if menu bar wrapped to two or more rows
		GetClientRect(hWnd, &rectClient);
		if (rectClient.bottom < (LONG) nBackgroundH)
		{
			rectWindow.bottom += (nBackgroundH - rectClient.bottom);
			SetWindowPos (hWnd, NULL, 0, 0,
				rectWindow.right  - rectWindow.left,
				rectWindow.bottom - rectWindow.top,
				SWP_NOMOVE | SWP_NOZORDER);
		}

		EnterCriticalSection(&csGDILock);	// solving NT GDI problems
		{
			_ASSERT(hWindowDC);				// move origin of destination window
			VERIFY(SetWindowOrgEx(hWindowDC, nBackgroundX, nBackgroundY, NULL));
			GdiFlush();
		}
		LeaveCriticalSection(&csGDILock);
		InvalidateRect(hWnd,NULL,TRUE);
	}
	return;
}

//################
//#
//# functions for gray scale implementation
//#
//################

// main display update routine
static VOID CALLBACK LcdProc(UINT uEventId, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	EnterCriticalSection(&csLcdLock);
	{
		UpdateMainDisplay();				// update display
		UpdateMenuDisplay();
		RefreshDisp0();  // CdB for HP: add header management
	}
	LeaveCriticalSection(&csLcdLock);

	QueryPerformanceCounter(&lLcdRef);		// actual time

	return;
	UNREFERENCED_PARAMETER(uEventId);
	UNREFERENCED_PARAMETER(uMsg);
	UNREFERENCED_PARAMETER(dwUser);
	UNREFERENCED_PARAMETER(dw1);
	UNREFERENCED_PARAMETER(dw2);
}

// LCD line counter calculation
static BYTE GetLineCounterGray(VOID)
{
	LARGE_INTEGER lLC;
	BYTE          byTime;

	if (uLcdTimerId == 0)					// display off
		return ((Chipset.IORam[LINECOUNT+1] & (LC5|LC4)) << 4) | Chipset.IORam[LINECOUNT];

	QueryPerformanceCounter(&lLC);			// get elapsed time since display update

	// elapsed ticks so far
	byTime = (BYTE) (((lLC.QuadPart - lLcdRef.QuadPart) << 12) / lFreq.QuadPart);

	if (byTime > 0x3F) byTime = 0x3F;		// all counts made

	return 0x3F - byTime;					// update display between VBL counter 0x3F-0x3E
}

static VOID StartDisplayGray(BYTE byInitial)
{
	if (uLcdTimerId)						// LCD update timer running
		return;								// -> quit

	if (Chipset.IORam[BITOFFSET]&DON)		// display on?
	{
		QueryPerformanceCounter(&lLcdRef);	// actual time of top line

		// adjust startup counter to get the right VBL value
		_ASSERT(byInitial <= 0x3F);			// line counter value 0 - 63
		lLcdRef.QuadPart -= ((LONGLONG) (0x3F - byInitial) * lFreq.QuadPart) >> 12;

		VERIFY(uLcdTimerId = timeSetEvent(DISPLAY_FREQ,0,(LPTIMECALLBACK)&LcdProc,0,TIME_PERIODIC));
	}
	return;
}

static VOID StopDisplayGray(VOID)
{
	BYTE a[2];
	ReadIO(a,LINECOUNT,2,TRUE);				// update VBL at display off time

	if (uLcdTimerId == 0)					// timer stopped
		return;								// -> quit

	timeKillEvent(uLcdTimerId);				// stop display update
	uLcdTimerId = 0;						// set flag display update stopped

	EnterCriticalSection(&csLcdLock);		// update to last condition
	{
		UpdateMainDisplay();				// update display
		UpdateMenuDisplay();
		RefreshDisp0();  // CdB for HP: add header management
	}
	LeaveCriticalSection(&csLcdLock);
	return;
}
//################
//#
//# functions for black and white implementation
//#
//################

// LCD line counter calculation in BW mode
static BYTE F4096Hz(VOID)					// get a 6 bit 4096Hz down counter value
{
	LARGE_INTEGER lLC;

	QueryPerformanceCounter(&lLC);			// get counter value

	// calculate 4096 Hz frequency down counter value
	return -(BYTE)(((lLC.QuadPart - lAppStart.QuadPart) << 12) / lFreq.QuadPart) & 0x3F;
}

static BYTE GetLineCounterBW(VOID)			// get line counter value
{
	_ASSERT(byVblRef < 0x40);
	return (0x40 + F4096Hz() - byVblRef) & 0x3F;
}

static VOID StartDisplayBW(BYTE byInitial)
{
	// get positive VBL difference between now and stop time
	byVblRef = (0x40 + F4096Hz() - byInitial) & 0x3F;
	return;
}

static VOID StopDisplayBW(VOID)
{
	BYTE a[2];
	ReadIO(a,LINECOUNT,2,TRUE);				// update VBL at display off time
	return;
}