/*
 *   Emu48Dll.c
 *
 *   This file is part of Emu48
 *
 *   Copyright (C) 2000 Christoph Gießelink
 *
 */

#include "pch.h"
#include "resource.h"
#include "Emu48.h"
#include "io.h"
#include "kml.h"
#include "debugger.h"

#include "Emu48Dll.h"

static LPCTSTR pArgv[3];					// command line memory
static ATOM    classAtom = INVALID_ATOM;	// window class atom
static HACCEL  hAccel;						// accelerator table

static HSZ hszService, hszTopic;			// variables for DDE server

extern LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);

// callback function notify document filename
VOID (CALLBACK *pEmuDocumentNotify)(LPCTSTR lpszFilename) = NULL;

// callback function notify Emu48 closed
static VOID (CALLBACK *pEmuClose)(VOID) = NULL;

//################
//#
//#    Public internal functions
//#
//################

//
// DllMain
//
BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	BOOL bSucc = TRUE;

	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		WNDCLASS wc;

		wc.style = CS_BYTEALIGNCLIENT;
		wc.lpfnWndProc = (WNDPROC)MainWndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hinstDLL;
		wc.hIcon = LoadIcon(hApp, MAKEINTRESOURCE(IDI_EMU48));
		wc.hCursor = NULL;
		wc.hbrBackground = NULL;
		wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU);
		wc.lpszClassName = _T("CEmu48");
		VERIFY(bSucc = ((classAtom = RegisterClass(&wc)) != INVALID_ATOM));
	}

	if (fdwReason == DLL_PROCESS_DETACH)
	{
		if (INVALID_ATOM != classAtom)
		{
			VERIFY(UnregisterClass(MAKEINTATOM(classAtom),hinstDLL));
			classAtom = INVALID_ATOM;
		}
	}

	return bSucc;
	UNREFERENCED_PARAMETER(lpvReserved);
}

