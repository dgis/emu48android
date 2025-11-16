/*
 *   kml.c
 *
 *   This file is part of Emu48
 *
 *   Copyright (C) 1995 Sebastien Carlier
 *
 */
#include "pch.h"
#include "resource.h"
#include "Emu48.h"
#include "kml.h"

static VOID      InitLex(LPCTSTR szScript);
static VOID      CleanLex(VOID);
static VOID      SkipWhite(UINT nMode);
static TokenId   ParseToken(UINT nMode);
static DWORD     ParseInteger(VOID);
static LPTSTR    ParseString(VOID);
static TokenId   Lex(UINT nMode);
static KmlLine*  ParseLine(TokenId eCommand);
static KmlLine*  IncludeLines(BOOL bInclude, LPCTSTR szFilename);
static KmlLine*  ParseLines(BOOL bInclude);
static KmlBlock* ParseBlock(BOOL bInclude, TokenId eBlock);
static KmlBlock* IncludeBlocks(BOOL bInclude, LPCTSTR szFilename);
static KmlBlock* ParseBlocks(BOOL bInclude, BOOL bEndTokenEn);
static VOID      FreeLines(KmlLine* pLine);
static VOID      PressButton(UINT nId);
static VOID      ReleaseButton(UINT nId);
static VOID      PressButtonById(UINT nId);
static VOID      ReleaseButtonById(UINT nId);
static LPCTSTR   GetStringParam(KmlBlock* pBlock, TokenId eBlock, TokenId eCommand, UINT nParam);
static DWORD     GetIntegerParam(KmlBlock* pBlock, TokenId eBlock, TokenId eCommand, UINT nParam);
static KmlLine*  SkipLines(KmlLine* pLine, TokenId eCommand);
static KmlLine*  If(KmlLine* pLine, BOOL bCondition);
static KmlLine*  RunLine(KmlLine* pLine);
static KmlBlock* LoadKMLGlobal(LPCTSTR szFilename);

KmlBlock* pKml = NULL;
static KmlBlock* pVKey[256];
static BYTE      byVKeyMap[256];
static KmlButton pButton[256];
static KmlAnnunciator pAnnunciator[6];
static UINT      nButtons = 0;
static UINT      nScancodes = 0;
static UINT      nAnnunciators = 0;
static BOOL      bDebug = TRUE;
static WORD      wKeybLocId = 0;
static BOOL      bLocaleInc = FALSE;		// no locale block content included
static UINT      nLexLine;
static UINT      nLexInteger;
static UINT      nBlocksIncludeLevel;
static UINT      nLinesIncludeLevel;
static DWORD     nKMLFlags = 0;
static LPTSTR    szLexString;
static LPCTSTR   szText;
static LPCTSTR   szLexDelim[] =
{
	_T(" \t\n\r"),							// valid whitespaces for LEX_BLOCK
	_T(" \t\n\r"),							// valid whitespaces for LEX_COMMAND
	_T(" \t\r")								// valid whitespaces for LEX_PARAM
};

static CONST KmlToken pLexToken[] =
{
	{TOK_ANNUNCIATOR,000001,11,_T("Annunciator")},
	{TOK_BACKGROUND, 000000,10,_T("Background")},
	{TOK_IFPRESSED,  000001, 9,_T("IfPressed")},
	{TOK_RESETFLAG,  000001, 9,_T("ResetFlag")},
	{TOK_SCANCODE,   000001, 8,_T("Scancode")},
	{TOK_HARDWARE,   000002, 8,_T("Hardware")},
	{TOK_MENUITEM,   000001, 8,_T("MenuItem")},
	{TOK_SYSITEM,    000001, 7,_T("SysItem")},
	{TOK_SETFLAG,    000001, 7,_T("SetFlag")},
	{TOK_RELEASE,    000001, 7,_T("Release")},
	{TOK_VIRTUAL,    000000, 7,_T("Virtual")},
	{TOK_INCLUDE,    000002, 7,_T("Include")},
	{TOK_NOTFLAG,    000001, 7,_T("NotFlag")},
	{TOK_MENUBAR,    000001, 7,_T("Menubar")},		// for PPC compatibility reasons
	{TOK_GLOBAL,     000000, 6,_T("Global")},
	{TOK_AUTHOR,     000002, 6,_T("Author")},
	{TOK_BITMAP,     000002, 6,_T("Bitmap")},
	{TOK_OFFSET,     000011, 6,_T("Offset")},
	{TOK_ZOOMXY,     000011, 6,_T("Zoomxy")},
	{TOK_BUTTON,     000001, 6,_T("Button")},
	{TOK_IFFLAG,     000001, 6,_T("IfFlag")},
	{TOK_ONDOWN,     000000, 6,_T("OnDown")},
	{TOK_NOHOLD,     000000, 6,_T("NoHold")},
	{TOK_LOCALE,     000001, 6,_T("Locale")},
	{TOK_TOPBAR,     000001, 6,_T("Topbar")},		// for PPC compatibility reasons
	{TOK_TITLE,      000002, 5,_T("Title")},
	{TOK_OUTIN,      000011, 5,_T("OutIn")},
	{TOK_PATCH,      000002, 5,_T("Patch")},
	{TOK_PRINT,      000002, 5,_T("Print")},
	{TOK_DEBUG,      000001, 5,_T("Debug")},
	{TOK_COLOR,      001111, 5,_T("Color")},
	{TOK_MODEL,      000002, 5,_T("Model")},
	{TOK_CLASS,      000001, 5,_T("Class")},
	{TOK_PRESS,      000001, 5,_T("Press")},
	{TOK_IFMEM,      000111, 5,_T("IfMem")},
	{TOK_SCALE,      000011, 5,_T("Scale")},
	{TOK_TYPE,       000001, 4,_T("Type")},
	{TOK_SIZE,       000011, 4,_T("Size")},
	{TOK_ZOOM,       000001, 4,_T("Zoom")},
	{TOK_DOWN,       000011, 4,_T("Down")},
	{TOK_ELSE,       000000, 4,_T("Else")},
	{TOK_ONUP,       000000, 4,_T("OnUp")},
	{TOK_ICON,       000002, 4,_T("Icon")},
	{TOK_MAP,        000011, 3,_T("Map")},
	{TOK_ROM,        000002, 3,_T("Rom")},
	{TOK_VGA,        000001, 3,_T("Vga")},			// for PPC compatibility reasons
	{TOK_LCD,        000000, 3,_T("Lcd")},
	{TOK_END,        000000, 3,_T("End")},
	{TOK_NONE,       000000, 0,_T("")}
};

static CONST TokenId eIsGlobalBlock[] =
{
	TOK_GLOBAL,
	TOK_BACKGROUND,
	TOK_LCD,
	TOK_ANNUNCIATOR,
	TOK_BUTTON,
	TOK_SCANCODE,
	TOK_LOCALE
};

static CONST TokenId eIsBlock[] =
{
	TOK_IFFLAG,
	TOK_IFPRESSED,
	TOK_IFMEM,
	TOK_ONDOWN,
	TOK_ONUP
};

static BOOL bClicking = FALSE;
static UINT uButtonClicked = 0;

static BOOL bKeyPressed = FALSE;			// no key pressed
static UINT uLastKeyPressed = 0;			// var for last pressed key

static INT nScaleMul = 0;					// no scaling
static INT nScaleDiv = 0;

//################
//#
//#    Compilation Result
//#
//################

static UINT nLogLength = 0;
static LPTSTR szLog = NULL;

static VOID ClearLog()
{
	nLogLength = 0;
	if (szLog != NULL)
	{
		free(szLog);
		szLog = NULL;
	}
	return;
}

static VOID AddToLog(LPCTSTR szString)
{
	LPTSTR szLogTmp;

	UINT nLength = lstrlen(szString) + 2;	// CR+LF
	if (szLog == NULL)						// no log
	{
		nLogLength = 1;						// room for \0
	}

	szLogTmp = (LPTSTR) realloc(szLog,(nLogLength+nLength)*sizeof(szLog[0]));
	if (szLogTmp == NULL)
	{
		ClearLog();
		return;
	}
	szLog = szLogTmp;
	lstrcpy(&szLog[nLogLength-1],szString);
	nLogLength += nLength;
	szLog[nLogLength-3] = _T('\r');
	szLog[nLogLength-2] = _T('\n');
	szLog[nLogLength-1] = 0;
	return;
}

static VOID __cdecl PrintfToLog(LPCTSTR lpFormat, ...)
{
	TCHAR cOutput[1024];
	va_list arglist;

	va_start(arglist,lpFormat);
	wvsprintf(cOutput,lpFormat,arglist);
	AddToLog(cOutput);
	va_end(arglist);
	return;
}

