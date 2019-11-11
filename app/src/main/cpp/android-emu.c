// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#include "core/pch.h"
#include "core/resource.h"
#include "emu.h"
#include "core/io.h"
#include "core/kml.h"
#include "core/debugger.h"
#include "win32-layer.h"

LPTSTR szTitle   = NULL;

CRITICAL_SECTION csGDILock;					// critical section for hWindowDC
CRITICAL_SECTION csLcdLock;					// critical section for display update
CRITICAL_SECTION csKeyLock;					// critical section for key scan
CRITICAL_SECTION csIOLock;					// critical section for I/O access
CRITICAL_SECTION csT1Lock;					// critical section for timer1 access
CRITICAL_SECTION csT2Lock;					// critical section for timer2 access
CRITICAL_SECTION csTxdLock;					// critical section for transmit byte
CRITICAL_SECTION csRecvLock;				// critical section for receive byte
CRITICAL_SECTION csSlowLock;				// critical section for speed slow down
CRITICAL_SECTION csDbgLock;					// critical section for	debugger purpose
INT              nArgc;						// no. of command line arguments
LPCTSTR          *ppArgv;					// command line arguments
LARGE_INTEGER    lFreq;						// high performance counter frequency
LARGE_INTEGER    lAppStart;					// high performance counter value at Appl. start
HANDLE           hThread;
HANDLE           hEventShutdn;				// event handle to stop cpu thread

HINSTANCE        hApp = NULL;
HWND             hWnd = NULL;
HWND             hDlgDebug = NULL;			// handle for debugger dialog
HWND             hDlgFind = NULL;			// handle for debugger find dialog
HWND             hDlgProfile = NULL;		// handle for debugger profile dialog
HWND             hDlgRplObjView = NULL;		// handle for debugger rpl object viewer
HDC              hWindowDC = NULL;
HPALETTE         hPalette = NULL;
HPALETTE         hOldPalette = NULL;		// old palette of hWindowDC
DWORD            dwTColor = (DWORD) -1;		// transparency color
DWORD            dwTColorTol = 0;			// transparency color tolerance
HRGN             hRgn = NULL;
HCURSOR          hCursorArrow = NULL;
HCURSOR          hCursorHand = NULL;
UINT             uWaveDevId = WAVE_MAPPER;	// default audio device
DWORD            dwWakeupDelay = 200;		// ON key hold time to switch on calculator
BOOL             bAutoSave = FALSE;
BOOL             bAutoSaveOnExit = TRUE;
BOOL             bSaveDefConfirm = TRUE;	// yes
BOOL             bStartupBackup = FALSE;
BOOL             bAlwaysDisplayLog = TRUE;
BOOL             bLoadObjectWarning = TRUE;
BOOL             bShowTitle = TRUE;			// show main window title bar
BOOL             bShowMenu = TRUE;			// show main window menu bar
BOOL             bAlwaysOnTop = FALSE;		// emulator window always on top
BOOL             bActFollowsMouse = FALSE;	// emulator window activation follows mouse
BOOL             bClientWinMove = FALSE;	// emulator window can be moved over client area
BOOL             bSingleInstance = FALSE;	// multiple emulator instances allowed


//################
//#
//#    Window Status
//#
//################

VOID SetWindowTitle(LPCTSTR szString)
{
	return;
}

VOID ForceForegroundWindow(HWND hWnd)
{
	return;
}

//################
//#
//#    Clipboard Tool
//#
//################

VOID CopyItemsToClipboard(HWND hWnd)		// save selected Listbox Items to Clipboard
{
	return;
}

#define WIDTHBYTES(bits) ((((bits) + 31) / 32) * 4)

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

extern JavaVM *java_machine;
extern jobject bitmapMainScreen;
extern AndroidBitmapInfo androidBitmapInfo;

static void MakeBitmapTransparent(HBITMAP hBmp,COLORREF color,DWORD dwTol) {

	BOOL (*fnColorCmp)(DWORD dwColor1,DWORD dwColor2,DWORD dwTol);
	if (dwTol >= 1000) {					// use CIE L*a*b compare
		fnColorCmp = LabColorCmp;
		dwTol -= 1000;						// remove L*a*b compare selector
	} else {								// use Abs summation compare
		fnColorCmp = AbsColorCmp;
	}

	JNIEnv * jniEnv = NULL;
	jint ret;

	BOOL needDetach = FALSE;
	ret = (*java_machine)->GetEnv(java_machine, (void **) &jniEnv, JNI_VERSION_1_6);
	if (ret == JNI_EDETACHED) {
		// GetEnv: not attached
		ret = (*java_machine)->AttachCurrentThread(java_machine, &jniEnv, NULL);
		if (ret == JNI_OK) {
			needDetach = TRUE;
		}
	}

	int destinationWidth = androidBitmapInfo.width;
	int destinationHeight = androidBitmapInfo.height;
	int destinationBitCount = 32;
	int destinationStride = androidBitmapInfo.stride;
	void * pixelsDestination = NULL;
	if ((ret = AndroidBitmap_lockPixels(jniEnv, bitmapMainScreen, &pixelsDestination)) < 0) {
		LOGD("AndroidBitmap_lockPixels() failed ! error=%d", ret);
		return;
	}

	LPBYTE pbyBits = pixelsDestination;

	// convert COLORREF to RGBQUAD color
	DWORD dwRed   = 0x00FF0000;
	DWORD dwGreen = 0x0000FF00;
	DWORD dwBlue  = 0x000000FF;

	color = EncodeColorBits((color >> 16), dwBlue)
			| EncodeColorBits((color >>  8), dwGreen)
			| EncodeColorBits((color >>  0), dwRed);

	DWORD dwBpp = (DWORD) (destinationBitCount >> 3);
	LPBYTE pbyColor = pbyBits + (destinationHeight - 1) * destinationStride;

	for (LONG y = 0; y < destinationHeight; ++y) {
		LPBYTE pbyLineStart = pbyColor;
		for (LONG x = 0; x < destinationWidth; ++x) {
			if(!fnColorCmp(*(LPDWORD)pbyColor,color,dwTol))
				*(LPDWORD)pbyColor = 0x00000000;
			pbyColor += dwBpp;
		}
		pbyColor = pbyLineStart - destinationStride;
	}

	if(jniEnv && (ret = AndroidBitmap_unlockPixels(jniEnv, bitmapMainScreen)) < 0)
		LOGD("AndroidBitmap_unlockPixels() failed ! error=%d", ret);
}