//
// DLLCreateWnd
//
BOOL DLLCreateWnd(LPCTSTR lpszFilename, LPCTSTR lpszPort2Name)
{
	typedef DWORD (WINAPI *LPFN_STIP)(HANDLE hThread,DWORD dwIdealProcessor);

	RECT rectWindow;
	DWORD dwThreadId;
	LPFN_STIP fnSetThreadIdealProcessor;
	DWORD dwProcessor;

	BOOL bFileExist = FALSE;				// state file don't exist

	hApp = GetModuleHandle(_T("EMU48.DLL"));
	if (hApp == NULL) return TRUE;

	nArgc = 1;								// no argument
	if (lpszFilename[0])
	{
		// try to open given filename
		HANDLE hFile = CreateFile(lpszFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (bFileExist = (hFile != INVALID_HANDLE_VALUE))
			CloseHandle(hFile);

		ppArgv = pArgv;						// command line arguments
		nArgc = 2;							// one argument: state file, no port2 file
		pArgv[1] = lpszFilename;			// name of state file

		if (lpszPort2Name)					// port2 filename
		{
			nArgc = 3;						// two arguments: state file, port2 file
			pArgv[2] = lpszPort2Name;		// name of port2 file
		}
	}

	// read emulator settings
	GetCurrentDirectory(ARRAYSIZEOF(szCurrentDirectory),szCurrentDirectory);
	ReadSettings();

	// Create window
	rectWindow.left   = 0;
	rectWindow.top    = 0;
	rectWindow.right  = 256;
	rectWindow.bottom = 0;
	AdjustWindowRect(&rectWindow, WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_OVERLAPPED, TRUE);

	hWnd = CreateWindow(MAKEINTATOM(classAtom),_T("Emu48"),
		WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_OVERLAPPED,
		CW_USEDEFAULT, CW_USEDEFAULT,
		rectWindow.right  - rectWindow.left,
		rectWindow.bottom - rectWindow.top,
		NULL,NULL,hApp,NULL
		);

	if (hWnd == NULL)
	{
		return TRUE;
	}

	VERIFY(hAccel = LoadAccelerators(hApp,MAKEINTRESOURCE(IDR_MENU)));

	// remove debugger menu entry from resource
	DeleteMenu(GetMenu(hWnd),ID_TOOL_DEBUG,MF_BYCOMMAND);

	// initialization
	EmuClearAllBreakpoints();
	QueryPerformanceFrequency(&lFreq);		// init high resolution counter

	szCurrentKml[0] = 0;					// no KML file selected
	SetSpeed(bRealSpeed);					// set speed
	MruInit(0);								// init MRU entries

	// create shutdown auto event handle
	hEventShutdn = CreateEvent(NULL,FALSE,FALSE,NULL);
	if (hEventShutdn == NULL)
	{
		DestroyWindow(hWnd);
		return TRUE;
	}

	// create debugger auto event handle
	hEventDebug = CreateEvent(NULL,FALSE,FALSE,NULL);
	if (hEventDebug == NULL)
	{
		CloseHandle(hEventShutdn);			// close shutdown event handle
		DestroyWindow(hWnd);
		return TRUE;
	}

	nState     = SM_RUN;					// init state must be <> nNextState
	nNextState = SM_INVALID;				// go into invalid state
	hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&WorkerThread, NULL, CREATE_SUSPENDED, &dwThreadId);
	if (hThread == NULL)
	{
		CloseHandle(hEventDebug);			// close debugger event handle
		CloseHandle(hEventShutdn);			// close event handle
		DestroyWindow(hWnd);
		return TRUE;
	}

	// SetThreadIdealProcessor() is available since Windows NT4.0
	fnSetThreadIdealProcessor = (LPFN_STIP) GetProcAddress(GetModuleHandle(_T("kernel32")),
														   "SetThreadIdealProcessor");

	// bind Saturn CPU emulation thread to current ideal processor
	dwProcessor = (fnSetThreadIdealProcessor != NULL)					// running on NT4.0 or later
				? fnSetThreadIdealProcessor(hThread,MAXIMUM_PROCESSORS)	// get ideal processor no.
				: 0;													// select 1st processor

	// on multiprocessor machines for QueryPerformanceCounter()
	VERIFY(SetThreadAffinityMask(hThread,(DWORD_PTR) (1 << dwProcessor)));
	ResumeThread(hThread);					// start thread
	while (nState!=nNextState) Sleep(0);	// wait for thread initialized

	idDdeInst = 0;							// initialize DDE server
	if (DdeInitialize(&idDdeInst,(PFNCALLBACK) &DdeCallback,
					  APPCLASS_STANDARD |
					  CBF_FAIL_EXECUTES | CBF_FAIL_ADVISES |
					  CBF_SKIP_REGISTRATIONS | CBF_SKIP_UNREGISTRATIONS,0))
	{
		TerminateThread(hThread, 0);		// kill emulation thread
		CloseHandle(hEventDebug);			// close debugger event handle
		CloseHandle(hEventShutdn);			// close event handle
		DestroyWindow(hWnd);
		return TRUE;
	}

	// init clipboard format and name service
	uCF_HpObj = RegisterClipboardFormat(_T(CF_HPOBJ));
	hszService = DdeCreateStringHandle(idDdeInst,szAppName,0);
	hszTopic   = DdeCreateStringHandle(idDdeInst,szTopic,0);
	DdeNameService(idDdeInst,hszService,NULL,DNS_REGISTER);

	SoundOpen(uWaveDevId);					// open waveform-audio output device

	_ASSERT(hWnd != NULL);
	_ASSERT(hWindowDC != NULL);

	szBufferFilename[0] = 0;
	if (bFileExist)							// open existing file
	{
		lstrcpyn(szBufferFilename,ppArgv[1],ARRAYSIZEOF(szBufferFilename));
	}
	if (nArgc == 1)							// no argument
	{
		ReadLastDocument(szBufferFilename,ARRAYSIZEOF(szBufferFilename));
	}

	if (szBufferFilename[0])				// given default document
	{
		TCHAR szTemp[MAX_PATH+8] = _T("Loading ");
		RECT  rectClient;

		_ASSERT(hWnd != NULL);
		VERIFY(GetClientRect(hWnd,&rectClient));
		GetCutPathName(szBufferFilename,&szTemp[8],MAX_PATH,rectClient.right/11);
		SetWindowTitle(szTemp);
		if (OpenDocument(szBufferFilename))
		{
			MruAdd(szCurrentFilename);
			ShowWindow(hWnd,SW_SHOWNORMAL);
			goto start;
		}
	}

	SetWindowTitle(_T("New Document"));
	ShowWindow(hWnd,SW_SHOWNORMAL);

	if (NewDocument())
	{
		if (nArgc >= 2)
			SaveDocumentAs(ppArgv[1]);
		else
			SetWindowTitle(_T("Untitled"));
		goto start;
	}

	DestroyWindow(hWnd);					// clean up system
	return TRUE;

start:
	if (bStartupBackup) SaveBackup();		// make a RAM backup at startup
	if (pbyRom) SwitchToState(SM_RUN);
	return FALSE;
}