static INT_PTR CALLBACK KMLLogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	LPCTSTR szString;

	switch (message)
	{
	case WM_INITDIALOG:
		// set OK
		EnableWindow(GetDlgItem(hDlg,IDOK),(BOOL) lParam);
		// set IDC_TITLE
		szString = GetStringParam(pKml, TOK_GLOBAL, TOK_TITLE, 0);
		if (szString == NULL) szString = _T("Untitled");
		SetDlgItemText(hDlg,IDC_TITLE,szString);
		// set IDC_AUTHOR
		szString = GetStringParam(pKml, TOK_GLOBAL, TOK_AUTHOR, 0);
		if (szString == NULL) szString = _T("<Unknown Author>");
		SetDlgItemText(hDlg,IDC_AUTHOR,szString);
		// set IDC_KMLLOG
		szString = szLog;
		if (szString == NULL) szString = _T("Memory Allocation Failure.");
		SetDlgItemText(hDlg,IDC_KMLLOG,szString);
		// set IDC_ALWAYSDISPLOG
		CheckDlgButton(hDlg,IDC_ALWAYSDISPLOG,bAlwaysDisplayLog);
		return TRUE;
	case WM_COMMAND:
		wParam = LOWORD(wParam);
		if ((wParam==IDOK)||(wParam==IDCANCEL))
		{
			bAlwaysDisplayLog = IsDlgButtonChecked(hDlg, IDC_ALWAYSDISPLOG);
			EndDialog(hDlg, wParam);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

BOOL DisplayKMLLog(BOOL bOkEnabled)
{
	return IDOK == DialogBoxParam(hApp,
								  MAKEINTRESOURCE(IDD_KMLLOG),
								  hWnd,
								  (DLGPROC)KMLLogProc,
								  bOkEnabled);
}



//################
//#
//#    Choose Script
//#
//################

typedef struct _KmlScript
{
	LPTSTR szFilename;
	LPTSTR szTitle;
	DWORD  nId;
	struct _KmlScript* pNext;
} KmlScript;

static KmlScript* pKmlList = NULL;
static CHAR cKmlType;

static VOID DestroyKmlList(VOID)
{
	while (pKmlList)
	{
		KmlScript* pList = pKmlList->pNext;
		free(pKmlList->szFilename);
		free(pKmlList->szTitle);
		free(pKmlList);
		pKmlList = pList;
	}
	return;
}

static VOID CreateKmlList(VOID)
{
	HANDLE hFindFile;
	WIN32_FIND_DATA pFindFileData;
	UINT nKmlFiles;

	_ASSERT(pKmlList == NULL);				// KML file list must be empty
	SetCurrentDirectory(szEmuDirectory);
	hFindFile = FindFirstFile(_T("*.KML"),&pFindFileData);
	SetCurrentDirectory(szCurrentDirectory);
	if (hFindFile == INVALID_HANDLE_VALUE) return;
	nKmlFiles = 0;
	do
	{
		KmlScript* pScript;
		KmlBlock*  pBlock;
		LPCTSTR szTitle;
		BOOL bInvalidPlatform;

		pBlock = LoadKMLGlobal(pFindFileData.cFileName);
		if (pBlock == NULL) continue;
		// check for correct KML script platform
		szTitle = GetStringParam(pBlock,TOK_GLOBAL,TOK_HARDWARE,0);
		bInvalidPlatform = (szTitle && lstrcmpi(_T(HARDWARE),szTitle) != 0);
		// check for supported Model
		szTitle = GetStringParam(pBlock,TOK_GLOBAL,TOK_MODEL,0);
		// skip all scripts with invalid platform and invalid or different Model statement
		if (   bInvalidPlatform
			|| (szTitle == NULL)
			|| (cKmlType && szTitle[0] != cKmlType)
			|| !isModelValid(szTitle[0]))
		{
			FreeBlocks(pBlock);
			continue;
		}
		pScript = (KmlScript*) malloc(sizeof(KmlScript));
		if (pScript)						// script node allocated
		{
			pScript->szFilename = DuplicateString(pFindFileData.cFileName);
			szTitle = GetStringParam(pBlock,TOK_GLOBAL,TOK_TITLE,0);
			if (szTitle == NULL) szTitle = pScript->szFilename;
			pScript->szTitle = DuplicateString(szTitle);
			if (pScript->szFilename == NULL || pScript->szTitle == NULL)
			{
				free(pScript->szFilename);
				free(pScript->szTitle);
				free(pScript);
				FreeBlocks(pBlock);
				continue;
			}
			pScript->nId = nKmlFiles;
			pScript->pNext = pKmlList;
			pKmlList = pScript;
			nKmlFiles++;
		}
		FreeBlocks(pBlock);
	} while (FindNextFile(hFindFile,&pFindFileData));
	FindClose(hFindFile);
	return;
};

static INT CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData)
{
	TCHAR szDir[MAX_PATH];

	switch(uMsg)
	{
	case BFFM_INITIALIZED:
		SendMessage(hwnd,BFFM_SETSELECTION,TRUE,pData);
		break;
	case BFFM_SELCHANGED:
		// Set the status window to the currently selected path.
		if (SHGetPathFromIDList((LPITEMIDLIST) lp,szDir))
		{
			SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,(LPARAM) szDir);
		}
		break;
	}
	return 0;
}

static VOID BrowseFolder(HWND hDlg)
{
	TCHAR szDir[MAX_PATH];
	BROWSEINFO bi;
	LPITEMIDLIST pidl;
	LPMALLOC pMalloc;

	// gets the shell's default allocator
	if (SUCCEEDED(SHGetMalloc(&pMalloc)))
	{
		GetDlgItemText(hDlg,IDC_EMUDIR,szDir,ARRAYSIZEOF(szDir));

		ZeroMemory(&bi,sizeof(bi));
		bi.hwndOwner = hDlg;
		bi.pidlRoot = NULL;
		bi.pszDisplayName = NULL;
		bi.lpszTitle = _T("Choose a folder:");
		bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT;
		bi.lpfn = BrowseCallbackProc;
		bi.lParam = (LPARAM) szDir;			// current setting

		pidl = SHBrowseForFolder(&bi);
		if (pidl)
		{
			if (SHGetPathFromIDList(pidl,szDir))
			{
				SetDlgItemText(hDlg,IDC_EMUDIR,szDir);
			}
			// free the PIDL allocated by SHBrowseForFolder
			#if defined __cplusplus
				pMalloc->Free(pidl);
			#else
				pMalloc->lpVtbl->Free(pMalloc,pidl);
			#endif
		}
		// release the shell's allocator
		#if defined __cplusplus
			pMalloc->Release();
		#else
			pMalloc->lpVtbl->Release(pMalloc);
		#endif
	}
	return;
}

static VOID UpdateScriptList(HWND hDlg)
{
	HWND hList;
	KmlScript* pList;
	UINT nIndex,nEntries;
	DWORD dwActId = (DWORD) CB_ERR;

	// add all script titles to combo box
	hList = GetDlgItem(hDlg,IDC_KMLSCRIPT);
	SendMessage(hList, CB_RESETCONTENT, 0, 0);
	for (nEntries = 0, pList = pKmlList; pList; pList = pList->pNext)
	{
		nIndex = (UINT) SendMessage(hList, CB_ADDSTRING, 0, (LPARAM)pList->szTitle);
		SendMessage(hList, CB_SETITEMDATA, nIndex, (LPARAM) pList->nId);

		// this has the same filename like the actual KML script
		if (lstrcmpi(szCurrentKml, pList->szFilename) == 0)
			dwActId = pList->nId;

		nEntries++;
	}

	while (--nEntries > 0)					// scan all combo box items
	{
		// found ID of actual KML script
		if ((DWORD) SendMessage(hList, CB_GETITEMDATA, nEntries, 0) == dwActId)
			break;
	}
	SendMessage(hList, CB_SETCURSEL, nEntries, 0);
	return;
}

static INT_PTR CALLBACK ChooseKMLProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND hList;
	KmlScript* pList;
	UINT nIndex;

	switch (message)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hDlg,IDC_EMUDIR,szEmuDirectory);
		UpdateScriptList(hDlg);				// update combo box with script titles
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_EMUDIRSEL:
			BrowseFolder(hDlg);				// select new folder for IDC_EMUDIR
			// fall into IDC_UPDATE to search for KML files in new folder
		case IDC_UPDATE:
			DestroyKmlList();
			GetDlgItemText(hDlg,IDC_EMUDIR,szEmuDirectory,ARRAYSIZEOF(szEmuDirectory));
			CreateKmlList();
			UpdateScriptList(hDlg);			// update combo box with script titles
			return TRUE;
		case IDOK:
			GetDlgItemText(hDlg,IDC_EMUDIR,szEmuDirectory,ARRAYSIZEOF(szEmuDirectory));
			hList = GetDlgItem(hDlg,IDC_KMLSCRIPT);
			nIndex = (UINT) SendMessage(hList, CB_GETCURSEL, 0, 0);
			nIndex = (UINT) SendMessage(hList, CB_GETITEMDATA, nIndex, 0);
			for (pList = pKmlList; pList; pList = pList->pNext)
			{
				if (pList->nId == nIndex)
				{
					lstrcpy(szCurrentKml, pList->szFilename);
					EndDialog(hDlg, IDOK);
					break;
				}
			}
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
	}
	return FALSE;
	UNREFERENCED_PARAMETER(lParam);
}

BOOL DisplayChooseKml(CHAR cType)
{
	INT_PTR nResult;
	cKmlType = cType;
	CreateKmlList();
	nResult = DialogBox(hApp, MAKEINTRESOURCE(IDD_CHOOSEKML), hWnd, (DLGPROC)ChooseKMLProc);
	DestroyKmlList();
	return (nResult == IDOK);
}



//################
//#
//#    KML File Mapping
//#
//################

static LPTSTR MapKMLFile(HANDLE hFile)
{
	DWORD  lBytesRead;
	DWORD  dwFileSizeLow;
	DWORD  dwFileSizeHigh;
	LPTSTR lpBuf = NULL;

	dwFileSizeLow = GetFileSize(hFile, &dwFileSizeHigh);
	if (dwFileSizeHigh != 0)
	{
		AddToLog(_T("File is too large."));
		goto fail;
	}

	lpBuf = (LPTSTR) malloc((dwFileSizeLow+1)*sizeof(lpBuf[0]));
	if (lpBuf == NULL)
	{
		PrintfToLog(_T("Cannot allocate %i bytes."), (dwFileSizeLow+1)*sizeof(lpBuf[0]));
		goto fail;
	}
	#if defined _UNICODE
	{
		LPSTR szTmp = (LPSTR) malloc(dwFileSizeLow+1);
		if (szTmp == NULL)
		{
			free(lpBuf);
			lpBuf = NULL;
			PrintfToLog(_T("Cannot allocate %i bytes."), dwFileSizeLow+1);
			goto fail;
		}
		ReadFile(hFile, szTmp, dwFileSizeLow, &lBytesRead, NULL);
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szTmp, lBytesRead, lpBuf, dwFileSizeLow+1);
		free(szTmp);
	}
	#else
	{
		ReadFile(hFile, lpBuf, dwFileSizeLow, &lBytesRead, NULL);
	}
	#endif
	lpBuf[dwFileSizeLow] = 0;

fail:
	CloseHandle(hFile);
	return lpBuf;
}



//################
//#
//#    Script Parsing
//#
//################

static VOID InitLex(LPCTSTR szScript)
{
	nLexLine = 1;
	szText = szScript;
	return;
}

static VOID CleanLex(VOID)
{
	nLexLine = 0;
	nLexInteger = 0;
	szLexString = NULL;
	szText = NULL;
	return;
}

static BOOL IsGlobalBlock(TokenId eId)
{
	UINT i;

	for (i = 0; i < ARRAYSIZEOF(eIsGlobalBlock); ++i)
	{
		if (eId == eIsGlobalBlock[i]) return TRUE;
	}
	return FALSE;
}

static BOOL IsBlock(TokenId eId)
{
	UINT i;

	for (i = 0; i < ARRAYSIZEOF(eIsBlock); ++i)
	{
		if (eId == eIsBlock[i]) return TRUE;
	}
	return FALSE;
}

static LPCTSTR GetStringOf(TokenId eId)
{
	UINT i;

	for (i = 0; pLexToken[i].nLen; ++i)
	{
		if (pLexToken[i].eId == eId) return pLexToken[i].szName;
	}
	return _T("<Undefined>");
}

static VOID SkipWhite(UINT nMode)
{
	LPCTSTR pcDelim;

	while (*szText)
	{
		// search for delimiter
		if ((pcDelim = _tcschr(szLexDelim[nMode],*szText)) != NULL)
		{
			_ASSERT(*pcDelim != 0);			// no EOS
			if (*pcDelim == _T('\n')) nLexLine++;
			szText++;
			continue;
		}
		if (*szText == _T('#'))				// start of remark
		{
			// skip until LF or EOS
			do szText++; while (*szText != _T('\n') && *szText != 0);
			if (nMode != LEX_PARAM) continue;
		}
		break;
	}
	return;
}

