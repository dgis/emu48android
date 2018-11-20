#include "pch.h"
#include "Emu48.h"

// Redeye.c
VOID IrPrinter(BYTE c) {
}

// Serial.c
BOOL CommOpen(LPTSTR strWirePort,LPTSTR strIrPort) {
    return 0;
}
VOID CommClose(VOID) {
}
VOID CommSetBaud(VOID) {
}
BOOL UpdateUSRQ(VOID) {
    return 0;
}
VOID CommTxBRK(VOID) {
}
VOID CommTransmit(VOID) {
}
VOID CommReceive(VOID) {
}

// Sound.c
DWORD dwWaveVol;
DWORD dwWaveTime;
VOID  SoundOut(CHIPSET* w, WORD wOut) {
}
VOID  SoundBeep(DWORD dwFrequency, DWORD dwDuration) {
}

// Emu48.c

#define VERSION   "1.59+"

//static BOOL bOwnCursor = FALSE;
//static BOOL bTitleBar = TRUE;
//
//CRITICAL_SECTION csGDILock;					// critical section for hWindowDC
//CRITICAL_SECTION csLcdLock;					// critical section for display update
//CRITICAL_SECTION csKeyLock;					// critical section for key scan
//CRITICAL_SECTION csIOLock;					// critical section for I/O access
//CRITICAL_SECTION csT1Lock;					// critical section for timer1 access
//CRITICAL_SECTION csT2Lock;					// critical section for timer2 access
//CRITICAL_SECTION csTxdLock;					// critical section for transmit byte
//CRITICAL_SECTION csRecvLock;				// critical section for receive byte
//CRITICAL_SECTION csSlowLock;				// critical section for speed slow down
//CRITICAL_SECTION csDbgLock;					// critical section for	debugger purpose
//INT              nArgc;						// no. of command line arguments
//LPCTSTR          *ppArgv;					// command line arguments
//LARGE_INTEGER    lFreq;						// high performance counter frequency
//LARGE_INTEGER    lAppStart;					// high performance counter value at Appl. start
//DWORD            idDdeInst;					// DDE server id
//UINT             uCF_HpObj;					// DDE clipboard format
//HANDLE           hThread;
//HANDLE           hEventShutdn;				// event handle to stop cpu thread
//
//HINSTANCE        hApp = NULL;
//HWND             hWnd = NULL;
//HWND             hDlgDebug = NULL;			// handle for debugger dialog
//HWND             hDlgFind = NULL;			// handle for debugger find dialog
//HWND             hDlgProfile = NULL;		// handle for debugger profile dialog
//HWND             hDlgRplObjView = NULL;		// handle for debugger rpl object viewer
//HDC              hWindowDC = NULL;
//HPALETTE         hPalette = NULL;
//HPALETTE         hOldPalette = NULL;		// old palette of hWindowDC
//DWORD            dwTColor = (DWORD) -1;		// transparency color
//DWORD            dwTColorTol = 0;			// transparency color tolerance
HRGN             hRgn = NULL;
//HCURSOR          hCursorArrow = NULL;
//HCURSOR          hCursorHand = NULL;
//UINT             uWaveDevId = WAVE_MAPPER;	// default audio device
//DWORD            dwWakeupDelay = 200;		// ON key hold time to switch on calculator
//BOOL             bAutoSave = FALSE;
//BOOL             bAutoSaveOnExit = TRUE;
//BOOL             bSaveDefConfirm = TRUE;	// yes
//BOOL             bStartupBackup = FALSE;
//BOOL             bAlwaysDisplayLog = TRUE;
//BOOL             bLoadObjectWarning = TRUE;
//BOOL             bShowTitle = TRUE;			// show main window title bar
//BOOL             bShowMenu = TRUE;			// show main window menu bar
//BOOL             bAlwaysOnTop = FALSE;		// emulator window always on top
//BOOL             bActFollowsMouse = FALSE;	// emulator window activation follows mouse
//BOOL             bClientWinMove = FALSE;	// emulator window can be moved over client area
//BOOL             bSingleInstance = FALSE;	// multiple emulator instances allowed