//
// WM_PAINT
//
static LRESULT OnPaint(HWND hWindow)
{
	PAINTSTRUCT Paint;
	HDC hPaintDC;

	PAINT_LOGD("PAINT OnPaint()");

	//UpdateWindowBars();						// update visibility of title and menu bar

	hPaintDC = BeginPaint(hWindow, &Paint);
	if (hMainDC != NULL)
	{
		RECT rcMainPaint = Paint.rcPaint;
		rcMainPaint.left   += nBackgroundX;	// coordinates in source bitmap
		rcMainPaint.top    += nBackgroundY;
		rcMainPaint.right  += nBackgroundX;
		rcMainPaint.bottom += nBackgroundY;

		EnterCriticalSection(&csGDILock);	// solving NT GDI problems
		{
			UINT nLines = MAINSCREENHEIGHT;

			// redraw background bitmap
			PAINT_LOGD("PAINT OnPaint() BitBlt()");
			BitBlt(hPaintDC, Paint.rcPaint.left, Paint.rcPaint.top,
				   Paint.rcPaint.right-Paint.rcPaint.left, Paint.rcPaint.bottom-Paint.rcPaint.top,
				   hMainDC, rcMainPaint.left, rcMainPaint.top, SRCCOPY);

			if (dwTColor != (DWORD) -1) // prepare background bitmap with transparency
				MakeBitmapTransparent((HBITMAP)GetCurrentObject(hPaintDC, OBJ_BITMAP), dwTColor, dwTColorTol);

			// CdB for HP: add apples display stuff
			SetWindowOrgEx(hPaintDC, nBackgroundX, nBackgroundY, NULL);

			// redraw header display area
			StretchBlt(hPaintDC, nLcdX, nLcdY,
					   131*nLcdZoom*nGdiXZoom, Chipset.d0size*nLcdZoom*nGdiYZoom,
					   hLcdDC, Chipset.d0offset, 0,
					   131, Chipset.d0size, SRCCOPY);
			// redraw main display area
			StretchBlt(hPaintDC, nLcdX, nLcdY+Chipset.d0size*nLcdZoom*nGdiYZoom,
					   131*nLcdZoom*nGdiXZoom, nLines*nLcdZoom*nGdiYZoom,
					   hLcdDC, Chipset.boffset, Chipset.d0size,
					   131, nLines, SRCCOPY);
			// redraw menu display area
			StretchBlt(hPaintDC, nLcdX, nLcdY+(nLines+Chipset.d0size)*nLcdZoom*nGdiYZoom,
					   131*nLcdZoom*nGdiXZoom, MENUHEIGHT*nLcdZoom*nGdiYZoom,
					   hLcdDC, 0, (nLines+Chipset.d0size),
					   131, MENUHEIGHT, SRCCOPY);
			GdiFlush();
		}
		LeaveCriticalSection(&csGDILock);
		UpdateAnnunciators();
		RefreshButtons(&rcMainPaint);
	}
	EndPaint(hWindow, &Paint);
	return 0;
}

static LRESULT OnLButtonDown(UINT nFlags, WORD x, WORD y)
{
	if (nMacroState == MACRO_PLAY) return 0; // playing macro
	if (nState == SM_RUN) {
		MouseButtonDownAt(nFlags, x,y);
		if(MouseIsButton(x,y)) {
            performHapticFeedback();
			return 1;
		}
	}
	return 0;
}

static LRESULT OnLButtonUp(UINT nFlags, WORD x, WORD y)
{
	if (nMacroState == MACRO_PLAY) return 0; // playing macro
	if (nState == SM_RUN) MouseButtonUpAt(nFlags, x,y);
	return 0;
}

static LRESULT OnKeyDown(int nVirtKey, LPARAM lKeyData)
{
	if (nMacroState == MACRO_PLAY) return 0; // playing macro
	// call RunKey() only once (suppress autorepeat feature)
	if (nState == SM_RUN && (lKeyData & 0x40000000) == 0)
		RunKey((BYTE)nVirtKey, TRUE);
	return 0;
}

static LRESULT OnKeyUp(int nVirtKey, LPARAM lKeyData)
{
	if (nMacroState == MACRO_PLAY) return 0; // playing macro
	if (nState == SM_RUN) RunKey((BYTE)nVirtKey, FALSE);
	return 0;
	UNREFERENCED_PARAMETER(lKeyData);
}

void draw() {
    OnPaint(NULL);
}

BOOL buttonDown(int x, int y) {
    return OnLButtonDown(MK_LBUTTON, x, y);
}

void buttonUp(int x, int y) {
    OnLButtonUp(MK_LBUTTON, x, y);
}

void keyDown(int virtKey) {
    OnKeyDown(virtKey, 0);
}
void keyUp(int virtKey) {
    OnKeyUp(virtKey, 0);
}