static TokenId ParseToken(UINT nMode)
{
	UINT i,j;

	for (i = 0; szText[i]; i++)				// search for delimeter
	{
		if (_tcschr(szLexDelim[nMode],szText[i]) != NULL)
			break;
	}
	if (i == 0) return TOK_NONE;

	// token length longer or equal than current command
	for (j = 0; pLexToken[j].nLen >= i; ++j)
	{
		if (pLexToken[j].nLen == i)			// token length has command length
		{
			if (_tcsncmp(pLexToken[j].szName,szText,i) == 0)
			{
				szText += i;				// remove command from text
				return pLexToken[j].eId;	// return token Id
			}
		}
	}
	if (bDebug)								// token not found
	{
		// allocate target string memory with token length
		LPTSTR szToken = (LPTSTR) malloc((i+1) * sizeof(szToken[0]));
		lstrcpyn(szToken,szText,i+1);		// copy token text and append EOS
		PrintfToLog(_T("%i: Undefined token %s"),nLexLine,szToken);
		free(szToken);
	}
	return TOK_NONE;
}

static DWORD ParseInteger(VOID)
{
	DWORD nNum = 0;
	while (_istdigit((_TUCHAR) *szText))
	{
		nNum = nNum * 10 + ((*szText) - _T('0'));
		szText++;
	}
	return nNum;
}

static LPTSTR ParseString(VOID)
{
	LPTSTR lpszString = NULL;				// no mem allocated
	UINT   nBlock = 0;
	UINT   nLength = 0;

	LPTSTR lpszAllocString;

	szText++;								// skip leading '"'

	while (*szText != _T('"'))
	{
		if (nLength >= nBlock)				// ran out of buffer space
		{
			nBlock += 256;
			lpszAllocString = (LPTSTR) realloc(lpszString,nBlock * sizeof(lpszString[0]));
			if (lpszAllocString)
			{
				lpszString = lpszAllocString;
			}
			else
			{
				free(lpszString);			// cleanup allocation failture
				return NULL;
			}
		}

		if (*szText == _T('\\'))			// escape char
		{
			// skip a '\' escape char before a quotation to
			// decode the \" sequence as a quotation mark inside text
			switch (szText[1])
			{
			case _T('\"'):
			case _T('\\'):
				++szText;					// skip escape char '\'
				break;
			}
		}

		if (*szText == 0)					// EOS found inside string
		{
			lpszString[nLength] = 0;		// set EOS
			PrintfToLog(_T("%i: Invalid string %s."), nLexLine, lpszString);
			free(lpszString);
			return NULL;
		}
		lpszString[nLength++] = *szText++;	// save char
	}
	szText++;								// skip ending '"'

	// release unnecessary allocated bytes or allocate byte for EOS
	if ((lpszAllocString = (LPTSTR) realloc(lpszString,(nLength+1) * sizeof(lpszString[0]))))
	{
		lpszAllocString[nLength] = 0;		// set EOS
	}
	else
	{
		free(lpszString);					// cleanup allocation failture
	}
	return lpszAllocString;
}

static TokenId Lex(UINT nMode)
{
	_ASSERT(nMode >= LEX_BLOCK && nMode <= LEX_PARAM);
	_ASSERT(nMode >= 0 && nMode < ARRAYSIZEOF(szLexDelim));

	SkipWhite(nMode);
	if (_istdigit((_TUCHAR) *szText))
	{
		nLexInteger = ParseInteger();
		return TOK_INTEGER;
	}
	if (*szText == _T('"'))
	{
		szLexString = ParseString();
		return TOK_STRING;
	}
	if (nMode == LEX_PARAM)
	{
		if (*szText == _T('\n'))			// end of line
		{
			nLexLine++;						// next line
			szText++;						// skip LF
			return TOK_EOL;
		}
		if (*szText == 0)					// end of file
		{
			return TOK_EOL;
		}
	}
	return ParseToken(nMode);
}

static KmlLine* ParseLine(TokenId eCommand)
{
	UINT     i, j;
	DWORD    nParams;
	TokenId  eToken;
	KmlLine* pLine;

	for (i = 0; pLexToken[i].nLen; ++i)
	{
		if (pLexToken[i].eId == eCommand) break;
	}
	if (pLexToken[i].nLen == 0) return NULL;

	if ((pLine = (KmlLine*) calloc(1,sizeof(KmlLine))) == NULL)
		return NULL;

	pLine->eCommand = eCommand;

	for (j = 0, nParams = pLexToken[i].nParams; TRUE; nParams >>= 3)
	{
		// check for parameter overflow
		_ASSERT(j < ARRAYSIZEOF(pLine->nParam));

		eToken = Lex(LEX_PARAM);			// decode argument token
		if ((nParams & 7) == TYPE_NONE)
		{
			if (eToken != TOK_EOL)
			{
				PrintfToLog(_T("%i: Too many parameters for %s (%i expected)."), nLexLine, pLexToken[i].szName, j);
				break;						// free memory of arguments
			}
			return pLine;					// normal exit -> parsed line
		}
		if ((nParams & 7) == TYPE_INTEGER)
		{
			if (eToken != TOK_INTEGER)
			{
				PrintfToLog(_T("%i: Parameter %i of %s must be an integer."), nLexLine, j+1, pLexToken[i].szName);
				break;						// free memory of arguments
			}
			pLine->nParam[j++] = nLexInteger;
			continue;
		}
		if ((nParams & 7) == TYPE_STRING)
		{
			if (eToken != TOK_STRING)
			{
				PrintfToLog(_T("%i: Parameter %i of %s must be a string."), nLexLine, j+1, pLexToken[i].szName);
				break;						// free memory of arguments
			}
			pLine->nParam[j++] = (DWORD_PTR) szLexString;
			szLexString = NULL;
			continue;
		}
		_ASSERT(FALSE);						// unknown parameter type
		break;
	}

	// if last argument was string, free it
	if (eToken == TOK_STRING)
	{
		free(szLexString);
		szLexString = NULL;
	}

	nParams = pLexToken[i].nParams;			// get argument types of command
	for (i = 0; i < j; ++i)					// handle all scanned arguments
	{
		if ((nParams & 7) == TYPE_STRING)	// string type
		{
			free((LPVOID)pLine->nParam[i]);
		}
		nParams >>= 3;						// next argument type
	}
	free(pLine);
	return NULL;
}

static KmlLine* IncludeLines(BOOL bInclude, LPCTSTR szFilename)
{
	HANDLE   hFile;
	LPTSTR   lpbyBuf;
	UINT     uOldLine;
	LPCTSTR  szOldText;
	KmlLine* pLine;

	SetCurrentDirectory(szEmuDirectory);
	hFile = CreateFile(szFilename,
					   GENERIC_READ,
					   FILE_SHARE_READ,
					   NULL,
					   OPEN_EXISTING,
					   FILE_FLAG_SEQUENTIAL_SCAN,
					   NULL);
	SetCurrentDirectory(szCurrentDirectory);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		PrintfToLog(_T("Error while opening include file %s."), szFilename);
		return NULL;
	}
	if ((lpbyBuf = MapKMLFile(hFile)) == NULL)
	{
		return NULL;
	}

	uOldLine  = nLexLine;
	szOldText = szText;

	nLinesIncludeLevel++;
	PrintfToLog(_T("l%i:%s %s"),
		nLinesIncludeLevel,
		(bInclude) ? _T("Including") : _T("Parsing"),
		szFilename);
	InitLex(lpbyBuf);
	pLine = ParseLines(bInclude);
	CleanLex();
	nLinesIncludeLevel--;

	nLexLine  = uOldLine;
	szText    = szOldText;
	free(lpbyBuf);

	return pLine;
}

static KmlLine* ParseLines(BOOL bInclude)
{
	KmlLine* pFirst = NULL;
	KmlLine* pLine  = NULL;
	TokenId  eToken;
	UINT     nLevel = 0;

	while ((eToken = Lex(LEX_COMMAND)) != TOK_NONE)
	{
		if (IsGlobalBlock(eToken))			// check for block command
		{
			PrintfToLog(_T("%i: Invalid Command %s."), nLexLine, GetStringOf(eToken));
			goto abort;
		}
		if (IsBlock(eToken)) nLevel++;
		if (eToken == TOK_INCLUDE)
		{
			LPTSTR szFilename;
			UINT   nLexLineKml;

			eToken = Lex(LEX_PARAM);		// get include parameter in 'szLexString'
			if (eToken != TOK_STRING)		// not a string (token don't begin with ")
			{
				PrintfToLog(_T("%i: Include: string expected as parameter."), nLexLine);
				goto abort;
			}
			szFilename = szLexString;		// save pointer to allocated memory
			szLexString = NULL;
			nLexLineKml = nLexLine;			// save line number
			eToken = Lex(LEX_PARAM);		// decode argument
			if (eToken != TOK_EOL)
			{
				free(szFilename);			// free filename string
				if (eToken == TOK_STRING)
				{
					free(szLexString);
					szLexString = NULL;
				}
				PrintfToLog(_T("%i: Include: Too many parameters."), nLexLine);
				goto abort;
			}
			if (pFirst)
			{
				pLine = pLine->pNext = IncludeLines(bInclude,szFilename);
			}
			else
			{
				pLine = pFirst = IncludeLines(bInclude,szFilename);
			}
			free(szFilename);				// free filename string
			if (pLine == NULL)				// parsing error
			{
				nLexLine = nLexLineKml;		// restore line number
				goto abort;
			}
			while (pLine->pNext) pLine=pLine->pNext;
			continue;
		}
		if (eToken == TOK_END)
		{
			if (nLevel)
			{
				nLevel--;
			}
			else
			{
				if (pFirst == NULL)			// regular exit with empty block
				{
					// create an empty line
					if ((pFirst = (KmlLine*) calloc(1,sizeof(KmlLine))) == NULL)
						goto abort;

					pLine = pFirst;
					pLine->eCommand = TOK_NONE;
				}
				if (pLine) pLine->pNext = NULL;
				_ASSERT(szLexString == NULL);
				return pFirst;
			}
		}
		if (pFirst)
		{
			pLine = pLine->pNext = ParseLine(eToken);
		}
		else
		{
			pLine = pFirst = ParseLine(eToken);
		}
		if (pLine == NULL)					// parsing error
			goto abort;
	}
	if (nLinesIncludeLevel)
	{
		if (pLine) pLine->pNext = NULL;
		_ASSERT(szLexString == NULL);
		return pFirst;
	}
abort:
	if (pFirst) FreeLines(pFirst);
	_ASSERT(szLexString == NULL);
	return NULL;
}

