/*
 *   Keymacro.c
 *
 *   This file is part of Emu48
 *
 *   Copyright (C) 2004 Christoph Gießelink
 *
 */
#include "pch.h"
#include "resource.h"
#include "Emu48.h"
#include "kml.h"

#define KEYMACROHEAD "Emu-KeyMacro"			// macro signature

#define MIN_SPEED    0
#define MAX_SPEED    500

typedef struct
{
	DWORD dwTime;							// elapsed time
	DWORD dwKeyEvent;						// key code
} KeyData;

INT   nMacroState = MACRO_OFF;
INT   nMacroTimeout = MIN_SPEED;
BOOL  bMacroRealSpeed = TRUE;
DWORD dwMacroMinDelay = 0;					// minimum macro play key hold time in ms

static DWORD  dwTimeRef;

static HANDLE hMacroFile = INVALID_HANDLE_VALUE;
static HANDLE hEventPlay = NULL;
static HANDLE hThreadEv = NULL;

static VOID InitializeOFN(LPOPENFILENAME ofn)
{
	ZeroMemory((LPVOID)ofn, sizeof(OPENFILENAME));
	ofn->lStructSize = sizeof(OPENFILENAME);
	ofn->hwndOwner = hWnd;
	ofn->Flags = OFN_EXPLORER|OFN_HIDEREADONLY;
	return;
}

//
// thread playing keys
//
static DWORD WINAPI EventThread(LPVOID pParam)
{
	DWORD dwRead = 0;
	DWORD dwData = 0,dwTime = 0;

	while (WaitForSingleObject(hEventPlay,dwTime) == WAIT_TIMEOUT)
	{
		if (dwRead != 0)					// data read
		{
			UINT nIn    = (dwData >> 0)  & 0xFFFF;
			UINT nOut   = (dwData >> 16) & 0xFF;
			BOOL bPress = (dwData >> 24) & 0xFF;

			PlayKey(nOut,nIn,bPress);
		}

		dwTime = nMacroTimeout;				// set default speed

		while (TRUE)
		{
			// read next data element
			if (   !ReadFile(hMacroFile,&dwData,sizeof(dwData),&dwRead,NULL)
				|| dwRead != sizeof(dwData))
			{
				// play record empty -> quit
				PostMessage(hWnd,WM_COMMAND,ID_TOOL_MACRO_STOP,0);
				return 0;					// exit on file end
			}

			if ((dwData & 0x80000000) != 0)	// time information
			{
				if (bMacroRealSpeed)		// realspeed from data
				{
					dwTime = dwData & 0x7FFFFFFF;
				}
				continue;
			}

			// hold the key state the minimum macro play key hold time 
			if (dwTime < dwMacroMinDelay) dwTime = dwMacroMinDelay;

			dwTime -= dwKeyMinDelay;		// remove the actual key hold time
			// set negative number to zero
			if ((dwTime & 0x80000000) != 0) dwTime = 0;
			break;							// got key information
		}
	}
	return 0;								// exit on stop
	UNREFERENCED_PARAMETER(pParam);
}

//
// callback function for recording keys
//
VOID KeyMacroRecord(BOOL bPress, UINT out, UINT in)
{
	if (nMacroState == MACRO_NEW)			// save key event
	{
		KeyData Data;
		DWORD   dwWritten;

		dwWritten = GetTickCount();			// time reference
		Data.dwTime = (dwWritten - dwTimeRef);
		Data.dwTime |= 0x80000000;			// set time marker
		dwTimeRef = dwWritten;

		Data.dwKeyEvent = (bPress & 0xFF);
		Data.dwKeyEvent = (Data.dwKeyEvent << 8)  | (out & 0xFF);
		Data.dwKeyEvent = (Data.dwKeyEvent << 16) | (in & 0xFFFF);

		// save key event in file
		WriteFile(hMacroFile,&Data,sizeof(Data),&dwWritten,NULL);
		_ASSERT(dwWritten == sizeof(Data));
	}
	return;
}

//
// message handler for save new keyboard macro
//
LRESULT OnToolMacroNew(VOID)
{
	TCHAR szMacroFile[MAX_PATH];
	OPENFILENAME ofn;
	DWORD dwExtensionLength,dwWritten;

	// get filename for saving
	InitializeOFN(&ofn);
	ofn.lpstrFilter =
		_T("Keyboard Macro Files (*.MAC)\0*.MAC\0")
		_T("All Files (*.*)\0*.*\0");
	ofn.lpstrDefExt = _T("MAC");
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = szMacroFile;
	ofn.lpstrFile[0] = 0;
	ofn.nMaxFile = ARRAYSIZEOF(szMacroFile);
	ofn.Flags |= OFN_CREATEPROMPT|OFN_OVERWRITEPROMPT;
	if (GetSaveFileName(&ofn) == FALSE) return 0;

	// open file for writing
	hMacroFile = CreateFile(szMacroFile,
							GENERIC_READ|GENERIC_WRITE,
							0,
							NULL,
							CREATE_ALWAYS,
							FILE_ATTRIBUTE_NORMAL,
							NULL);
	if (hMacroFile == INVALID_HANDLE_VALUE) return 0;

	// write header
	WriteFile(hMacroFile,KEYMACROHEAD,sizeof(KEYMACROHEAD) - 1,&dwWritten,NULL);
	_ASSERT(dwWritten == (DWORD) strlen(KEYMACROHEAD));

	// write extension length
	dwExtensionLength = 0;					// no extension
	WriteFile(hMacroFile,&dwExtensionLength,sizeof(dwExtensionLength),&dwWritten,NULL);
	_ASSERT(dwWritten == sizeof(dwExtensionLength));

	nMacroState = MACRO_NEW;

	MessageBox(hWnd,
			   _T("Press OK to begin to record the Macro."),
			   _T("Macro Recorder"),
			   MB_OK|MB_ICONINFORMATION);

	dwTimeRef = GetTickCount();				// time reference
	return 0;
}