//
// DLLDestroyWnd
//
BOOL DLLDestroyWnd(VOID)
{
	LPTSTR lpFilePart;

	// clean up DDE server
	DdeNameService(idDdeInst, hszService, NULL, DNS_UNREGISTER);
	DdeFreeStringHandle(idDdeInst, hszService);
	DdeFreeStringHandle(idDdeInst, hszTopic);
	DdeUninitialize(idDdeInst);

	SoundClose();							// close waveform-audio output device

	// get full path name of szCurrentFilename
	if (GetFullPathName(szCurrentFilename,ARRAYSIZEOF(szBufferFilename),szBufferFilename,&lpFilePart) == 0)
		szBufferFilename[0] = 0;			// no last document name

	WriteLastDocument(szBufferFilename);	// save last document setting
	WriteSettings();						// save variable settings

	CloseHandle(hThread);					// close emulation thread handle
	CloseHandle(hEventShutdn);				// close shutdown event handle
	CloseHandle(hEventDebug);				// close debugger event handle
	_ASSERT(nState == SM_RETURN);			// emulation thread down?
	ResetDocument();
	ResetBackup();
	MruCleanup();
	_ASSERT(pbyRom == NULL);				// rom file unmapped
	_ASSERT(pbyPort2 == NULL);				// port2 file unmapped
	_ASSERT(pKml == NULL);					// KML script not closed
	_ASSERT(szTitle == NULL);				// freed allocated memory
	_ASSERT(hPalette == NULL);				// freed resource memory
	if (pEmuClose) pEmuClose();				// call notify function
	return FALSE;
}


//################
//#
//#    Public external functions
//#
//################

/****************************************************************************
* EmuCreate
*****************************************************************************
*
* @func   start Emu48 and load Ram file into emulator, if Ram file don't
*         exist create a new one and save it under the given name
*
* @xref   none
*
* @rdesc  BOOL: FALSE = OK, TRUE = Error
*
****************************************************************************/

DECLSPEC BOOL CALLBACK EmuCreate(
	LPCTSTR lpszFilename)					// @parm String with RAM filename
{
	return DLLCreateWnd(lpszFilename, NULL);
}

/****************************************************************************
* EmuCreateEx
*****************************************************************************
*
* @func   start Emu48 and load Ram and Port2 file into emulator, if Ram file
*         don't exist create a new one and save it under the given name
*
* @xref   none
*
* @rdesc  BOOL: FALSE = OK, TRUE = Error
*
****************************************************************************/

DECLSPEC BOOL CALLBACK EmuCreateEx(
	LPCTSTR lpszFilename,					// @parm String with RAM filename
	LPCTSTR lpszPort2Name)					// @parm String with Port2 filename
											//       or NULL for using name inside INI file
{
	return DLLCreateWnd(lpszFilename, lpszPort2Name);
}

/****************************************************************************
* EmuDestroy
*****************************************************************************
*
* @func   close Emu48, free all memory
*
* @xref   none
*
* @rdesc  BOOL: FALSE = OK, TRUE = Error
*
****************************************************************************/