static KmlBlock* ParseBlock(BOOL bInclude, TokenId eType)
{
	UINT      i;
	KmlBlock* pBlock;
	TokenId   eToken;

	nLinesIncludeLevel = 0;

	if ((pBlock = (KmlBlock *) calloc(1,sizeof(KmlBlock))) == NULL)
		return NULL;

	pBlock->eType = eType;

	for (i = 0; pLexToken[i].nLen; ++i)		// search for token
	{
		if (pLexToken[i].eId == eType) break;
	}

	if (pLexToken[i].nParams)				// has block command arguments
	{
		// block command parser accept only one integer argument
		_ASSERT(pLexToken[i].nParams == TYPE_INTEGER);

		eToken = Lex(LEX_PARAM);			// decode argument
		if (eToken != TOK_INTEGER)
		{
			if (eToken == TOK_STRING)
			{
				free(szLexString);
				szLexString = NULL;
			}
			PrintfToLog(_T("%i: Block %s parameter must be an integer."), nLexLine, pLexToken[i].szName);
			free(pBlock);
			_ASSERT(szLexString == NULL);
			return NULL;
		}

		pBlock->nId = nLexInteger;			// remember block no.
	}

	eToken = Lex(LEX_PARAM);				// decode argument
	if (eToken != TOK_EOL)
	{
		if (eToken == TOK_STRING)
		{
			free(szLexString);
			szLexString = NULL;
		}
		PrintfToLog(_T("%i: Too many parameters for block %s."), nLexLine, pLexToken[i].szName);
		free(pBlock);
		_ASSERT(szLexString == NULL);
		return NULL;
	}

	pBlock->pFirstLine = ParseLines(bInclude);
	if (pBlock->pFirstLine == NULL)			// break on ParseLines error
	{
		free(pBlock);
		pBlock = NULL;
	}
	_ASSERT(szLexString == NULL);
	return pBlock;
}

static KmlBlock* IncludeBlocks(BOOL bInclude, LPCTSTR szFilename)
{
	HANDLE    hFile;
	LPTSTR    lpbyBuf;
	UINT      uOldLine;
	LPCTSTR   szOldText;
	KmlBlock* pFirst;

	SetCurrentDirectory(szEmuDirectory);
	hFile = CreateFile(szFilename,
					   GENERIC_READ,
					   FILE_SHARE_READ,
					   NULL,
					   OPEN_EXISTING,
					   FILE_FLAG_SEQUENTIAL_SCAN,
					   NULL);
	SetCurrentDirectory(szCurrentDirectory);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		PrintfToLog(_T("Error while opening include file %s."), szFilename);
		return NULL;
	}
	if ((lpbyBuf = MapKMLFile(hFile)) == NULL)
	{
		return NULL;
	}

	uOldLine  = nLexLine;
	szOldText = szText;

	nBlocksIncludeLevel++;
	PrintfToLog(_T("b%i:%s %s"),
		nBlocksIncludeLevel,
		(bInclude) ? _T("Including") : _T("Parsing"),
		szFilename);
	InitLex(lpbyBuf);
	pFirst = ParseBlocks(bInclude, FALSE);
	CleanLex();
	nBlocksIncludeLevel--;

	nLexLine  = uOldLine;
	szText    = szOldText;
	free(lpbyBuf);

	return pFirst;
}

static KmlBlock* ParseBlocks(BOOL bInclude, BOOL bEndTokenEn)
{
	TokenId eToken;
	KmlBlock* pFirst = NULL;
	KmlBlock* pBlock = NULL;

	while ((eToken = Lex(LEX_BLOCK)) != TOK_NONE)
	{
		// allow TOK_END token only as end of a "Locale" block
		if (bEndTokenEn && eToken == TOK_END)
		{
			return pFirst;
		}
		if (eToken == TOK_INCLUDE)
		{
			LPTSTR szFilename;
			UINT   nLexLineKml;

			eToken = Lex(LEX_PARAM);		// get include parameter in 'szLexString'
			if (eToken != TOK_STRING)		// not a string (token don't begin with ")
			{
				AddToLog(_T("Include: string expected as parameter."));
				goto abort;
			}
			szFilename = szLexString;		// save pointer to allocated memory
			szLexString = NULL;
			nLexLineKml = nLexLine;			// save line number
			eToken = Lex(LEX_PARAM);		// decode argument
			if (eToken != TOK_EOL)
			{
				free(szFilename);			// free filename string
				PrintfToLog(_T("%i: Include: Too many parameters."), nLexLine);
				goto abort;
			}
			if (pFirst)
				pBlock = pBlock->pNext = IncludeBlocks(bInclude,szFilename);
			else
				pBlock = pFirst = IncludeBlocks(bInclude,szFilename);
			free(szFilename);				// free filename string
			if (pBlock == NULL)				// parsing error
			{
				nLexLine = nLexLineKml;		// restore line number
				goto abort;
			}
			while (pBlock->pNext) pBlock = pBlock->pNext;
			continue;
		}
		if (eToken == TOK_LOCALE)
		{
			WORD      wLocId,wKeybId;
			KmlBlock* pData;
			BOOL      bIncludeId;

			eToken = Lex(LEX_PARAM);		// get include parameter in 'nLexInteger'
			if (eToken != TOK_INTEGER)
			{
				PrintfToLog(_T("%i: Locale parameter must be an integer."), nLexLine);
				goto abort;
			}
			wLocId = nLexInteger;			// requested keyboard locale id
			eToken = Lex(LEX_PARAM);		// decode argument
			if (eToken != TOK_EOL)
			{
				PrintfToLog(_T("%i: Too many parameters for Locale."), nLexLine);
				goto abort;
			}

			wKeybId = wKeybLocId;			// get current keyboard layout input locale
			if (SUBLANGID(wLocId) == SUBLANG_NEUTRAL)
			{
				wKeybId = (PRIMARYLANGID(wLocId) != LANG_NEUTRAL)
						? PRIMARYLANGID(wKeybId)
						: LANG_NEUTRAL;
			}

			// check if block should be included or skipped
			bIncludeId = bInclude && !bLocaleInc && (wKeybId == wLocId);

			PrintfToLog(_T("b%i:%s \"Locale %i\""),
				nBlocksIncludeLevel,
				(bIncludeId) ? _T("Including") : _T("Skipping"),
				wLocId);

			pData = ParseBlocks(bIncludeId,TRUE); // parse block, allow "End"
			if (pData == NULL)				// parsing error
			{
				// don't blame the block twice
				if (pFirst) FreeBlocks(pFirst);
				return NULL;
			}

			if (bIncludeId)					// insert blocks to block list
			{
				if (pFirst)
					pBlock = pBlock->pNext = pData;
				else
					pBlock = pFirst = pData;

				// goto end of insertion
				while (pBlock->pNext) pBlock = pBlock->pNext;
				bLocaleInc = TRUE;			// locale block content included
			}
			else							// skip block
			{
				if (pData) FreeBlocks(pData);
			}
			continue;
		}
		if (!IsGlobalBlock(eToken))			// check for valid block commands
		{
			PrintfToLog(_T("%i: Invalid Block %s."), nLexLine, GetStringOf(eToken));
			goto abort;
		}
		if (pFirst)
			pBlock = pBlock->pNext = ParseBlock(bInclude,eToken);
		else
			pBlock = pFirst = ParseBlock(bInclude,eToken);

		if (pBlock == NULL) goto abort;
	}
	if (*szText != 0)						// still KML text left
	{
		goto abort;
	}
	_ASSERT(szLexString == NULL);
	return pFirst;

abort:
	PrintfToLog(_T("Fatal Error at line %i."), nLexLine);
	if (szLexString && eToken == TOK_STRING)
	{
		free(szLexString);
		szLexString = NULL;
	}
	if (pFirst) FreeBlocks(pFirst);
	_ASSERT(szLexString == NULL);
	return NULL;
}



//################
//#
//#    Initialization Phase
//#
//################

static VOID InitGlobal(KmlBlock* pBlock)
{
	KmlLine* pLine = pBlock->pFirstLine;
	while (pLine)
	{
		switch (pLine->eCommand)
		{
		case TOK_TITLE:
			PrintfToLog(_T("Title: %s"), (LPTSTR)pLine->nParam[0]);
			break;
		case TOK_AUTHOR:
			PrintfToLog(_T("Author: %s"), (LPTSTR)pLine->nParam[0]);
			break;
		case TOK_PRINT:
			AddToLog((LPTSTR)pLine->nParam[0]);
			break;
		case TOK_HARDWARE:
			PrintfToLog(_T("Hardware Platform: %s"), (LPTSTR)pLine->nParam[0]);
			break;
		case TOK_MODEL:
			cCurrentRomType = ((BYTE *)pLine->nParam[0])[0];
			PrintfToLog(_T("Calculator Model : %c"), cCurrentRomType);
			break;
		case TOK_CLASS:
			nCurrentClass = (UINT) pLine->nParam[0];
			PrintfToLog(_T("Calculator Class : %u"), nCurrentClass);
			break;
		case TOK_ICON:
			if (!LoadIconFromFile((LPTSTR) pLine->nParam[0]))
			{
				PrintfToLog(_T("Cannot load Icon %s."), (LPTSTR)pLine->nParam[0]);
				break;
			}
			PrintfToLog(_T("Icon %s loaded."), (LPTSTR)pLine->nParam[0]);
			break;
		case TOK_DEBUG:
			bDebug = (BOOL) pLine->nParam[0]&1;
			PrintfToLog(_T("Debug %s"), bDebug?_T("On"):_T("Off"));
			break;
		case TOK_ROM:
			if (pbyRom != NULL)
			{
				PrintfToLog(_T("Rom %s ignored."), (LPTSTR)pLine->nParam[0]);
				AddToLog(_T("Please put only one Rom command in the Global block."));
				break;
			}
			if (!MapRom((LPTSTR)pLine->nParam[0]))
			{
				PrintfToLog(_T("Cannot open Rom %s."), (LPTSTR)pLine->nParam[0]);
				break;
			}
			PrintfToLog(_T("Rom %s loaded."), (LPTSTR)pLine->nParam[0]);
			break;
		case TOK_PATCH:
			if (pbyRom == NULL)
			{
				PrintfToLog(_T("Patch %s ignored."), (LPTSTR)pLine->nParam[0]);
				AddToLog(_T("Please put the Rom command before any Patch."));
				break;
			}
			if (PatchRom((LPTSTR)pLine->nParam[0]) == TRUE)
				PrintfToLog(_T("Patch %s loaded."), (LPTSTR)pLine->nParam[0]);
			else
				PrintfToLog(_T("Patch %s is Wrong or Missing."), (LPTSTR)pLine->nParam[0]);
			break;
		case TOK_BITMAP:
			if (hMainDC != NULL)
			{
				PrintfToLog(_T("Bitmap %s ignored."), (LPTSTR)pLine->nParam[0]);
				AddToLog(_T("Please put only one Bitmap command in the Global block."));
				break;
			}
			if (!CreateMainBitmap((LPTSTR)pLine->nParam[0]))
			{
				PrintfToLog(_T("Cannot load Bitmap %s."), (LPTSTR)pLine->nParam[0]);
				break;
			}
			PrintfToLog(_T("Bitmap %s loaded."), (LPTSTR)pLine->nParam[0]);
			break;
		case TOK_COLOR:
			dwTColorTol = (DWORD) pLine->nParam[0];
			dwTColor = RGB((BYTE) pLine->nParam[1],(BYTE) pLine->nParam[2],(BYTE) pLine->nParam[3]);
			break;
		case TOK_SCALE:
			nScaleMul = (INT) pLine->nParam[0];
			nScaleDiv = (INT) pLine->nParam[1];
			break;
		default:
			PrintfToLog(_T("Command %s Ignored in Block %s"), GetStringOf(pLine->eCommand), GetStringOf(pBlock->eType));
		}
		pLine = pLine->pNext;
	}
	return;
}