//
// message handler for play keyboard macro
//
LRESULT OnToolMacroPlay(VOID)
{
	BYTE  byHeader[sizeof(KEYMACROHEAD)-1];
	TCHAR szMacroFile[MAX_PATH];
	OPENFILENAME ofn;
	DWORD dwExtensionLength,dwRead,dwThreadId;

	InitializeOFN(&ofn);
	ofn.lpstrFilter =
		_T("Keyboard Macro Files (*.MAC)\0*.MAC\0")
		_T("All Files (*.*)\0*.*\0");
	ofn.lpstrDefExt = _T("MAC");
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = szMacroFile;
	ofn.lpstrFile[0] = 0;
	ofn.nMaxFile = ARRAYSIZEOF(szMacroFile);
	ofn.Flags |= OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
	if (GetOpenFileName(&ofn) == FALSE) return 0;

	// open file for Reading
	hMacroFile = CreateFile(szMacroFile,
							GENERIC_READ,
							FILE_SHARE_READ,
							NULL,
							OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL,
							NULL);
	if (hMacroFile == INVALID_HANDLE_VALUE) return 0;

	// read header
	ReadFile(hMacroFile,byHeader,sizeof(byHeader),&dwRead,NULL);
	if (   dwRead != sizeof(byHeader)
		|| memcmp(byHeader,KEYMACROHEAD,dwRead) != 0)
	{
		MessageBox(hWnd,
				   _T("Wrong keyboard macro file format."),
				   _T("Macro Recorder"),
				   MB_OK|MB_ICONSTOP);
		CloseHandle(hMacroFile);
		return 0;
	}

	// read extension length
	ReadFile(hMacroFile,&dwExtensionLength,sizeof(dwExtensionLength),&dwRead,NULL);
	if (dwRead != sizeof(dwExtensionLength))
	{
		CloseHandle(hMacroFile);
		return 0;
	}

	// read extension
	while (dwExtensionLength-- > 0)
	{
		BYTE byData;

		ReadFile(hMacroFile,&byData,sizeof(byData),&dwRead,NULL);
		if (dwRead != sizeof(byData))
		{
			CloseHandle(hMacroFile);
			return 0;
		}
	}

	// event for quit playing
	hEventPlay = CreateEvent(NULL,FALSE,FALSE,NULL);

	nMacroState = MACRO_PLAY;

	// start playing thread
	VERIFY(hThreadEv = CreateThread(NULL,0,&EventThread,NULL,0,&dwThreadId));
	return 0;
}

//
// message handler for stop recording/playing
//
LRESULT OnToolMacroStop(VOID)
{
	if (nMacroState != MACRO_OFF)
	{
		if (hEventPlay)						// playing keys
		{
			// stop playing thread
			SetEvent(hEventPlay);			// quit play loop

			WaitForSingleObject(hThreadEv,INFINITE);
			CloseHandle(hThreadEv);
			hThreadEv = NULL;

			CloseHandle(hEventPlay);		// close playing keys event
			hEventPlay = NULL;
		}

		// macro file open
		if (hMacroFile != INVALID_HANDLE_VALUE) CloseHandle(hMacroFile);

		nMacroState = MACRO_OFF;
	}
	return 0;
}

//
// activate/deactivate slider
//
static VOID SliderEnable(HWND hDlg,BOOL bEnable)
{
	EnableWindow(GetDlgItem(hDlg,IDC_MACRO_SLOW),bEnable);
	EnableWindow(GetDlgItem(hDlg,IDC_MACRO_FAST),bEnable);
	EnableWindow(GetDlgItem(hDlg,IDC_MACRO_SLIDER),bEnable);
	return;
}

//
// Macro settings dialog
//
static INT_PTR CALLBACK MacroProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		// set slider
		SendDlgItemMessage(hDlg,IDC_MACRO_SLIDER,TBM_SETRANGE,FALSE,MAKELONG(0,MAX_SPEED-MIN_SPEED));
		SendDlgItemMessage(hDlg,IDC_MACRO_SLIDER,TBM_SETTICFREQ,MAX_SPEED/10,0);
		SendDlgItemMessage(hDlg,IDC_MACRO_SLIDER,TBM_SETPOS,TRUE,MAX_SPEED-nMacroTimeout);

		// set button
		CheckDlgButton(hDlg,bMacroRealSpeed ? IDC_MACRO_REAL : IDC_MACRO_MANUAL,BST_CHECKED);
		SliderEnable(hDlg,!bMacroRealSpeed);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_MACRO_REAL:
			SliderEnable(hDlg,FALSE);
			return TRUE;
		case IDC_MACRO_MANUAL:
			SliderEnable(hDlg,TRUE);
			return TRUE;
		case IDOK:
			// get macro data
			nMacroTimeout = MAX_SPEED - (INT) SendDlgItemMessage(hDlg,IDC_MACRO_SLIDER,TBM_GETPOS,0,0);
			bMacroRealSpeed = IsDlgButtonChecked(hDlg,IDC_MACRO_REAL);
			// no break
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
		}
		break;
	}
	return FALSE;
	UNREFERENCED_PARAMETER(lParam);
}

LRESULT OnToolMacroSettings(VOID)
{
	if (DialogBox(hApp, MAKEINTRESOURCE(IDD_MACROSET), hWnd, (DLGPROC)MacroProc) == -1)
		AbortMessage(_T("Macro Dialog Box Creation Error !"));
	return 0;
}