DECLSPEC BOOL CALLBACK EmuDestroy(VOID)
{
	if (hWnd == NULL) return TRUE;			// Emu48 closed

	// close Emu48 via exit
	SendMessage(hWnd,WM_SYSCOMMAND,SC_CLOSE,0);
	return FALSE;
}

/****************************************************************************
* EmuAcceleratorTable
*****************************************************************************
*
* @func   load accelerator table of emulator
*
* @xref   none
*
* @rdesc  HACCEL: handle of the loaded accelerator table
*
****************************************************************************/

DECLSPEC HACCEL CALLBACK EmuAcceleratorTable(
	HWND *phEmuWnd)							// @parm return of emulator window handle
{
	*phEmuWnd = hWnd;
	return hAccel;
}

/****************************************************************************
* EmuCallBackClose
*****************************************************************************
*
* @func   init CallBack handler to notify caller when Emu48 window close
*
* @xref   none
*
* @rdesc  VOID
*
****************************************************************************/

DECLSPEC VOID CALLBACK EmuCallBackClose(
	VOID (CALLBACK *EmuClose)(VOID))		// @parm CallBack function notify caller Emu48 closed
{
	pEmuClose = EmuClose;					// set new handler
	return;
}

/****************************************************************************
* EmuCallBackDocumentNotify
*****************************************************************************
*
* @func   init CallBack handler to notify caller for actual document file
*
* @xref   none
*
* @rdesc  VOID
*
****************************************************************************/

DECLSPEC VOID CALLBACK EmuCallBackDocumentNotify(
	VOID (CALLBACK *EmuDocumentNotify)(LPCTSTR lpszFilename)) // @parm CallBack function notify document filename
{
	pEmuDocumentNotify = EmuDocumentNotify;	// set new handler
	return;
}

/****************************************************************************
* EmuLoadRamFile
*****************************************************************************
*
* @func   load Ram file into emulator
*
* @xref   none
*
* @rdesc  BOOL: FALSE = OK, TRUE = Error (old file reloaded)
*
****************************************************************************/

DECLSPEC BOOL CALLBACK EmuLoadRamFile(
	LPCTSTR lpszFilename)					// @parm String with RAM filename
{
	BOOL bErr;

	if (pbyRom) SwitchToState(SM_INVALID);	// stop emulation thread
	bErr = !OpenDocument(lpszFilename);		// load state file
	if (pbyRom) SwitchToState(SM_RUN);		// restart emulation thread
	return bErr;
}

/****************************************************************************
* EmuSaveRamFile
*****************************************************************************
*
* @func   save the current emulator Ram to file
*
* @xref   none
*
* @rdesc  BOOL: FALSE = OK, TRUE = Error (old file reloaded)
*
****************************************************************************/

DECLSPEC BOOL CALLBACK EmuSaveRamFile(VOID)
{
	BOOL bErr;

	if (pbyRom == NULL) return TRUE;		// fail
	SwitchToState(SM_INVALID);				// stop emulation thread
	bErr = !SaveDocument();					// save current state file
	SwitchToState(SM_RUN);					// restart emulation thread
	return bErr;
}

/****************************************************************************
* EmuLoadObject
*****************************************************************************
*
* @func   load object file to stack
*
* @xref   none
*
* @rdesc  BOOL: FALSE = OK, TRUE = Error
*
****************************************************************************/