static KmlLine* InitBackground(KmlBlock* pBlock)
{
	KmlLine* pLine = pBlock->pFirstLine;
	while (pLine)
	{
		switch (pLine->eCommand)
		{
		case TOK_OFFSET:
			nBackgroundX = (UINT) pLine->nParam[0];
			nBackgroundY = (UINT) pLine->nParam[1];
			break;
		case TOK_SIZE:
			nBackgroundW = (UINT) pLine->nParam[0];
			nBackgroundH = (UINT) pLine->nParam[1];
			break;
		case TOK_END:
			return pLine;
		default:
			PrintfToLog(_T("Command %s Ignored in Block %s"), GetStringOf(pLine->eCommand), GetStringOf(pBlock->eType));
		}
		pLine = pLine->pNext;
	}
	return NULL;
}

static KmlLine* InitLcd(KmlBlock* pBlock)
{
	KmlLine* pLine = pBlock->pFirstLine;
	while (pLine)
	{
		switch (pLine->eCommand)
		{
		case TOK_OFFSET:
			nLcdX = (UINT) pLine->nParam[0];
			nLcdY = (UINT) pLine->nParam[1];
			break;
		case TOK_ZOOM:
			if ((nGdiXZoom = (UINT) pLine->nParam[0]) == 0)
				nGdiXZoom = 1;				// default zoom

			// search for memory DC zoom (1-4)
			for (nLcdZoom = 4; (nGdiXZoom % nLcdZoom) != 0; --nLcdZoom) { };
			_ASSERT(nLcdZoom > 0);			// because (nGdiXZoom % 1) == 0
			nGdiXZoom /= nLcdZoom;			// remainder is GDI zoom
			nGdiYZoom = nGdiXZoom;
			break;
		case TOK_ZOOMXY:
			if ((nGdiXZoom = (UINT) pLine->nParam[0]) == 0)
				nGdiXZoom = 1;				// default zoom
			if ((nGdiYZoom = (UINT) pLine->nParam[1]) == 0)
				nGdiYZoom = 1;				// default zoom

			// search for memory DC zoom (1-4)
			for (nLcdZoom = 4; ((nGdiXZoom % nLcdZoom) | (nGdiYZoom % nLcdZoom)) != 0 ; --nLcdZoom) { };
			_ASSERT(nLcdZoom > 0);			// because (nGdiYZoom % 1) == 0 && (nGdiYZoom % 1) == 0
			nGdiXZoom /= nLcdZoom;			// remainder is GDI zoom
			nGdiYZoom /= nLcdZoom;
			break;
		case TOK_COLOR:
			SetLcdColor((UINT) pLine->nParam[0],(UINT) pLine->nParam[1],
						(UINT) pLine->nParam[2],(UINT) pLine->nParam[3]);
			break;
		case TOK_BITMAP:
			if (hAnnunDC != NULL)
			{
				PrintfToLog(_T("Bitmap %s ignored."), (LPCTSTR)pLine->nParam[0]);
				AddToLog(_T("Please put only one Bitmap command in the Lcd block."));
				break;
			}
			if (!CreateAnnunBitmap((LPCTSTR)pLine->nParam[0]))
			{
				PrintfToLog(_T("Cannot load Annunciator Bitmap %s."), (LPCTSTR)pLine->nParam[0]);
				break;
			}
			PrintfToLog(_T("Annunciator Bitmap %s loaded."), (LPCTSTR)pLine->nParam[0]);
			break;
		case TOK_END:
			return pLine;
		default:
			PrintfToLog(_T("Command %s Ignored in Block %s"), GetStringOf(pLine->eCommand), GetStringOf(pBlock->eType));
		}
		pLine = pLine->pNext;
	}
	return NULL;
}

static KmlLine* InitAnnunciator(KmlBlock* pBlock)
{
	KmlLine* pLine = pBlock->pFirstLine;
	UINT nId = pBlock->nId-1;
	if (nId >= ARRAYSIZEOF(pAnnunciator))
	{
		PrintfToLog(_T("Wrong Annunciator Id %i"), nId);
		return NULL;
	}
	nAnnunciators++;
	while (pLine)
	{
		switch (pLine->eCommand)
		{
		case TOK_OFFSET:
			pAnnunciator[nId].nOx = (UINT) pLine->nParam[0];
			pAnnunciator[nId].nOy = (UINT) pLine->nParam[1];
			break;
		case TOK_DOWN:
			pAnnunciator[nId].nDx = (UINT) pLine->nParam[0];
			pAnnunciator[nId].nDy = (UINT) pLine->nParam[1];
			break;
		case TOK_SIZE:
			pAnnunciator[nId].nCx = (UINT) pLine->nParam[0];
			pAnnunciator[nId].nCy = (UINT) pLine->nParam[1];
			break;
		case TOK_END:
			return pLine;
		default:
			PrintfToLog(_T("Command %s Ignored in Block %s"), GetStringOf(pLine->eCommand), GetStringOf(pBlock->eType));
		}
		pLine = pLine->pNext;
	}
	return NULL;
}

static VOID InitButton(KmlBlock* pBlock)
{
	KmlLine* pLine = pBlock->pFirstLine;
	UINT nLevel = 0;

	_ASSERT(ARRAYSIZEOF(pButton) == 256);	// adjust warning message
	if (nButtons >= ARRAYSIZEOF(pButton))
	{
		AddToLog(_T("Only the first 256 buttons will be defined."));
		return;
	}
	pButton[nButtons].nId = pBlock->nId;
	pButton[nButtons].bDown = FALSE;
	pButton[nButtons].nType = 0;			// default: user defined button
	while (pLine)
	{
		if (nLevel)
		{
			if (IsBlock(pLine->eCommand)) nLevel++;
			if (pLine->eCommand == TOK_END) nLevel--;
			pLine = pLine->pNext;
			continue;
		}
		if (IsBlock(pLine->eCommand)) nLevel++;
		switch (pLine->eCommand)
		{
		case TOK_TYPE:
			pButton[nButtons].nType = (UINT) pLine->nParam[0];
			break;
		case TOK_OFFSET:
			pButton[nButtons].nOx = (UINT) pLine->nParam[0];
			pButton[nButtons].nOy = (UINT) pLine->nParam[1];
			break;
		case TOK_DOWN:
			pButton[nButtons].nDx = (UINT) pLine->nParam[0];
			pButton[nButtons].nDy = (UINT) pLine->nParam[1];
			break;
		case TOK_SIZE:
			pButton[nButtons].nCx = (UINT) pLine->nParam[0];
			pButton[nButtons].nCy = (UINT) pLine->nParam[1];
			break;
		case TOK_OUTIN:
			pButton[nButtons].nOut = (UINT) pLine->nParam[0];
			pButton[nButtons].nIn  = (UINT) pLine->nParam[1];
			break;
		case TOK_ONDOWN:
			pButton[nButtons].pOnDown = pLine;
			break;
		case TOK_ONUP:
			pButton[nButtons].pOnUp = pLine;
			break;
		case TOK_NOHOLD:
			pButton[nButtons].dwFlags &= ~(BUTTON_VIRTUAL);
			pButton[nButtons].dwFlags |= BUTTON_NOHOLD;
			break;
		case TOK_VIRTUAL:
			pButton[nButtons].dwFlags &= ~(BUTTON_NOHOLD);
			pButton[nButtons].dwFlags |= BUTTON_VIRTUAL;
			break;
		default:
			PrintfToLog(_T("Command %s Ignored in Block %s %i"), GetStringOf(pLine->eCommand), GetStringOf(pBlock->eType), pBlock->nId);
		}
		pLine = pLine->pNext;
	}
	if (nLevel)
		PrintfToLog(_T("%i Open Block(s) in Block %s %i"), nLevel, GetStringOf(pBlock->eType), pBlock->nId);
	nButtons++;
	return;
}



//################
//#
//#    Execution
//#
//################

static KmlLine* SkipLines(KmlLine* pLine, TokenId eCommand)
{
	UINT nLevel = 0;
	while (pLine)
	{
		if (IsBlock(pLine->eCommand)) nLevel++;
		if (pLine->eCommand == eCommand)
		{
			// found token, return command behind token
			if (nLevel == 0) return pLine->pNext;
		}
		if (pLine->eCommand == TOK_END)
		{
			if (nLevel)
				nLevel--;
			else
				break;
		}
		pLine = pLine->pNext;
	}
	return pLine;
}

static KmlLine* If(KmlLine* pLine, BOOL bCondition)
{
	pLine = pLine->pNext;
	if (bCondition)
	{
		while (pLine)
		{
			if (pLine->eCommand == TOK_END)
			{
				pLine = pLine->pNext;
				break;
			}
			if (pLine->eCommand == TOK_ELSE)
			{
				pLine = SkipLines(pLine, TOK_END);
				break;
			}
			pLine = RunLine(pLine);
		}
	}
	else
	{
		pLine = SkipLines(pLine, TOK_ELSE);
		while (pLine)
		{
			if (pLine->eCommand == TOK_END)
			{
				pLine = pLine->pNext;
				break;
			}
			pLine = RunLine(pLine);
		}
	}
	return pLine;
}

static KmlLine* RunLine(KmlLine* pLine)
{
	BYTE byVal;

	switch (pLine->eCommand)
	{
	case TOK_MAP:
		if (byVKeyMap[pLine->nParam[0]&0xFF]&1)
			PressButtonById((UINT) pLine->nParam[1]);
		else
			ReleaseButtonById((UINT) pLine->nParam[1]);
		break;
	case TOK_PRESS:
		PressButtonById((UINT) pLine->nParam[0]);
		break;
	case TOK_RELEASE:
		ReleaseButtonById((UINT) pLine->nParam[0]);
		break;
	case TOK_MENUITEM:
		PostMessage(hWnd, WM_COMMAND, 0x19C40+(pLine->nParam[0]&0xFF), 0);
		break;
	case TOK_SYSITEM:
		PostMessage(hWnd, WM_SYSCOMMAND, pLine->nParam[0], 0);
		break;
	case TOK_SETFLAG:
		nKMLFlags |= 1<<(pLine->nParam[0]&0x1F);
		break;
	case TOK_RESETFLAG:
		nKMLFlags &= ~(1<<(pLine->nParam[0]&0x1F));
		break;
	case TOK_NOTFLAG:
		nKMLFlags ^= 1<<(pLine->nParam[0]&0x1F);
		break;
	case TOK_IFPRESSED:
		return If(pLine,byVKeyMap[pLine->nParam[0]&0xFF]);
	case TOK_IFFLAG:
		return If(pLine,(nKMLFlags>>(pLine->nParam[0]&0x1F))&1);
	case TOK_IFMEM:
		Npeek(&byVal,(DWORD) pLine->nParam[0],1);
		return If(pLine,(byVal & pLine->nParam[1]) == pLine->nParam[2]);
	default:
		break;
	}
	return pLine->pNext;
}



