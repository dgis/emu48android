#include "core/pch.h"
#include "core/resource.h"
#include "core/Emu48.h"
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

//
// WM_PAINT
//
static LRESULT OnPaint(HWND hWindow)
{
	PAINTSTRUCT Paint;
	HDC hPaintDC;

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
			BitBlt(hPaintDC, Paint.rcPaint.left, Paint.rcPaint.top,
				   Paint.rcPaint.right-Paint.rcPaint.left, Paint.rcPaint.bottom-Paint.rcPaint.top,
				   hMainDC, rcMainPaint.left, rcMainPaint.top, SRCCOPY);

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
	if (nState == SM_RUN) MouseButtonDownAt(nFlags, x,y);
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

void buttonDown(int x, int y) {
    OnLButtonDown(MK_LBUTTON, x, y);
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