DECLSPEC BOOL CALLBACK EmuLoadObject(
	LPCTSTR lpszObjectFilename)				// @parm String with object filename
{
	HANDLE hFile;
	DWORD  dwFileSizeLow;
	DWORD  dwFileSizeHigh;
	LPBYTE lpBuf;
	WORD   wError = S_ERR_BINARY;			// set into error state

	SuspendDebugger();						// suspend debugger
	bDbgAutoStateCtrl = FALSE;				// disable automatic debugger state control

	if (!(Chipset.IORam[BITOFFSET]&DON))	// calculator off, turn on
	{
		KeyboardEvent(TRUE,0,0x8000);
		Sleep(dwWakeupDelay);
		KeyboardEvent(FALSE,0,0x8000);
		Sleep(dwWakeupDelay);
		// wait for sleep mode
		while(Chipset.Shutdn == FALSE) Sleep(0);
	}

	if (nState != SM_RUN) goto cancel;		// emulator must run to load on object
	if (WaitForSleepState()) goto cancel;	// wait for cpu SHUTDN then sleep state

	_ASSERT(nState == SM_SLEEP);

	hFile = CreateFile(lpszObjectFilename,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		SwitchToState(SM_RUN);				// run state
		goto cancel;
	}
	dwFileSizeLow = GetFileSize(hFile, &dwFileSizeHigh);
	if (dwFileSizeHigh != 0)
	{ // file is too large.
		SwitchToState(SM_RUN);				// run state
		CloseHandle(hFile);
		goto cancel;
	}
	lpBuf = malloc(dwFileSizeLow*2);
	if (lpBuf == NULL)
	{
		SwitchToState(SM_RUN);				// run state
		CloseHandle(hFile);
		goto cancel;
	}
	ReadFile(hFile, lpBuf+dwFileSizeLow, dwFileSizeLow, &dwFileSizeHigh, NULL);
	CloseHandle(hFile);

	wError = WriteStack(1,lpBuf,dwFileSizeLow);

	free(lpBuf);

	SwitchToState(SM_RUN);					// run state
	while (nState!=nNextState) Sleep(0);
	_ASSERT(nState == SM_RUN);
	KeyboardEvent(TRUE,0,0x8000);
	Sleep(dwWakeupDelay);
	KeyboardEvent(FALSE,0,0x8000);
	while(Chipset.Shutdn == FALSE) Sleep(0);

cancel:
	bDbgAutoStateCtrl = TRUE;				// enable automatic debugger state control
	ResumeDebugger();
	return wError != S_ERR_NO;
}

/****************************************************************************
* EmuSaveObject
*****************************************************************************
*
* @func   save object on stack to file
*
* @xref   none
*
* @rdesc  BOOL: FALSE = OK, TRUE = Error
*
****************************************************************************/

DECLSPEC BOOL CALLBACK EmuSaveObject(
	LPCTSTR lpszObjectFilename)				// @parm String with object filename
{
	BOOL bErr;

	if (nState != SM_RUN) return TRUE;		// emulator must run to load on object
	if (WaitForSleepState()) return TRUE;	// wait for cpu SHUTDN then sleep state
	_ASSERT(nState == SM_SLEEP);
	bErr = !SaveObject(lpszObjectFilename);
	SwitchToState(SM_RUN);
	return bErr;
}

/****************************************************************************
* EmuCalculatorType
*****************************************************************************
*
* @func   get ID of current calculator type
*
* @xref   none
*
* @rdesc  BYTE: '6' = HP38G with 64KB RAM
*               'A' = HP38G
*               'E' = HP39/40G
*               'S' = HP48SX
*               'G' = HP48GX
*               'X' = HP49G
*               'P' = HP39G+
*               '2' = HP48GII
*               'Q' = HP49G+
*
****************************************************************************/

DECLSPEC BYTE CALLBACK EmuCalculatorType(VOID)
{
	return Chipset.type;
}

/****************************************************************************
* EmuSimulateKey
*****************************************************************************
*
* @func   simulate a key action
*
* @xref   none
*
* @rdesc  VOID
*
****************************************************************************/

DECLSPEC VOID CALLBACK EmuSimulateKey(
	BOOL bKeyState,							// @parm TRUE = pressed, FALSE = released
	UINT out,								// @parm key out line
	UINT in)								// @parm key in  line
{
	KeyboardEvent(bKeyState,out,in);
}

/****************************************************************************
* EmuPressOn
*****************************************************************************
*
* @func   press On key (emulation must run)
*
* @xref   none
*
* @rdesc  VOID
*
****************************************************************************/

DECLSPEC VOID CALLBACK EmuPressOn(
	BOOL bKeyState)							// @parm TRUE = pressed, FALSE = released
{
	KeyboardEvent(bKeyState,0,0x8000);
}