//################
//#
//#    Clean Up
//#
//################

static VOID FreeLines(KmlLine* pLine)
{
	while (pLine)
	{
		KmlLine* pThisLine = pLine;
		UINT i = 0;
		DWORD nParams;
		while (pLexToken[i].nLen)			// search in all token definitions
		{
			// break when token definition found
			if (pLexToken[i].eId == pLine->eCommand) break;
			i++;							// next token definition
		}
		nParams = pLexToken[i].nParams;		// get argument types of command
		i = 0;								// first parameter
		while ((nParams&7))					// argument left
		{
			if ((nParams&7) == TYPE_STRING)	// string type
			{
				free((LPVOID)pLine->nParam[i]);
			}
			i++;							// incr. parameter buffer index
			nParams >>= 3;					// next argument type
		}
		pLine = pLine->pNext;				// get next line
		free(pThisLine);
	}
	return;
}

VOID FreeBlocks(KmlBlock* pBlock)
{
	while (pBlock)
	{
		KmlBlock* pThisBlock = pBlock;
		pBlock = pBlock->pNext;
		FreeLines(pThisBlock->pFirstLine);
		free(pThisBlock);
	}
	return;
}

VOID KillKML(VOID)
{
	if ((nState==SM_RUN)||(nState==SM_SLEEP))
	{
		AbortMessage(_T("FATAL: KillKML while emulator is running !!!"));
		SwitchToState(SM_RETURN);
		DestroyWindow(hWnd);
	}
	UnmapRom();
	DestroyLcdBitmap();
	DestroyAnnunBitmap();
	DestroyMainBitmap();
	if (hPalette)
	{
		if (hWindowDC) SelectPalette(hWindowDC, hOldPalette, FALSE);
		VERIFY(DeleteObject(hPalette));
		hPalette = NULL;
	}
	if (hRgn != NULL)						// region defined
	{
		if (hWnd != NULL)					// window available
		{
			EnterCriticalSection(&csGDILock);
			{
				// deletes the region resource
				SetWindowRgn(hWnd,NULL,FALSE);
				GdiFlush();
			}
			LeaveCriticalSection(&csGDILock);
		}
		hRgn = NULL;
	}
	LoadIconDefault();
	bClicking = FALSE;
	uButtonClicked = 0;
	FreeBlocks(pKml);
	pKml = NULL;
	nButtons = 0;
	nScancodes = 0;
	nAnnunciators = 0;
	bDebug = TRUE;
	wKeybLocId = 0;
	bLocaleInc = FALSE;
	nKMLFlags = 0;
	ZeroMemory(pButton, sizeof(pButton));
	ZeroMemory(pAnnunciator, sizeof(pAnnunciator));
	ZeroMemory(pVKey, sizeof(pVKey));
	ZeroMemory(byVKeyMap, sizeof(byVKeyMap));
	ClearLog();
	nBackgroundX = 0;
	nBackgroundY = 0;
	nBackgroundW = 256;
	nBackgroundH = 0;
	nLcdZoom = 1;
	nGdiXZoom = 1;
	nGdiYZoom = 1;
	dwTColor = (DWORD) -1;
	dwTColorTol = 0;
	nScaleMul = 0;
	nScaleDiv = 0;
	cCurrentRomType = 0;
	nCurrentClass = 0;
	ResizeWindow();
	return;
}



//################
//#
//#    Extract Keyword's Parameters
//#
//################

static LPCTSTR GetStringParam(KmlBlock* pBlock, TokenId eBlock, TokenId eCommand, UINT nParam)
{
	while (pBlock)
	{
		if (pBlock->eType == eBlock)
		{
			KmlLine* pLine = pBlock->pFirstLine;
			while (pLine)
			{
				if (pLine->eCommand == eCommand)
				{
					return (LPCTSTR) pLine->nParam[nParam];
				}
				pLine = pLine->pNext;
			}
		}
		pBlock = pBlock->pNext;
	}
	return NULL;
}

static DWORD GetIntegerParam(KmlBlock* pBlock, TokenId eBlock, TokenId eCommand, UINT nParam)
{
	while (pBlock)
	{
		if (pBlock->eType == eBlock)
		{
			KmlLine* pLine = pBlock->pFirstLine;
			while (pLine)
			{
				if (pLine->eCommand == eCommand)
				{
					return (DWORD) pLine->nParam[nParam];
				}
				pLine = pLine->pNext;
			}
		}
		pBlock = pBlock->pNext;
	}
	return 0;
}



//################
//#
//#    Buttons
//#
//################

static UINT iSqrt(UINT nNumber)				// integer y=sqrt(x) function
{
	UINT b, t;

	b = t = nNumber;

	if (nNumber > 0)
	{
		do
		{
			b = t;
			t = (t + nNumber / t) / 2;		// Heron's method
		}
		while (t < b);
	}
	return b;
}

static VOID AdjustPixel(LPBYTE pbyPixel, BYTE byOffset)
{
	INT i = 3;								// BGR colors

	while (--i >= 0)
	{
		WORD wColor = (WORD) *pbyPixel + byOffset;
		// jumpless saturation to 0xFF
		wColor = -(wColor >> 8) | (BYTE) wColor;
		*pbyPixel++ = (BYTE) wColor;
	}
	return;
}

// draw transparent circle button type
static __inline VOID TransparentCircle(UINT nOx, UINT nOy, UINT nCx, UINT nCy)
{
	#define HIGHADJ 0x80					// color incr. at center
	#define LOWADJ  0x10					// color incr. at border

	BITMAPINFO bmi;
	HDC     hMemDC;
	HBITMAP hMemBitMap;
	LPBYTE  pbyPixels;						// BMP data

	UINT x, y, cx, cy, r, rr, rrc;

	r = min(nCx,nCy) / 2;					// radius
	if (r < 2) return;						// radius 2 pixel minimum

	// create memory copy of button rectangle
	ZeroMemory(&bmi,sizeof(bmi));
	bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biWidth = (LONG) nCx;
	bmi.bmiHeader.biHeight = (LONG) nCy;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;			// use 32 bit bitmap for easier buffer calculation
	bmi.bmiHeader.biCompression = BI_RGB;
	VERIFY(hMemBitMap = CreateDIBSection(hWindowDC,
										 &bmi,
										 DIB_RGB_COLORS,
										 (VOID **)&pbyPixels,
										 NULL,
										 0));
	if (hMemBitMap == NULL) return;

	hMemDC = CreateCompatibleDC(hWindowDC);
	hMemBitMap = (HBITMAP) SelectObject(hMemDC,hMemBitMap);
	BitBlt(hMemDC, 0, 0, nCx, nCy, hWindowDC, nOx, nOy, SRCCOPY);

	cx = nCx / 2;							// x-center coordinate
	cy = nCy / 2;							// y-center coordinate

	rr  = r * r;							// calculate r^2
	rrc = (r-1) * (r-1);					// calculate (r-1)^2 for color steps

	// y-rows of circle
	for (y = 0; y < r; ++y)
	{
		UINT yy = y * y;					// calculate y^2

		// x-columns of circle
		UINT nXWidth = iSqrt(rr-yy);

		for (x = 0; x < nXWidth; ++x)
		{
			// color offset, sqrt(x*x+y*y) < r !!!
			BYTE byOff = HIGHADJ - (BYTE) (iSqrt((x*x+yy) * (HIGHADJ-LOWADJ)*(HIGHADJ-LOWADJ) / rrc));

			AdjustPixel(pbyPixels + (((cy+y) * bmi.bmiHeader.biWidth + (cx+x)) << 2), byOff);

			if (x != 0)
			{
				AdjustPixel(pbyPixels + (((cy+y) * bmi.bmiHeader.biWidth + (cx-x)) << 2), byOff);
			}

			if (y != 0)
			{
				AdjustPixel(pbyPixels + (((cy-y) * bmi.bmiHeader.biWidth + (cx+x)) << 2), byOff);
			}

			if (x != 0 && y != 0)
			{
				AdjustPixel(pbyPixels + (((cy-y) * bmi.bmiHeader.biWidth + (cx-x)) << 2), byOff);
			}
		}
	}

	// update button area with modified data
	BitBlt(hWindowDC, nOx, nOy, nCx, nCy, hMemDC, 0, 0, SRCCOPY);

	// delete memory bitmap
	VERIFY(DeleteObject(SelectObject(hMemDC,hMemBitMap)));
	DeleteDC(hMemDC);
	return;

	#undef HIGHADJ
	#undef LOWADJ
}

static VOID DrawButton(UINT nId)
{
	UINT x0 = pButton[nId].nOx;
	UINT y0 = pButton[nId].nOy;

	EnterCriticalSection(&csGDILock);		// solving NT GDI problems
	{
		switch (pButton[nId].nType)
		{
		case 0: // bitmap key
			if (pButton[nId].bDown)
			{
				BitBlt(hWindowDC, x0, y0, pButton[nId].nCx, pButton[nId].nCy, hMainDC, pButton[nId].nDx, pButton[nId].nDy, SRCCOPY);
			}
			else
			{
				// update background only
				BitBlt(hWindowDC, x0, y0, pButton[nId].nCx, pButton[nId].nCy, hMainDC, x0, y0, SRCCOPY);
			}
			break;
		case 1: // shift key to right down
			if (pButton[nId].bDown)
			{
				UINT x1 = x0+pButton[nId].nCx-1;
				UINT y1 = y0+pButton[nId].nCy-1;
				BitBlt(hWindowDC, x0+3,y0+3,pButton[nId].nCx-5,pButton[nId].nCy-5,hMainDC,x0+2,y0+2,SRCCOPY);
				SelectObject(hWindowDC, GetStockObject(BLACK_PEN));
				MoveToEx(hWindowDC, x0, y0, NULL); LineTo(hWindowDC, x1, y0);
				MoveToEx(hWindowDC, x0, y0, NULL); LineTo(hWindowDC, x0, y1);
				SelectObject(hWindowDC, GetStockObject(WHITE_PEN));
				MoveToEx(hWindowDC, x1, y0, NULL); LineTo(hWindowDC, x1,   y1);
				MoveToEx(hWindowDC, x0, y1, NULL); LineTo(hWindowDC, x1+1, y1);
			}
			else
			{
				BitBlt(hWindowDC, x0, y0, pButton[nId].nCx, pButton[nId].nCy, hMainDC, x0, y0, SRCCOPY);
			}
			break;
		case 2: // do nothing
			break;
		case 3: // invert key color, even in display
			if (pButton[nId].bDown)
			{
				PatBlt(hWindowDC, x0, y0, pButton[nId].nCx, pButton[nId].nCy, DSTINVERT);
			}
			else
			{
				RECT Rect;
				Rect.left = x0 - nBackgroundX;
				Rect.top  = y0 - nBackgroundY;
				Rect.right  = Rect.left + pButton[nId].nCx;
				Rect.bottom = Rect.top + pButton[nId].nCy;
				InvalidateRect(hWnd, &Rect, FALSE);	// call WM_PAINT for background and display redraw
			}
			break;
		case 4: // bitmap key, even in display
			if (pButton[nId].bDown)
			{
				// update background only
				BitBlt(hWindowDC, x0, y0, pButton[nId].nCx, pButton[nId].nCy, hMainDC, x0, y0, SRCCOPY);
			}
			else
			{
				RECT Rect;
				Rect.left = x0 - nBackgroundX;
				Rect.top  = y0 - nBackgroundY;
				Rect.right  = Rect.left + pButton[nId].nCx;
				Rect.bottom = Rect.top + pButton[nId].nCy;
				InvalidateRect(hWnd, &Rect, FALSE);	// call WM_PAINT for background and display redraw
			}
			break;
		case 5: // transparent circle
			if (pButton[nId].bDown)
			{
				TransparentCircle(x0, y0, pButton[nId].nCx, pButton[nId].nCy);
			}
			else
			{
				// update background only
				BitBlt(hWindowDC, x0, y0, pButton[nId].nCx, pButton[nId].nCy, hMainDC, x0, y0, SRCCOPY);
			}
			break;
		default: // black key, default drawing on illegal types
			if (pButton[nId].bDown)
			{
				PatBlt(hWindowDC, x0, y0, pButton[nId].nCx, pButton[nId].nCy, BLACKNESS);
			}
			else
			{
				// update background only
				BitBlt(hWindowDC, x0, y0, pButton[nId].nCx, pButton[nId].nCy, hMainDC, x0, y0, SRCCOPY);
			}
		}
		GdiFlush();
	}
	LeaveCriticalSection(&csGDILock);
	return;
}

static VOID PressButton(UINT nId)
{
	if (!pButton[nId].bDown)				// button not pressed
	{
		pButton[nId].bDown = TRUE;
		DrawButton(nId);
		if (pButton[nId].nIn)
		{
			KeyboardEvent(TRUE,pButton[nId].nOut,pButton[nId].nIn);
		}
		else
		{
			KmlLine* pLine = pButton[nId].pOnDown;
			while ((pLine)&&(pLine->eCommand!=TOK_END))
			{
				pLine = RunLine(pLine);
			}
		}
	}
	return;
}

static VOID ReleaseButton(UINT nId)
{
	if (pButton[nId].bDown)					// button not released
	{
		pButton[nId].bDown = FALSE;
		DrawButton(nId);
		if (pButton[nId].nIn)
		{
			KeyboardEvent(FALSE,pButton[nId].nOut,pButton[nId].nIn);
		}
		else
		{
			KmlLine* pLine = pButton[nId].pOnUp;
			while ((pLine)&&(pLine->eCommand!=TOK_END))
			{
				pLine = RunLine(pLine);
			}
		}
	}
	return;
}

static VOID PressButtonById(UINT nId)
{
	UINT i;
	for (i=0; i<nButtons; i++)
	{
		if (nId == pButton[i].nId)
		{
			PressButton(i);
			return;
		}
	}
	return;
}

static VOID ReleaseButtonById(UINT nId)
{
	UINT i;
	for (i=0; i<nButtons; i++)
	{
		if (nId == pButton[i].nId)
		{
			ReleaseButton(i);
			return;
		}
	}
	return;
}

static VOID ReleaseAllButtons(VOID)			// release all buttons
{
	UINT i;
	for (i=0; i<nButtons; i++)				// scan all buttons
	{
		if (pButton[i].bDown)				// button pressed
			ReleaseButton(i);				// release button
	}

	bKeyPressed = FALSE;					// key not pressed
	bClicking = FALSE;						// var uButtonClicked not valid (no virtual or nohold key)
	uButtonClicked = 0;						// set var to default
}

VOID ReloadButtons(BYTE *Keyboard_Row, UINT nSize)
{
	UINT i;
	bKeyPressed = FALSE;					// no key pressed
	for (i=0; i<nButtons; i++)				// scan all buttons
	{
		if (pButton[i].nOut < nSize)		// valid out code
		{
			if (pButton[i].nIn == 0x8000)	// ON key
			{
				// get state of ON button from interrupt line
				pButton[i].bDown = (Chipset.IR15X != 0);
			}
			else
			{
				// get state of button from keyboard matrix
				pButton[i].bDown = ((Keyboard_Row[pButton[i].nOut] & pButton[i].nIn) != 0);
			}

			// any key pressed?
			bKeyPressed = bKeyPressed || pButton[i].bDown;
		}
	}
}

VOID RefreshButtons(RECT *rc)
{
	UINT i;
	for (i=0; i<nButtons; i++)
	{
		if (   pButton[i].bDown
			&& rc->right  >  (LONG) (pButton[i].nOx)
			&& rc->bottom >  (LONG) (pButton[i].nOy)
			&& rc->left   <= (LONG) (pButton[i].nOx + pButton[i].nCx)
			&& rc->top    <= (LONG) (pButton[i].nOy + pButton[i].nCy))
		{
			// on button type 3 and 5 clear complete key area before drawing
			if (pButton[i].nType == 3 || pButton[i].nType == 5)
			{
				UINT x0 = pButton[i].nOx;
				UINT y0 = pButton[i].nOy;
				EnterCriticalSection(&csGDILock); // solving NT GDI problems
				{
					BitBlt(hWindowDC, x0, y0, pButton[i].nCx, pButton[i].nCy, hMainDC, x0, y0, SRCCOPY);
					GdiFlush();
				}
				LeaveCriticalSection(&csGDILock);
			}
			DrawButton(i);					// redraw pressed button
		}
	}
	return;
}


//################
//#
//#    Annunciators
//#
//################

VOID DrawAnnunciator(UINT nId, BOOL bOn)
{
	HDC  hDC;
	UINT nSx,nSy;

	--nId;									// zero based ID
	if (nId >= ARRAYSIZEOF(pAnnunciator)) return;
	if (bOn)
	{
		hDC = hAnnunDC != NULL ? hAnnunDC : hMainDC;

		nSx = pAnnunciator[nId].nDx;		// position of annunciator
		nSy = pAnnunciator[nId].nDy;
	}
	else
	{
		hDC = hMainDC;

		nSx = pAnnunciator[nId].nOx;		// position of background
		nSy = pAnnunciator[nId].nOy;
	}
	EnterCriticalSection(&csGDILock);		// solving NT GDI problems
	{
		BitBlt(hWindowDC,
			pAnnunciator[nId].nOx, pAnnunciator[nId].nOy,
			pAnnunciator[nId].nCx, pAnnunciator[nId].nCy,
			hDC,
			nSx, nSy,
			SRCCOPY);
		GdiFlush();
	}
	LeaveCriticalSection(&csGDILock);
	return;
}



//################
//#
//#    Mouse
//#
//################

static BOOL ClipButton(UINT x, UINT y, UINT nId)
{
	x += nBackgroundX;						// source display offset
	y += nBackgroundY;

	return (pButton[nId].nOx<=x)
		&& (pButton[nId].nOy<=y)
		&&(x<(pButton[nId].nOx+pButton[nId].nCx))
		&&(y<(pButton[nId].nOy+pButton[nId].nCy));
}

BOOL MouseIsButton(DWORD x, DWORD y)
{
	UINT i;
	for (i = 0; i < nButtons; i++)			// scan all buttons
	{
		if (ClipButton(x,y,i))				// cursor over button?
		{
			return TRUE;
		}
	}
	return FALSE;
}

VOID MouseButtonDownAt(UINT nFlags, DWORD x, DWORD y)
{
	UINT i;
	for (i=0; i<nButtons; i++)
	{
		if (ClipButton(x,y,i))
		{
			if (pButton[i].dwFlags&BUTTON_NOHOLD)
			{
				if (nFlags&MK_LBUTTON)		// use only with left mouse button
				{
					bClicking = TRUE;
					uButtonClicked = i;
					pButton[i].bDown = TRUE;
					DrawButton(i);
				}
				return;
			}
			if (pButton[i].dwFlags&BUTTON_VIRTUAL)
			{
				if (!(nFlags&MK_LBUTTON))	// use only with left mouse button
					return;
				bClicking = TRUE;
				uButtonClicked = i;
			}
			bKeyPressed = TRUE;				// key pressed
			uLastKeyPressed = i;			// save pressed key
			PressButton(i);
			return;
		}
	}
}

VOID MouseButtonUpAt(UINT nFlags, DWORD x, DWORD y)
{
	UINT i;
	if (bKeyPressed)						// emulator key pressed
	{
		ReleaseAllButtons();				// release all buttons
		return;
	}
	for (i=0; i<nButtons; i++)
	{
		if (ClipButton(x,y,i))
		{
			if ((bClicking)&&(uButtonClicked != i)) break;
			ReleaseButton(i);
			break;
		}
	}
	bClicking = FALSE;
	uButtonClicked = 0;
	return;
	UNREFERENCED_PARAMETER(nFlags);
}

VOID MouseMovesTo(UINT nFlags, DWORD x, DWORD y)
{
	// set cursor
	_ASSERT(hCursorArrow != NULL && hCursorHand != NULL);
	// make sure that class cursor is NULL
	_ASSERT(GetClassLongPtr(hWnd,GCLP_HCURSOR) == 0);

	// cursor over button -> hand cursor else normal arrow cursor
	SetCursor(MouseIsButton(x,y) ? hCursorHand : hCursorArrow);

	if (!(nFlags&MK_LBUTTON)) return;						// left mouse key not pressed -> quit
	if (bKeyPressed && !ClipButton(x,y,uLastKeyPressed))	// not on last pressed key
		ReleaseAllButtons();								// release all buttons
	if (!bClicking) return;									// normal emulation key -> quit

	if (pButton[uButtonClicked].dwFlags&BUTTON_NOHOLD)
	{
		if (ClipButton(x,y, uButtonClicked) != pButton[uButtonClicked].bDown)
		{
			pButton[uButtonClicked].bDown = !pButton[uButtonClicked].bDown;
			DrawButton(uButtonClicked);
		}
		return;
	}
	if (pButton[uButtonClicked].dwFlags&BUTTON_VIRTUAL)
	{
		if (!ClipButton(x,y, uButtonClicked))
		{
			ReleaseButton(uButtonClicked);
			bClicking = FALSE;
			uButtonClicked = 0;
		}
		return;
	}
	return;
}



//################
//#
//#    Keyboard
//#
//################

VOID RunKey(BYTE nId, BOOL bPressed)
{
	if (pVKey[nId])
	{
		KmlLine* pLine = pVKey[nId]->pFirstLine;
		byVKeyMap[nId] = (BYTE) bPressed;
		while (pLine) pLine = RunLine(pLine);
	}
	else
	{
		if (bDebug&&bPressed)
		{
			TCHAR szTemp[128];
			wsprintf(szTemp,_T("Scancode %i"),nId);
			InfoMessage(szTemp);
		}
	}
	return;
}



//################
//#
//#    Macro player
//#
//################

VOID PlayKey(UINT nOut, UINT nIn, BOOL bPressed)
{
	// scan from last buttons because LCD buttons mostly defined first
	INT i = nButtons;
	while (--i >= 0)
	{
		if (pButton[i].nOut == nOut && pButton[i].nIn == nIn)
		{
			if (bPressed)
				PressButton(i);
			else
				ReleaseButton(i);
			return;
		}
	}
	return;
}



//################
//#
//#    Load and Initialize Script
//#
//################

static VOID ResizeMainBitmap(INT nMul, INT nDiv)
{
	if (nMul * nDiv > 0)					// resize main picture
	{
		BITMAP Bitmap;
		int    nMode;
		INT    nWidth,nHeight;
		UINT   i;

		// update graphic
		nBackgroundX = MulDiv(nBackgroundX,nMul,nDiv);
		nBackgroundY = MulDiv(nBackgroundY,nMul,nDiv);
		nBackgroundW = MulDiv(nBackgroundW,nMul,nDiv);
		nBackgroundH = MulDiv(nBackgroundH,nMul,nDiv);
		nLcdX = MulDiv(nLcdX,nMul,nDiv);
		nLcdY = MulDiv(nLcdY,nMul,nDiv);
		nGdiXZoom = MulDiv(nGdiXZoom * nLcdZoom,nMul,nDiv);
		nGdiYZoom = MulDiv(nGdiYZoom * nLcdZoom,nMul,nDiv);

		// search for memory DC zoom (1-4)
		for (nLcdZoom = 4; ((nGdiXZoom % nLcdZoom) | (nGdiYZoom % nLcdZoom)) != 0 ; --nLcdZoom) { };
		_ASSERT(nLcdZoom > 0);				// because (nGdiYZoom % 1) == 0 && (nGdiYZoom % 1) == 0
		nGdiXZoom /= nLcdZoom;				// remainder is GDI zoom
		nGdiYZoom /= nLcdZoom;

		// update script coordinates (buttons)
		for (i = 0; i < nButtons; ++i)
		{
			pButton[i].nOx = (UINT) (pButton[i].nOx * nMul / nDiv);
			pButton[i].nOy = (UINT) (pButton[i].nOy * nMul / nDiv);
			pButton[i].nDx = (UINT) (pButton[i].nDx * nMul / nDiv);
			pButton[i].nDy = (UINT) (pButton[i].nDy * nMul / nDiv);
			pButton[i].nCx = (UINT) (pButton[i].nCx * nMul / nDiv);
			pButton[i].nCy = (UINT) (pButton[i].nCy * nMul / nDiv);
		}

		// update script coordinates (annunciators)
		for (i = 0; i < ARRAYSIZEOF(pAnnunciator); ++i)
		{
			// translate offset
			pAnnunciator[i].nOx = (UINT) (pAnnunciator[i].nOx * nMul / nDiv);
			pAnnunciator[i].nOy = (UINT) (pAnnunciator[i].nOy * nMul / nDiv);

			if (hAnnunDC == NULL)			// no external annunciator bitmap
			{
				// translate down (with rounding)
				pAnnunciator[i].nDx = (UINT) MulDiv(pAnnunciator[i].nDx,nMul,nDiv);
				pAnnunciator[i].nDy = (UINT) MulDiv(pAnnunciator[i].nDy,nMul,nDiv);
				// translate size (with rounding)
				pAnnunciator[i].nCx = (UINT) MulDiv(pAnnunciator[i].nCx,nMul,nDiv);
				pAnnunciator[i].nCy = (UINT) MulDiv(pAnnunciator[i].nCy,nMul,nDiv);
			}
		}

		EnterCriticalSection(&csGDILock);	// solving NT GDI problems
		{
			// get bitmap size
			GetObject(GetCurrentObject(hMainDC,OBJ_BITMAP),sizeof(Bitmap),&Bitmap);

			// resulting bitmap size
			nWidth  = MulDiv(Bitmap.bmWidth,nMul,nDiv);
			nHeight = MulDiv(Bitmap.bmHeight,nMul,nDiv);

			VERIFY(nMode = SetStretchBltMode(hMainDC,HALFTONE));

			if (nMul <= nDiv)				// shrinking bitmap
			{
				VERIFY(StretchBlt(
					hMainDC,0,0,nWidth,nHeight,
					hMainDC,0,0,Bitmap.bmWidth,Bitmap.bmHeight,
					SRCCOPY));
			}
			else							// expanding bitmap
			{
				// bitmap with new size
				HDC hMemDC = CreateCompatibleDC(hMainDC);
				HBITMAP hMainBitMap = CreateCompatibleBitmap(hMainDC,nWidth,nHeight);
				HBITMAP hMemBitMap = (HBITMAP) SelectObject(hMemDC,SelectObject(hMainDC,hMainBitMap));

				VERIFY(StretchBlt(
					hMainDC,0,0,nWidth,nHeight,
					hMemDC,0,0,Bitmap.bmWidth,Bitmap.bmHeight,
					SRCCOPY));

				// delete original bitmap
				VERIFY(DeleteObject(SelectObject(hMemDC,hMemBitMap)));
				DeleteDC(hMemDC);
			}

			VERIFY(SetStretchBltMode(hMainDC,nMode));

			GdiFlush();
		}
		LeaveCriticalSection(&csGDILock);
		ResizeWindow();
	}
	return;
}

static KmlBlock* LoadKMLGlobal(LPCTSTR szFilename)
{
	HANDLE    hFile;
	LPTSTR    lpBuf;
	KmlBlock* pBlock;
	TokenId   eToken;

	SetCurrentDirectory(szEmuDirectory);
	hFile = CreateFile(szFilename,
					   GENERIC_READ,
					   FILE_SHARE_READ,
					   NULL,
					   OPEN_EXISTING,
					   FILE_FLAG_SEQUENTIAL_SCAN,
					   NULL);
	SetCurrentDirectory(szCurrentDirectory);
	if (hFile == INVALID_HANDLE_VALUE) return NULL;
	if ((lpBuf = MapKMLFile(hFile)) == NULL)
		return NULL;

	InitLex(lpBuf);
	pBlock = NULL;
	while ((eToken = Lex(LEX_BLOCK)) != TOK_NONE)
	{
		if (eToken == TOK_GLOBAL)
		{
			pBlock = ParseBlock(TRUE,eToken);
			if (pBlock) pBlock->pNext = NULL;
			break;
		}
		if (eToken == TOK_STRING)
		{
			free(szLexString);
			szLexString = NULL;
		}
	}
	CleanLex();
	ClearLog();
	free(lpBuf);
	return pBlock;
}

BOOL InitKML(LPCTSTR szFilename, BOOL bNoLog)
{
	TCHAR     szKLID[KL_NAMELENGTH];
	HANDLE    hFile;
	LPTSTR    lpBuf;
	KmlBlock* pBlock;
	BOOL      bOk = FALSE;

	KillKML();

	// get current keyboard layout input locale
	if (GetKeyboardLayoutName(szKLID))
	{
		wKeybLocId = (WORD) _tcstoul(szKLID,NULL,16);
	}

	nBlocksIncludeLevel = 0;
	PrintfToLog(_T("Reading %s"), szFilename);
	SetCurrentDirectory(szEmuDirectory);
	hFile = CreateFile(szFilename,
					   GENERIC_READ,
					   FILE_SHARE_READ,
					   NULL,
					   OPEN_EXISTING,
					   FILE_FLAG_SEQUENTIAL_SCAN,
					   NULL);
	SetCurrentDirectory(szCurrentDirectory);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		AddToLog(_T("Error while opening the file."));
		goto quit;
	}
	if ((lpBuf = MapKMLFile(hFile)) == NULL)
		goto quit;

	InitLex(lpBuf);
	pKml = ParseBlocks(TRUE,				// include blocks
					   FALSE);				// keyword "End" is invalid
	CleanLex();

	free(lpBuf);
	if (pKml == NULL) goto quit;

	pBlock = pKml;
	while (pBlock)
	{
		switch (pBlock->eType)
		{
		case TOK_BUTTON:
			InitButton(pBlock);
			break;
		case TOK_SCANCODE:
			nScancodes++;
			pVKey[pBlock->nId] = pBlock;
			break;
		case TOK_ANNUNCIATOR:
			InitAnnunciator(pBlock);
			break;
		case TOK_GLOBAL:
			InitGlobal(pBlock);
			break;
		case TOK_LCD:
			InitLcd(pBlock);
			break;
		case TOK_BACKGROUND:
			InitBackground(pBlock);
			break;
		default:
			PrintfToLog(_T("Block %s Ignored."), GetStringOf(pBlock->eType));
			pBlock = pBlock->pNext;
		}
		pBlock = pBlock->pNext;
	}

	if (!isModelValid(cCurrentRomType))
	{
		AddToLog(_T("This KML Script doesn't specify a valid model."));
		goto quit;
	}
	if (pbyRom == NULL)
	{
		AddToLog(_T("This KML Script doesn't specify the ROM to use, or the ROM could not be loaded."));
		goto quit;
	}
	if (bRomCrcCorrection)					// ROM CRC correction enabled
	{
		AddToLog(_T("Rebuild the ROM CRC."));
		RebuildRomCrc();					// rebuild the ROM CRC's
	}
	if (hMainDC == NULL)
	{
		AddToLog(_T("This KML Script doesn't specify the background bitmap, or bitmap could not be loaded."));
		goto quit;
	}
	if (!CrcRom(&wRomCrc))					// build patched ROM fingerprint and check for unpacked data
	{
		AddToLog(_T("Error, packed ROM image detected."));
		goto quit;
	}
	if (CheckForBeepPatch())				// check if ROM contain beep patches
	{
		AddToLog(_T("Error, ROM beep patch detected. Remove beep patches please."));
		goto quit;
	}

	ResizeMainBitmap(nScaleMul,nScaleDiv);	// resize main picture
	CreateLcdBitmap();

	PrintfToLog(_T("%i Buttons Defined"), nButtons);
	PrintfToLog(_T("%i Scancodes Defined"), nScancodes);
	PrintfToLog(_T("%i Annunciators Defined"), nAnnunciators);
	PrintfToLog(_T("Keyboard Locale: %i"), wKeybLocId);

	bOk = TRUE;

quit:
	if (bOk)
	{
		// HP38G/HP39G(+|s)/HP40G(s) have no object loading, ignore   // CdB for HP: add apples
		DragAcceptFiles(hWnd,cCurrentRomType != '6' && cCurrentRomType != 'A' && cCurrentRomType != 'E' && cCurrentRomType != 'P');

		if (!bNoLog)
		{
			AddToLog(_T("Press Ok to Continue."));
			if (bAlwaysDisplayLog&&(!DisplayKMLLog(bOk)))
			{
				KillKML();
				return FALSE;
			}
		}
	}
	else
	{
		AddToLog(_T("Press Cancel to Abort."));
		if (!DisplayKMLLog(bOk))
		{
			KillKML();
			return FALSE;
		}
	}

	ResizeWindow();
	ClearLog();
	return bOk;
}
