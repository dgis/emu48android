/*
 *   debugger.c
 *
 *   This file is part of Emu48
 *
 *   Copyright (C) 1999 Christoph Gießelink
 *
 */
#include "pch.h"
#include "resource.h"
#include "Emu48.h"
#include "Opcodes.h"
#include "ops.h"
#include "color.h"
#include "disrpl.h"
#include "debugger.h"

#define MAXCODELINES     15					// number of lines in code window
#define MAXMEMLINES       6					// number of lines in memory window
#define MAXMEMITEMS      16					// number of address items in a memory window line
#define MAXBREAKPOINTS  256					// max. number of breakpoints

#define REG_START  IDC_REG_A				// first register in register update table
#define REG_STOP   IDC_MISC_BS				// last register in register update table
#define REG_SIZE   (REG_STOP-REG_START+1)	// size of register update table

// assert for register update
#define _ASSERTREG(r) _ASSERT(r >= REG_START && r <= REG_STOP)

#define WM_UPDATE (WM_USER+0x1000)			// update debugger dialog box

#define MEMWNDMAX (sizeof(nCol) / sizeof(nCol[0]))

#define RT_TOOLBAR MAKEINTRESOURCE(241)		// MFC toolbar resource type

#define CODELABEL  0x80000000				// label in code window

// trace log file modes
enum TRACE_MODE { TRACE_FILE_NEW = 0, TRACE_FILE_APPEND };

typedef struct CToolBarData
{
	WORD wVersion;
	WORD wWidth;
	WORD wHeight;
	WORD wItemCount;
	WORD aItems[1];
} CToolBarData;

typedef struct								// type of breakpoint table
{
	BOOL	bEnable;						// breakpoint enabled
	UINT	nType;							// breakpoint type
	DWORD	dwAddr;							// breakpoint address
} BP_T;

static CONST int nCol[] =
{
	IDC_DEBUG_MEM_COL0, IDC_DEBUG_MEM_COL1, IDC_DEBUG_MEM_COL2, IDC_DEBUG_MEM_COL3,
	IDC_DEBUG_MEM_COL4, IDC_DEBUG_MEM_COL5, IDC_DEBUG_MEM_COL6, IDC_DEBUG_MEM_COL7
};

static CONST TCHAR cHex[] = { _T('0'),_T('1'),_T('2'),_T('3'),
							  _T('4'),_T('5'),_T('6'),_T('7'),
							  _T('8'),_T('9'),_T('A'),_T('B'),
							  _T('C'),_T('D'),_T('E'),_T('F') };

static INT    nDbgPosX = CW_USEDEFAULT;		// position of debugger window
static INT    nDbgPosY = CW_USEDEFAULT;

static WORD   wBreakpointCount = 0;			// number of breakpoints
static BP_T   sBreakpoint[MAXBREAKPOINTS];	// breakpoint table

static WORD   wBackupBreakpointCount = 0;	// number of breakpoints
static BP_T   sBackupBreakpoint[MAXBREAKPOINTS]; // breakpoint table

static INT    nRplBreak;					// RPL breakpoint

static DWORD  dwAdrLine[MAXCODELINES];		// addresses of disassember lines in code window
static DWORD  dwAdrMem = 0;					// start address of memory window

static UINT   uIDFol = ID_DEBUG_MEM_FNONE;	// follow mode
static DWORD  dwAdrMemFol = 0;				// follow address memory window

static UINT   uIDMap = ID_DEBUG_MEM_MAP;	// current memory view mode

static BOOL   bDbgTrace = FALSE;			// enable trace output
static HANDLE hLogFile = NULL;				// log file handle
static TCHAR  szTraceFilename[MAX_PATH] = _T("trace.log"); // filename for trace file
static UINT   uTraceMode = TRACE_FILE_NEW;	// trace log file mode
static BOOL   bTraceReg = TRUE;				// enable register logging
static BOOL   bTraceMmu = FALSE;			// disable MMU logging
static BOOL   bTraceOpc = TRUE;				// enable opcode logging

static LONG   lCharWidth;					// width of a character (is a fix font)

static HMENU  hMenuCode,hMenuMem,hMenuStack;// handle of context menues
static HWND   hWndToolbar;					// toolbar handle

static DWORD   dwDbgRefCycles;				// cpu cycles counter from last opcode

static CHIPSET OldChipset;					// old chipset content
static BOOL    bRegUpdate[REG_SIZE];		// register update table

static HBITMAP hBmpCheckBox;				// checked and unchecked bitmap
static HFONT   hFontBold;					// bold font for symbol labels in code window

// function prototypes
static BOOL    OnMemFind(HWND hDlg);
static BOOL    OnProfile(HWND hDlg);
static VOID    UpdateRplObjViewWnd(HWND hDlg, DWORD dwAddr);
static BOOL    OnRplObjView(HWND hDlg);
static BOOL    OnSettings(HWND hDlg);
static INT_PTR OnNewValue(LPTSTR lpszValue);
static VOID    OnEnterAddress(HWND hDlg, DWORD *dwValue);
static BOOL    OnEditBreakpoint(HWND hDlg);
static BOOL    OnInfoIntr(HWND hDlg);
static BOOL    OnInfoWoRegister(HWND hDlg);
static VOID    UpdateProfileWnd(HWND hDlg);
static BOOL    OnMemLoadData(HWND hDlg);
static BOOL    OnMemSaveData(HWND hDlg);
static VOID    StartTrace(VOID);
static VOID    StopTrace(VOID);
static VOID    FlushTrace(VOID);
static VOID    OutTrace(VOID);
static BOOL    OnTraceSettings(HWND hDlg);
static BOOL    OnTraceEnable(HWND hDlg);

//################
//#
//#    Low level subroutines
//#
//################

//
// load external rpl symbol table
//
static BOOL LoadSymbTable(VOID)
{
	TCHAR szSymbFilename[MAX_PATH];
	TCHAR szItemname[16];

	wsprintf(szItemname,_T("Symb%c"),(TCHAR) cCurrentRomType);
	ReadSettingsString(_T("Disassembler"),szItemname,_T(""),szSymbFilename,ARRAYSIZEOF(szSymbFilename));

	if (*szSymbFilename == 0)				// no reference table defined
		return FALSE;						// no success

	return RplLoadTable(szSymbFilename);	// load external reference table
}

//
// disable menu keys
//
static VOID DisableMenuKeys(HWND hDlg)
{
	HMENU hMenu = GetMenu(hDlg);

	EnableMenuItem(hMenu,ID_DEBUG_RUN,MF_GRAYED);
	EnableMenuItem(hMenu,ID_DEBUG_RUNCURSOR,MF_GRAYED);
	EnableMenuItem(hMenu,ID_DEBUG_STEP,MF_GRAYED);
	EnableMenuItem(hMenu,ID_DEBUG_STEPOVER,MF_GRAYED);
	EnableMenuItem(hMenu,ID_DEBUG_STEPOUT,MF_GRAYED);
	EnableMenuItem(hMenu,ID_INFO_LASTINSTRUCTIONS,MF_GRAYED);
	EnableMenuItem(hMenu,ID_INFO_WRITEONLYREG,MF_GRAYED);

	SendMessage(hWndToolbar,TB_ENABLEBUTTON,ID_DEBUG_RUN,MAKELONG((FALSE),0));
	SendMessage(hWndToolbar,TB_ENABLEBUTTON,ID_DEBUG_BREAK,MAKELONG((TRUE),0));
	SendMessage(hWndToolbar,TB_ENABLEBUTTON,ID_DEBUG_RUNCURSOR,MAKELONG((FALSE),0));
	SendMessage(hWndToolbar,TB_ENABLEBUTTON,ID_DEBUG_STEP,MAKELONG((FALSE),0));
	SendMessage(hWndToolbar,TB_ENABLEBUTTON,ID_DEBUG_STEPOVER,MAKELONG((FALSE),0));
	SendMessage(hWndToolbar,TB_ENABLEBUTTON,ID_DEBUG_STEPOUT,MAKELONG((FALSE),0));
	return;
}

//
// read edit control and decode content as hex number or if enabled as symbol name
//
static BOOL GetAddr(HWND hDlg,INT nID,DWORD *pdwAddr,DWORD dwMaxAddr,BOOL bSymbEnable)
{
	TCHAR szBuffer[48];
	INT   i;
	BOOL  bSucc = TRUE;

	HWND hWnd = GetDlgItem(hDlg,nID);

	GetWindowText(hWnd,szBuffer,ARRAYSIZEOF(szBuffer));

	if (*szBuffer != 0)
	{
		// if address is not a symbol name decode number
		if (   !bSymbEnable || szBuffer[0] != _T('=')
			|| RplGetAddr(&szBuffer[1],pdwAddr))
		{
			// test if valid hex address
			for (i = 0; bSucc && i < (LONG) lstrlen(szBuffer); ++i)
			{
				bSucc = (_istxdigit((_TUCHAR) szBuffer[i]) != 0);
			}

			if (bSucc)						// valid characters
			{
				// convert string to number
				*pdwAddr = _tcstoul(szBuffer,NULL,16);
			}
		}

		// inside address range?
		bSucc = bSucc && (*pdwAddr <= dwMaxAddr);

		if (!bSucc)							// invalid address
		{
			SendMessage(hWnd,EM_SETSEL,0,-1);
			SetFocus(hWnd);					// focus to edit control
		}
	}
	return bSucc;
}

//
// set mapping menu
//
static VOID SetMappingMenu(HWND hDlg,UINT uID)
{
	enum MEM_MAPPING eMode;
	LPTSTR szCaption;

	UINT uEnable = MF_GRAYED;				// disable Memory Data menu items

	CheckMenuItem(hMenuMem,uIDMap,MF_UNCHECKED);

	switch (uID)
	{
	case ID_DEBUG_MEM_MAP:
		szCaption = _T("Memory");
		eMode = MEM_MMU;
		uEnable = MF_ENABLED;				// enable Memory Data menu items
		break;
	case ID_DEBUG_MEM_NCE1:
		szCaption = _T("Memory (NCE1)");
		eMode = MEM_NCE1;
		break;
	case ID_DEBUG_MEM_NCE2:
		szCaption = _T("Memory (NCE2)");
		eMode = MEM_NCE2;
		break;
	case ID_DEBUG_MEM_CE1:
		szCaption = _T("Memory (CE1)");
		eMode = MEM_CE1;
		break;
	case ID_DEBUG_MEM_CE2:
		szCaption = _T("Memory (CE2)");
		eMode = MEM_CE2;
		break;
	case ID_DEBUG_MEM_NCE3:
		szCaption = _T("Memory (NCE3)");
		eMode = MEM_NCE3;
		break;
	default: _ASSERT(0);
	}

	VERIFY(SetMemMapType(eMode));

	if (uIDMap != uID) dwAdrMem = 0;		// view from address 0

	uIDMap = uID;
	CheckMenuItem(hMenuMem,uIDMap,MF_CHECKED);

	EnableMenuItem(hMenuMem,ID_DEBUG_MEM_LOAD,uEnable);
	EnableMenuItem(hMenuMem,ID_DEBUG_MEM_SAVE,uEnable);

	SetDlgItemText(hDlg,IDC_STATIC_MEMORY,szCaption);
	return;
};

//
// get address of cursor in memory window
//
static DWORD GetMemCurAddr(HWND hDlg)
{
	INT   i,nPos;
	DWORD dwAddr = dwAdrMem;
	DWORD dwMapDataMask = GetMemDataMask();

	for (i = 0; i < MEMWNDMAX; ++i)			// scan all memory cols
	{
		// column has cursor
		if ((nPos = (INT) SendDlgItemMessage(hDlg,nCol[i],LB_GETCURSEL,0,0)) != LB_ERR)
		{
			dwAddr += (DWORD) (nPos * MEMWNDMAX + i) * 2;
			dwAddr &= dwMapDataMask;
			break;
		}
	}
	return dwAddr;
}

//
// set/reset breakpoint
//
static __inline VOID ToggleBreakpoint(DWORD dwAddr)
{
	INT i;

	for (i = 0; i < wBreakpointCount; ++i)	// scan all breakpoints
	{
		// code breakpoint found
		if (sBreakpoint[i].nType == BP_EXEC && sBreakpoint[i].dwAddr == dwAddr)
		{
			if (!sBreakpoint[i].bEnable)	// breakpoint disabled
			{
				sBreakpoint[i].bEnable = TRUE;
				return;
			}

			while (++i < wBreakpointCount)	// purge breakpoint
				sBreakpoint[i-1] = sBreakpoint[i];
			--wBreakpointCount;
			return;
		}
	}

	// breakpoint not found
	if (wBreakpointCount >= MAXBREAKPOINTS)	// breakpoint buffer full
	{
		AbortMessage(_T("Reached maximum number of breakpoints !"));
		return;
	}

	sBreakpoint[wBreakpointCount].bEnable = TRUE;
	sBreakpoint[wBreakpointCount].nType   = BP_EXEC;
	sBreakpoint[wBreakpointCount].dwAddr  = dwAddr;
	++wBreakpointCount;
	return;
}

//
// init memory mapping table and mapping menu entry
//
static __inline VOID InitMemMap(HWND hDlg)
{
	BOOL bActive;

	SetMemRomType(cCurrentRomType);			// set current model

	_ASSERT(hMenuMem);

	// enable menu mappings
	EnableMenuItem(hMenuMem,ID_DEBUG_MEM_NCE1,GetMemAvail(MEM_NCE1) ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hMenuMem,ID_DEBUG_MEM_NCE2,GetMemAvail(MEM_NCE2) ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hMenuMem,ID_DEBUG_MEM_CE1, GetMemAvail(MEM_CE1)  ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hMenuMem,ID_DEBUG_MEM_CE2, GetMemAvail(MEM_CE2)  ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hMenuMem,ID_DEBUG_MEM_NCE3,GetMemAvail(MEM_NCE3) ? MF_ENABLED : MF_GRAYED);

	bActive =  (ID_DEBUG_MEM_NCE1 == uIDMap && GetMemAvail(MEM_NCE1))
			|| (ID_DEBUG_MEM_NCE2 == uIDMap && GetMemAvail(MEM_NCE2))
			|| (ID_DEBUG_MEM_CE1  == uIDMap && GetMemAvail(MEM_CE1))
			|| (ID_DEBUG_MEM_CE2  == uIDMap && GetMemAvail(MEM_CE2))
			|| (ID_DEBUG_MEM_NCE3 == uIDMap && GetMemAvail(MEM_NCE3));

	SetMappingMenu(hDlg,bActive ? uIDMap : ID_DEBUG_MEM_MAP);
	return;
}

//
// init bank switcher area
//
static __inline VOID InitBsArea(HWND hDlg)
{
	// not 38 / 48S // CdB for HP: add apples type
	if (cCurrentRomType!='A' && cCurrentRomType!='S')
	{
		EnableWindow(GetDlgItem(hDlg,IDC_MISC_BS_TXT),TRUE);
		EnableWindow(GetDlgItem(hDlg,IDC_MISC_BS),TRUE);
	}
	return;
}

//
// convert nibble register to string
//
static LPTSTR RegToStr(BYTE *pReg, WORD wNib)
{
	static TCHAR szBuffer[32];

	WORD i;

	for (i = 0;i < wNib;++i)
		szBuffer[i] = cHex[pReg[wNib-i-1]];
	szBuffer[i] = 0;

	return szBuffer;
}

//
// convert string to nibble register
//
static VOID StrToReg(BYTE *pReg, WORD wNib, LPTSTR lpszValue)
{
	int i,nValuelen;

	nValuelen = lstrlen(lpszValue);
	for (i = wNib - 1;i >= 0;--i)
	{
		if (i >= nValuelen)					// no character in string
		{
			pReg[i] = 0;					// fill with zero
		}
		else
		{
			// convert to number
			pReg[i] = _totupper(*lpszValue) - _T('0');
			if (pReg[i] > 9) pReg[i] -= _T('A') - _T('9') - 1;
			++lpszValue;
		}
	}
	return;
}

//
// write code window
//
static INT ViewCodeWnd(HWND hWnd, DWORD dwAddress)
{
	enum MEM_MAPPING eMapMode;
	LPCTSTR lpszName;
	TCHAR   szAddress[64];
	DWORD   dwNxtAddr;
	INT     i,j,nLinePC;

	nLinePC = -1;							// PC not shown (no selection)

	eMapMode = GetMemMapType();				// get current map mode
	SetMemMapType(MEM_MMU);					// disassemble in mapped mode

	dwAddress &= 0xFFFFF;					// adjust to Saturn address range
	SendMessage(hWnd,WM_SETREDRAW,FALSE,0);
	SendMessage(hWnd,LB_RESETCONTENT,0,0);
	for (i = 0; i < MAXCODELINES; ++i)
	{
		// entry has a name
		if (disassembler_symb && (lpszName = RplGetName(dwAddress)) != NULL)
		{
			// save address as label
			dwAdrLine[i] = dwAddress | CODELABEL;

			szAddress[0] = _T('=');
			lstrcpyn(&szAddress[1],lpszName,ARRAYSIZEOF(szAddress)-1);
			SendMessage(hWnd,LB_ADDSTRING,0,(LPARAM) szAddress);
			if (++i == MAXCODELINES) break;
		}

		// remember line of PC
		if (dwAddress == Chipset.pc) nLinePC = i;

		dwAdrLine[i] = dwAddress;
		j = wsprintf(szAddress,
					 (dwAddress == Chipset.pc) ? _T("%05lX-%c ") : _T("%05lX   "),
					 dwAddress,nRplBreak ? _T('R') : _T('>'));

		dwNxtAddr = (dwAddress + 5) & 0xFFFFF;

		// check if address content is a PCO (Primitive Code Object)
		// make sure when the PC points to such an address that it's
		// interpreted as opcode
		if ((dwAddress != Chipset.pc) && (Read5(dwAddress) == dwNxtAddr))
		{
			if (disassembler_mode == HP_MNEMONICS)
			{
				_tcscat(&szAddress[j],_T("CON(5)  (*)+5"));
			}
			else
			{
				wsprintf(&szAddress[j],_T("dcr.5   $%05x"),dwNxtAddr);
			}
			dwAddress = dwNxtAddr;
		}
		else
		{
			dwAddress = disassemble(dwAddress,&szAddress[j]);
		}
		SendMessage(hWnd,LB_ADDSTRING,0,(LPARAM) szAddress);
	}
	SendMessage(hWnd,WM_SETREDRAW,TRUE,0);
	SetMemMapType(eMapMode);				// switch back to old map mode
	return nLinePC;
}

//
// write memory window
//
static VOID ViewMemWnd(HWND hDlg, DWORD dwAddress)
{
	#define TEXTOFF 32

	INT   i,j;
	TCHAR szBuffer[16],szItem[4];
	DWORD dwMapDataMask;
	BYTE  cChar;

	szItem[2] = 0;							// end of string
	dwAdrMem = dwAddress;					// save start address of memory window
	dwMapDataMask = GetMemDataMask();		// size mask of data mapping

	// purge all list boxes
	SendDlgItemMessage(hDlg,IDC_DEBUG_MEM_ADDR,LB_RESETCONTENT,0,0);
	SendDlgItemMessage(hDlg,IDC_DEBUG_MEM_TEXT,LB_RESETCONTENT,0,0);
	for (j = 0; j < MEMWNDMAX; ++j)
		SendDlgItemMessage(hDlg,nCol[j],LB_RESETCONTENT,0,0);

	for (i = 0; i < MAXMEMLINES; ++i)
	{
		BYTE byLineData[MAXMEMITEMS];

		if (ID_DEBUG_MEM_MAP == uIDMap)		// mapped memory content
		{
			wsprintf(szBuffer,_T("%05lX"),dwAddress);
		}
		else								// module memory content
		{
			wsprintf(szBuffer,_T("%06lX"),dwAddress);
		}
		SendDlgItemMessage(hDlg,IDC_DEBUG_MEM_ADDR,LB_ADDSTRING,0,(LPARAM) szBuffer);

		// fetch data line
		GetMemPeek(byLineData, dwAddress, MAXMEMITEMS);

		for (j = 0; j < MAXMEMITEMS; ++j)
		{
			// read from fetched data line
			szItem[j&0x1] = cHex[byLineData[j]];
			// characters are saved in LBS, MSB order
			cChar = (cChar >> 4) | (byLineData[j] << 4);

			if ((j&0x1) != 0)
			{
				// byte field
				_ASSERT(j/2 < MEMWNDMAX);
				SendDlgItemMessage(hDlg,nCol[j/2],LB_ADDSTRING,0,(LPARAM) szItem);

				// text field
				szBuffer[j/2] = (isprint(cChar) != 0) ? cChar : _T('.');
			}
		}
		szBuffer[j/2] = 0;					// end of text string
		SendDlgItemMessage(hDlg,IDC_DEBUG_MEM_TEXT,LB_ADDSTRING,0,(LPARAM) szBuffer);

		dwAddress = (dwAddress + MAXMEMITEMS) & dwMapDataMask;
	}
	return;
	#undef TEXTOFF
}


//################
//#
//#    High level Window draw routines
//#
//################

//
// update code window with scrolling
//
static VOID UpdateCodeWnd(HWND hDlg)
{
	DWORD dwAddress,dwPriorAddr;
	INT   i,j;

	HWND hWnd = GetDlgItem(hDlg,IDC_DEBUG_CODE);

	j = (INT) SendMessage(hWnd,LB_GETCOUNT,0,0); // no. of items in table

	// seach for actual address in code area
	for (i = 0; i < j; ++i)
	{
		if (dwAdrLine[i] == Chipset.pc)		// found new pc address line
			break;
	}

	// redraw code window
	dwAddress = dwAdrLine[0];				// redraw list box with modified pc
	if (i == j)								// address not found
	{
		dwAddress = Chipset.pc;				// begin with actual pc

		// check if current pc is the begin of a PCO, show PCO address
		dwPriorAddr = (dwAddress - 5) & 0xFFFFF;
		if (Read5(dwPriorAddr) == dwAddress)
			dwAddress = dwPriorAddr;
	}
	else
	{
		if (i > 10)							// cursor near bottom line
		{
			j = 10;							// pc to line 11

			// new address line is label
			if ((dwAdrLine[i-11] & CODELABEL) != 0)
				--j;						// pc to line 10

			dwAddress = dwAdrLine[i-j];		// move that pc is in line 11
		}
	}

	i = ViewCodeWnd(hWnd,dwAddress);		// init code area
	SendMessage(hWnd,LB_SETCURSEL,i,0);		// set cursor on actual pc
	return;
}

//
// update register window
//
static VOID UpdateRegisterWnd(HWND hDlg)
{
	TCHAR szBuffer[64];

	_ASSERTREG(IDC_REG_A);
	bRegUpdate[IDC_REG_A-REG_START] = memcmp(Chipset.A, OldChipset.A, sizeof(Chipset.A)) != 0;
	wsprintf(szBuffer,_T("A= %s"),RegToStr(Chipset.A,16));
	SetDlgItemText(hDlg,IDC_REG_A,szBuffer);
	_ASSERTREG(IDC_REG_B);
	bRegUpdate[IDC_REG_B-REG_START] = memcmp(Chipset.B, OldChipset.B, sizeof(Chipset.B)) != 0;
	wsprintf(szBuffer,_T("B= %s"),RegToStr(Chipset.B,16));
	SetDlgItemText(hDlg,IDC_REG_B,szBuffer);
	_ASSERTREG(IDC_REG_C);
	bRegUpdate[IDC_REG_C-REG_START] = memcmp(Chipset.C, OldChipset.C, sizeof(Chipset.C)) != 0;
	wsprintf(szBuffer,_T("C= %s"),RegToStr(Chipset.C,16));
	SetDlgItemText(hDlg,IDC_REG_C,szBuffer);
	_ASSERTREG(IDC_REG_D);
	bRegUpdate[IDC_REG_D-REG_START] = memcmp(Chipset.D, OldChipset.D, sizeof(Chipset.D)) != 0;
	wsprintf(szBuffer,_T("D= %s"),RegToStr(Chipset.D,16));
	SetDlgItemText(hDlg,IDC_REG_D,szBuffer);
	_ASSERTREG(IDC_REG_R0);
	bRegUpdate[IDC_REG_R0-REG_START] = memcmp(Chipset.R0, OldChipset.R0, sizeof(Chipset.R0)) != 0;
	wsprintf(szBuffer,_T("R0=%s"),RegToStr(Chipset.R0,16));
	SetDlgItemText(hDlg,IDC_REG_R0,szBuffer);
	_ASSERTREG(IDC_REG_R1);
	bRegUpdate[IDC_REG_R1-REG_START] = memcmp(Chipset.R1, OldChipset.R1, sizeof(Chipset.R1)) != 0;
	wsprintf(szBuffer,_T("R1=%s"),RegToStr(Chipset.R1,16));
	SetDlgItemText(hDlg,IDC_REG_R1,szBuffer);
	_ASSERTREG(IDC_REG_R2);
	bRegUpdate[IDC_REG_R2-REG_START] = memcmp(Chipset.R2, OldChipset.R2, sizeof(Chipset.R2)) != 0;
	wsprintf(szBuffer,_T("R2=%s"),RegToStr(Chipset.R2,16));
	SetDlgItemText(hDlg,IDC_REG_R2,szBuffer);
	_ASSERTREG(IDC_REG_R3);
	bRegUpdate[IDC_REG_R3-REG_START] = memcmp(Chipset.R3, OldChipset.R3, sizeof(Chipset.R3)) != 0;
	wsprintf(szBuffer,_T("R3=%s"),RegToStr(Chipset.R3,16));
	SetDlgItemText(hDlg,IDC_REG_R3,szBuffer);
	_ASSERTREG(IDC_REG_R4);
	bRegUpdate[IDC_REG_R4-REG_START] = memcmp(Chipset.R4, OldChipset.R4, sizeof(Chipset.R4)) != 0;
	wsprintf(szBuffer,_T("R4=%s"),RegToStr(Chipset.R4,16));
	SetDlgItemText(hDlg,IDC_REG_R4,szBuffer);
	_ASSERTREG(IDC_REG_D0);
	bRegUpdate[IDC_REG_D0-REG_START] = Chipset.d0 != OldChipset.d0;
	wsprintf(szBuffer,_T("D0=%05X"),Chipset.d0);
	SetDlgItemText(hDlg,IDC_REG_D0,szBuffer);
	_ASSERTREG(IDC_REG_D1);
	bRegUpdate[IDC_REG_D1-REG_START] = Chipset.d1 != OldChipset.d1;
	wsprintf(szBuffer,_T("D1=%05X"),Chipset.d1);
	SetDlgItemText(hDlg,IDC_REG_D1,szBuffer);
	_ASSERTREG(IDC_REG_P);
	bRegUpdate[IDC_REG_P-REG_START] = Chipset.P != OldChipset.P;
	wsprintf(szBuffer,_T("P=%X"),Chipset.P);
	SetDlgItemText(hDlg,IDC_REG_P,szBuffer);
	_ASSERTREG(IDC_REG_PC);
	bRegUpdate[IDC_REG_PC-REG_START] = Chipset.pc != OldChipset.pc;
	wsprintf(szBuffer,_T("PC=%05X"),Chipset.pc);
	SetDlgItemText(hDlg,IDC_REG_PC,szBuffer);
	_ASSERTREG(IDC_REG_OUT);
	bRegUpdate[IDC_REG_OUT-REG_START] = Chipset.out != OldChipset.out;
	wsprintf(szBuffer,_T("OUT=%03X"),Chipset.out);
	SetDlgItemText(hDlg,IDC_REG_OUT,szBuffer);
	_ASSERTREG(IDC_REG_IN);
	bRegUpdate[IDC_REG_IN-REG_START] = Chipset.in != OldChipset.in;
	wsprintf(szBuffer,_T("IN=%04X"),Chipset.in);
	SetDlgItemText(hDlg,IDC_REG_IN,szBuffer);
	_ASSERTREG(IDC_REG_ST);
	bRegUpdate[IDC_REG_ST-REG_START] = memcmp(Chipset.ST, OldChipset.ST, sizeof(Chipset.ST)) != 0;
	wsprintf(szBuffer,_T("ST=%s"),RegToStr(Chipset.ST,4));
	SetDlgItemText(hDlg,IDC_REG_ST,szBuffer);
	_ASSERTREG(IDC_REG_CY);
	bRegUpdate[IDC_REG_CY-REG_START] = Chipset.carry != OldChipset.carry;
	wsprintf(szBuffer,_T("CY=%d"),Chipset.carry);
	SetDlgItemText(hDlg,IDC_REG_CY,szBuffer);
	_ASSERTREG(IDC_REG_MODE);
	bRegUpdate[IDC_REG_MODE-REG_START] = Chipset.mode_dec != OldChipset.mode_dec;
	wsprintf(szBuffer,_T("Mode=%c"),Chipset.mode_dec ? _T('D') : _T('H'));
	SetDlgItemText(hDlg,IDC_REG_MODE,szBuffer);
	_ASSERTREG(IDC_REG_MP);
	bRegUpdate[IDC_REG_MP-REG_START] = ((Chipset.HST ^ OldChipset.HST) & MP) != 0;
	wsprintf(szBuffer,_T("MP=%d"),(Chipset.HST & MP) != 0);
	SetDlgItemText(hDlg,IDC_REG_MP,szBuffer);
	_ASSERTREG(IDC_REG_SR);
	bRegUpdate[IDC_REG_SR-REG_START] = ((Chipset.HST ^ OldChipset.HST) & SR) != 0;
	wsprintf(szBuffer,_T("SR=%d"),(Chipset.HST & SR) != 0);
	SetDlgItemText(hDlg,IDC_REG_SR,szBuffer);
	_ASSERTREG(IDC_REG_SB);
	bRegUpdate[IDC_REG_SB-REG_START] = ((Chipset.HST ^ OldChipset.HST) & SB) != 0;
	wsprintf(szBuffer,_T("SB=%d"),(Chipset.HST & SB) != 0);
	SetDlgItemText(hDlg,IDC_REG_SB,szBuffer);
	_ASSERTREG(IDC_REG_XM);
	bRegUpdate[IDC_REG_XM-REG_START] = ((Chipset.HST ^ OldChipset.HST) & XM) != 0;
	wsprintf(szBuffer,_T("XM=%d"),(Chipset.HST & XM) != 0);
	SetDlgItemText(hDlg,IDC_REG_XM,szBuffer);
	return;
}

//
// update memory window
//
static VOID UpdateMemoryWnd(HWND hDlg)
{
	// check follow mode setting for memory window
	switch(uIDFol)
	{
	case ID_DEBUG_MEM_FNONE:                                break;
	case ID_DEBUG_MEM_FADDR: dwAdrMem = Read5(dwAdrMemFol); break;
	case ID_DEBUG_MEM_FPC:   dwAdrMem = Chipset.pc;         break;
	case ID_DEBUG_MEM_FD0:   dwAdrMem = Chipset.d0;         break;
	case ID_DEBUG_MEM_FD1:   dwAdrMem = Chipset.d1;         break;
	default: _ASSERT(FALSE);
	}

	ViewMemWnd(hDlg,dwAdrMem);
	return;
}

//
// update stack window
//
static VOID UpdateStackWnd(HWND hDlg)
{
	UINT  i;
	LONG  nPos;
	TCHAR szBuffer[64];

	HWND hWnd = GetDlgItem(hDlg,IDC_DEBUG_STACK);

	SendMessage(hWnd,WM_SETREDRAW,FALSE,0);
	nPos = (LONG) SendMessage(hWnd,LB_GETTOPINDEX,0,0);
	SendMessage(hWnd,LB_RESETCONTENT,0,0);
	for (i = 1; i <= ARRAYSIZEOF(Chipset.rstk); ++i)
	{
		INT j;

		wsprintf(szBuffer,_T("%d: %05X"), i, Chipset.rstk[(Chipset.rstkp-i)&7]);
		j = (INT) SendMessage(hWnd,LB_ADDSTRING,0,(LPARAM) szBuffer);
		SendMessage(hWnd,LB_SETITEMDATA,j,Chipset.rstk[(Chipset.rstkp-i)&7]);
	}
	SendMessage(hWnd,LB_SETTOPINDEX,nPos,0);
	SendMessage(hWnd,WM_SETREDRAW,TRUE,0);
	return;
}

//
// update MMU window
//
static VOID UpdateMmuWnd(HWND hDlg)
{
	TCHAR szBuffer[64];

	if (Chipset.IOCfig)
		wsprintf(szBuffer,_T("%05X"),Chipset.IOBase);
	else
		lstrcpy(szBuffer,_T("-----"));
	SetDlgItemText(hDlg,IDC_MMU_IO_A,szBuffer);
	if (Chipset.P0Cfig)
		wsprintf(szBuffer,_T("%05X"),Chipset.P0Base<<12);
	else
		lstrcpy(szBuffer,_T("-----"));
	SetDlgItemText(hDlg,IDC_MMU_NCE2_A,szBuffer);
	if (Chipset.P0Cfg2)
		wsprintf(szBuffer,_T("%05X"),(Chipset.P0Size^0xFF)<<12);
	else
		lstrcpy(szBuffer,_T("-----"));
	SetDlgItemText(hDlg,IDC_MMU_NCE2_S,szBuffer);
	if (Chipset.P1Cfig)
		wsprintf(szBuffer,_T("%05X"),Chipset.P1Base<<12);
	else
		lstrcpy(szBuffer,_T("-----"));
	SetDlgItemText(hDlg,(cCurrentRomType=='S') ? IDC_MMU_CE1_A : IDC_MMU_CE2_A,szBuffer);
	if (Chipset.P1Cfg2)
		wsprintf(szBuffer,_T("%05X"),(Chipset.P1Size^0xFF)<<12);
	else
		lstrcpy(szBuffer,_T("-----"));
	SetDlgItemText(hDlg,(cCurrentRomType=='S') ? IDC_MMU_CE1_S : IDC_MMU_CE2_S,szBuffer);
	if (Chipset.P2Cfig)
		wsprintf(szBuffer,_T("%05X"),Chipset.P2Base<<12);
	else
		lstrcpy(szBuffer,_T("-----"));
	SetDlgItemText(hDlg,(cCurrentRomType=='S') ? IDC_MMU_CE2_A : IDC_MMU_NCE3_A,szBuffer);
	if (Chipset.P2Cfg2)
		wsprintf(szBuffer,_T("%05X"),(Chipset.P2Size^0xFF)<<12);
	else
		lstrcpy(szBuffer,_T("-----"));
	SetDlgItemText(hDlg,(cCurrentRomType=='S') ? IDC_MMU_CE2_S : IDC_MMU_NCE3_S,szBuffer);
	if (Chipset.BSCfig)
		wsprintf(szBuffer,_T("%05X"),Chipset.BSBase<<12);
	else
		lstrcpy(szBuffer,_T("-----"));
	SetDlgItemText(hDlg,(cCurrentRomType=='S') ? IDC_MMU_NCE3_A : IDC_MMU_CE1_A,szBuffer);
	if (Chipset.BSCfg2)
		wsprintf(szBuffer,_T("%05X"),(Chipset.BSSize^0xFF)<<12);
	else
		lstrcpy(szBuffer,_T("-----"));
	SetDlgItemText(hDlg,(cCurrentRomType=='S') ? IDC_MMU_NCE3_S : IDC_MMU_CE1_S,szBuffer);
	return;
}

//
// update miscellaneous window
//
static VOID UpdateMiscWnd(HWND hDlg)
{
	_ASSERTREG(IDC_MISC_INT);
	bRegUpdate[IDC_MISC_INT-REG_START] = Chipset.inte != OldChipset.inte;
	SetDlgItemText(hDlg,IDC_MISC_INT,Chipset.inte ? _T("On ") : _T("Off"));

	_ASSERTREG(IDC_MISC_KEY);
	bRegUpdate[IDC_MISC_KEY-REG_START] = Chipset.intk != OldChipset.intk;
	SetDlgItemText(hDlg,IDC_MISC_KEY,Chipset.intk ? _T("On ") : _T("Off"));

	_ASSERTREG(IDC_MISC_BS);
	bRegUpdate[IDC_MISC_BS-REG_START] = FALSE;

	// not 38/48S // CdB for HP: add Apples type
	if (cCurrentRomType!='A' && cCurrentRomType!='S')
	{
		TCHAR szBuffer[64];

		bRegUpdate[IDC_MISC_BS-REG_START] = (Chipset.Bank_FF & 0x7F) != (OldChipset.Bank_FF & 0x7F);
		wsprintf(szBuffer,_T("%02X"),Chipset.Bank_FF & 0x7F);
		SetDlgItemText(hDlg,IDC_MISC_BS,szBuffer);
	}
	return;
}

//
// update complete debugger dialog
//
VOID OnUpdate(HWND hDlg)
{
	nDbgState = DBG_STEPINTO;				// state "step into"
	dwDbgStopPC = -1;						// disable "cursor stop address"

	// enable debug buttons
	EnableMenuItem(GetMenu(hDlg),ID_DEBUG_RUN,MF_ENABLED);
	EnableMenuItem(GetMenu(hDlg),ID_DEBUG_RUNCURSOR,MF_ENABLED);
	EnableMenuItem(GetMenu(hDlg),ID_DEBUG_STEP,MF_ENABLED);
	EnableMenuItem(GetMenu(hDlg),ID_DEBUG_STEPOVER,MF_ENABLED);
	EnableMenuItem(GetMenu(hDlg),ID_DEBUG_STEPOUT,MF_ENABLED);
	EnableMenuItem(GetMenu(hDlg),ID_INFO_LASTINSTRUCTIONS,MF_ENABLED);
	EnableMenuItem(GetMenu(hDlg),ID_INFO_WRITEONLYREG,MF_ENABLED);

	// enable toolbar buttons
	SendMessage(hWndToolbar,TB_ENABLEBUTTON,ID_DEBUG_RUN,MAKELONG((TRUE),0));
	SendMessage(hWndToolbar,TB_ENABLEBUTTON,ID_DEBUG_BREAK,MAKELONG((FALSE),0));
	SendMessage(hWndToolbar,TB_ENABLEBUTTON,ID_DEBUG_RUNCURSOR,MAKELONG((TRUE),0));
	SendMessage(hWndToolbar,TB_ENABLEBUTTON,ID_DEBUG_STEP,MAKELONG((TRUE),0));
	SendMessage(hWndToolbar,TB_ENABLEBUTTON,ID_DEBUG_STEPOVER,MAKELONG((TRUE),0));
	SendMessage(hWndToolbar,TB_ENABLEBUTTON,ID_DEBUG_STEPOUT,MAKELONG((TRUE),0));

	// update windows
	UpdateCodeWnd(hDlg);					// update code window
	UpdateRegisterWnd(hDlg);				// update registers window
	UpdateMemoryWnd(hDlg);					// update memory window
	UpdateStackWnd(hDlg);					// update stack window
	UpdateMmuWnd(hDlg);						// update MMU window
	UpdateMiscWnd(hDlg);					// update bank switcher window
	UpdateProfileWnd(hDlgProfile);			// update profiler dialog
	ShowWindow(hDlg,SW_RESTORE);			// pop up if minimized
	SetFocus(hDlg);							// set focus to debugger
	return;
}


//################
//#
//#    Virtual key handler
//#
//################

//
// toggle breakpoint key handler (F2)
//
static BOOL OnKeyF2(HWND hDlg)
{
	HWND hWnd;
	RECT rc;
	LONG i;

	hWnd = GetDlgItem(hDlg,IDC_DEBUG_CODE);
	i = (LONG) SendMessage(hWnd,LB_GETCURSEL,0,0); // get selected item
	ToggleBreakpoint(dwAdrLine[i]);			// toggle breakpoint at address
	// update region of toggled item
	SendMessage(hWnd,LB_GETITEMRECT,i,(LPARAM)&rc);
	InvalidateRect(hWnd,&rc,TRUE);
	return -1;								// call windows default handler
}

//
// run key handler (F5)
//
static BOOL OnKeyF5(HWND hDlg)
{
	HWND  hWnd;
	INT   i,nPos;
	TCHAR szBuf[64];

	if (nDbgState != DBG_RUN)				// emulation stopped
	{
		DisableMenuKeys(hDlg);				// disable menu keys

		hWnd = GetDlgItem(hDlg,IDC_DEBUG_CODE);
		nPos = (INT) SendMessage(hWnd,LB_GETCURSEL,0,0);

		// clear "->" in code window
		for (i = 0; i < MAXCODELINES; ++i)
		{
			if (dwAdrLine[i] == Chipset.pc)	// PC in window
			{
				SendMessage(hWnd,LB_GETTEXT,i,(LPARAM) szBuf);
				szBuf[5] = szBuf[6] = _T(' ');
				SendMessage(hWnd,LB_DELETESTRING,i,0);
				SendMessage(hWnd,LB_INSERTSTRING,i,(LPARAM) szBuf);
				break;
			}
		}
		SendMessage(hWnd,LB_SETCURSEL,nPos,0);

		nDbgState = DBG_RUN;				// state "run"
		OldChipset = Chipset;				// save chipset values
		SetEvent(hEventDebug);				// run emulation
	}
	return -1;								// call windows default handler
	UNREFERENCED_PARAMETER(hDlg);
}

//
// step cursor key handler (F6)
//
static BOOL OnKeyF6(HWND hDlg)
{
	if (nDbgState != DBG_RUN)				// emulation stopped
	{
		// get address of selected item
		INT nPos = (INT) SendDlgItemMessage(hDlg,IDC_DEBUG_CODE,LB_GETCURSEL,0,0);
		dwDbgStopPC = dwAdrLine[nPos];

		OnKeyF5(hDlg);						// run emulation
	}
	return -1;								// call windows default handler
}

//
// step into key handler (F7)
//
static BOOL OnKeyF7(HWND hDlg)
{
	if (nDbgState != DBG_RUN)				// emulation stopped
	{
		if (bDbgSkipInt)					// skip code in interrupt handler
			DisableMenuKeys(hDlg);			// disable menu keys

		nDbgState = DBG_STEPINTO;			// state "step into"
		OldChipset = Chipset;				// save chipset values
		SetEvent(hEventDebug);				// run emulation
	}
	return -1;								// call windows default handler
	UNREFERENCED_PARAMETER(hDlg);
}

//
// step over key handler (F8)
//
static BOOL OnKeyF8(HWND hDlg)
{
	if (nDbgState != DBG_RUN)				// emulation stopped
	{
		LPBYTE I = FASTPTR(Chipset.pc);

		if (bDbgSkipInt)					// skip code in interrupt handler
			DisableMenuKeys(hDlg);			// disable menu keys

		dwDbgRstkp = Chipset.rstkp;			// save stack level

		// GOSUB 7aaa, GOSUBL 8Eaaaa, GOSBVL 8Faaaaa
		if (I[0] == 0x7 || (I[0] == 0x8 && (I[1] == 0xE || I[1] == 0xF)))
		{
			nDbgState = DBG_STEPOVER;		// state "step over"
		}
		else
		{
			nDbgState = DBG_STEPINTO;		// state "step into"
		}
		OldChipset = Chipset;				// save chipset values
		SetEvent(hEventDebug);				// run emulation
	}
	return -1;								// call windows default handler
	UNREFERENCED_PARAMETER(hDlg);
}

//
// step out key handler (F9)
//
static BOOL OnKeyF9(HWND hDlg)
{
	if (nDbgState != DBG_RUN)				// emulation stopped
	{
		DisableMenuKeys(hDlg);				// disable menu keys
		dwDbgRstkp = (Chipset.rstkp-1)&7;	// save stack data
		dwDbgRstk  = Chipset.rstk[dwDbgRstkp];
		nDbgState = DBG_STEPOUT;			// state "step out"
		OldChipset = Chipset;				// save chipset values
		SetEvent(hEventDebug);				// run emulation
	}
	return -1;								// call windows default handler
	UNREFERENCED_PARAMETER(hDlg);
}

//
// break key handler (F11)
//
static BOOL OnKeyF11(HWND hDlg)
{
	nDbgState = DBG_STEPINTO;				// state "step into"
	if (Chipset.Shutdn)						// cpu thread stopped
		SetEvent(hEventShutdn);				// goto debug session
	return -1;								// call windows default handler
	UNREFERENCED_PARAMETER(hDlg);
}

//
// view of given address in disassembler window
//
static BOOL OnCodeGoAdr(HWND hDlg)
{
	DWORD dwAddress = -1;					// no address given

	OnEnterAddress(hDlg, &dwAddress);
	if (dwAddress != -1)
	{
		HWND hWnd = GetDlgItem(hDlg,IDC_DEBUG_CODE);
		ViewCodeWnd(hWnd,dwAddress);
		SendMessage(hWnd,LB_SETCURSEL,0,0);
	}
	return -1;								// call windows default handler
}

//
// view pc in disassembler window
//
static BOOL OnCodeGoPC(HWND hDlg)
{
	UpdateCodeWnd(hDlg);
	return 0;
}

//
// set pc to selection
//
static BOOL OnCodeSetPcToSelection(HWND hDlg)
{
	Chipset.pc = dwAdrLine[SendDlgItemMessage(hDlg,IDC_DEBUG_CODE,LB_GETCURSEL,0,0)];
	return OnCodeGoPC(hDlg);
}

//
// find PCO object in code window
//
static BOOL OnCodeFindPCO(HWND hDlg,DWORD dwAddr,INT nSearchDir)
{
	DWORD dwCnt;
	BOOL  bMatch;

	// searching upwards / downwards
	_ASSERT(nSearchDir == 1 || nSearchDir == -1);

	dwAddr += nSearchDir;					// start address for search

	// scan mapped address area until PCO found
	for (dwCnt = 0; dwCnt <= 0xFFFFF; ++dwCnt)
	{
		// is this a PCO?
		bMatch = (Read5(dwAddr & 0xFFFFF) == ((dwAddr + 5) & 0xFFFFF));

		if (bMatch)
		{
			// update code window
			ViewCodeWnd(GetDlgItem(hDlg,IDC_DEBUG_CODE),dwAddr);
			break;
		}

		dwAddr += nSearchDir;
	}
	return 0;
}

//
// view from address in memory window
//
static BOOL OnMemGoDx(HWND hDlg, DWORD dwAddress)
{
	HWND hWnd = GetDlgItem(hDlg,IDC_DEBUG_MEM_COL0);

	ViewMemWnd(hDlg, dwAddress);
	SendMessage(hWnd,LB_SETCURSEL,0,0);
	SetFocus(hWnd);
	return -1;								// call windows default handler
}

//
// view of given address in memory window
//
static BOOL OnMemGoAdr(HWND hDlg)
{
	DWORD dwAddress = -1;					// no address given

	OnEnterAddress(hDlg, &dwAddress);
	if (dwAddress != -1)					// not Cancel key
		OnMemGoDx(hDlg,dwAddress & GetMemDataMask());
	return -1;								// call windows default handler
}

//
// view from address in memory window
//
static BOOL OnMemFollow(HWND hDlg,UINT uID)
{
	if (ID_DEBUG_MEM_FADDR == uID)			// ask for follow address
	{
		DWORD dwAddress = -1;				// no address given

		OnEnterAddress(hDlg, &dwAddress);
		if (dwAddress == -1) return -1;		// return at cancel button

		dwAdrMemFol = dwAddress;
	}

	CheckMenuItem(hMenuMem,uIDFol,MF_UNCHECKED);
	uIDFol = uID;
	CheckMenuItem(hMenuMem,uIDFol,MF_CHECKED);
	UpdateMemoryWnd(hDlg);					// update memory window
	return -1;								// call windows default handler
}

//
// clear all breakpoints
//
static BOOL OnClearAll(HWND hDlg)
{
	wBreakpointCount = 0;
	// redraw code window
	InvalidateRect(GetDlgItem(hDlg,IDC_DEBUG_CODE),NULL,TRUE);
	return 0;
}

//
// toggle menu selection
//
static BOOL OnToggleMenuItem(HWND hDlg,UINT uIDCheckItem,BOOL *bCheck)
{
	*bCheck = !*bCheck;						// toggle flag
	CheckMenuItem(GetMenu(hDlg),uIDCheckItem,*bCheck ? MF_CHECKED : MF_UNCHECKED);
	return 0;
}

//
// change memory window mapping style
//
static BOOL OnMemMapping(HWND hDlg,UINT uID)
{
	if (uID == uIDMap) return -1;			// same view, call windows default handler

	SetMappingMenu(hDlg,uID);				// update menu settings
	UpdateMemoryWnd(hDlg);					// update memory window
	return 0;
}

//
// push value on hardware stack
//
static BOOL OnStackPush(HWND hDlg)
{
	TCHAR szBuffer[] = _T("00000");
	DWORD dwAddr;
	HWND  hWnd;
	UINT  i,j;

	if (nDbgState != DBG_STEPINTO)			// not in single step mode
		return TRUE;

	hWnd = GetDlgItem(hDlg,IDC_DEBUG_STACK);

	i = (UINT) SendMessage(hWnd,LB_GETCURSEL,0,0);
	if (LB_ERR == (INT) i) return TRUE;		// no selection

	if (IDOK != OnNewValue(szBuffer))		// canceled function
		return TRUE;
	_stscanf(szBuffer,_T("%5X"),&dwAddr);

	// push stack element
	for (j = ARRAYSIZEOF(Chipset.rstk); j > i + 1; --j)
	{
		Chipset.rstk[(Chipset.rstkp-j)&7] = Chipset.rstk[(Chipset.rstkp-j+1)&7];
	}
	Chipset.rstk[(Chipset.rstkp-j)&7] = dwAddr;

	UpdateStackWnd(hDlg);					// update stack window
	SendMessage(hWnd,LB_SETCURSEL,i,0);		// restore cursor postion
	return 0;
}

//
// pop value from hardware stack
//
static BOOL OnStackPop(HWND hDlg)
{
	HWND hWnd;
	UINT i,j;

	if (nDbgState != DBG_STEPINTO)			// not in single step mode
		return TRUE;

	hWnd = GetDlgItem(hDlg,IDC_DEBUG_STACK);

	i = (UINT) SendMessage(hWnd,LB_GETCURSEL,0,0);
	if (LB_ERR == (INT) i) return TRUE;		// no selection

	// pop stack element
	for (j = i + 1; j < ARRAYSIZEOF(Chipset.rstk); ++j)
	{
		Chipset.rstk[(Chipset.rstkp-j)&7] = Chipset.rstk[(Chipset.rstkp-j-1)&7];
	}
	Chipset.rstk[Chipset.rstkp] = 0;

	UpdateStackWnd(hDlg);					// update stack window
	SendMessage(hWnd,LB_SETCURSEL,i,0);		// restore cursor postion
	return 0;
}

// modify value on hardware stack
static BOOL OnStackModify(HWND hDlg)
{
	TCHAR szBuffer[16];
	HWND  hWnd;
	INT   i;

	if (nDbgState != DBG_STEPINTO)			// not in single step mode
		return TRUE;

	hWnd = GetDlgItem(hDlg,IDC_DEBUG_STACK);

	i = (INT) SendMessage(hWnd,LB_GETCURSEL,0,0);
	if (LB_ERR == i) return TRUE;			// no selection

	SendMessage(hWnd,LB_GETTEXT,i,(LPARAM) szBuffer);
	OnNewValue(&szBuffer[3]);
	_stscanf(&szBuffer[3],_T("%5X"),&Chipset.rstk[(Chipset.rstkp-i-1)&7]);

	UpdateStackWnd(hDlg);					// update stack window
	SendMessage(hWnd,LB_SETCURSEL,i,0);		// restore cursor postion
	return 0;
}

//
// new register setting
//
static BOOL OnLButtonUp(HWND hDlg, LPARAM lParam)
{
	TCHAR szBuffer[64];
	POINT pt;
	HWND  hWnd;
	INT   nId;

	if (nDbgState != DBG_STEPINTO)			// not in single step mode
		return TRUE;

	POINTSTOPOINT(pt,MAKEPOINTS(lParam));

	// handle of selected window
	hWnd = ChildWindowFromPointEx(hDlg,pt,CWP_SKIPDISABLED);
	nId = GetDlgCtrlID(hWnd);				// control ID of window

	GetWindowText(hWnd,szBuffer,ARRAYSIZEOF(szBuffer));
	switch (nId)
	{
	case IDC_REG_A: // A
		OnNewValue(&szBuffer[3]);
		StrToReg(Chipset.A,16,&szBuffer[3]);
		break;
	case IDC_REG_B: // B
		OnNewValue(&szBuffer[3]);
		StrToReg(Chipset.B,16,&szBuffer[3]);
		break;
	case IDC_REG_C: // C
		OnNewValue(&szBuffer[3]);
		StrToReg(Chipset.C,16,&szBuffer[3]);
		break;
	case IDC_REG_D: // D
		OnNewValue(&szBuffer[3]);
		StrToReg(Chipset.D,16,&szBuffer[3]);
		break;
	case IDC_REG_R0: // R0
		OnNewValue(&szBuffer[3]);
		StrToReg(Chipset.R0,16,&szBuffer[3]);
		break;
	case IDC_REG_R1: // R1
		OnNewValue(&szBuffer[3]);
		StrToReg(Chipset.R1,16,&szBuffer[3]);
		break;
	case IDC_REG_R2: // R2
		OnNewValue(&szBuffer[3]);
		StrToReg(Chipset.R2,16,&szBuffer[3]);
		break;
	case IDC_REG_R3: // R3
		OnNewValue(&szBuffer[3]);
		StrToReg(Chipset.R3,16,&szBuffer[3]);
		break;
	case IDC_REG_R4: // R4
		OnNewValue(&szBuffer[3]);
		StrToReg(Chipset.R4,16,&szBuffer[3]);
		break;
	case IDC_REG_D0: // D0
		OnNewValue(&szBuffer[3]);
		_stscanf(&szBuffer[3],_T("%5X"),&Chipset.d0);
		break;
	case IDC_REG_D1: // D1
		OnNewValue(&szBuffer[3]);
		_stscanf(&szBuffer[3],_T("%5X"),&Chipset.d1);
		break;
	case IDC_REG_P: // P
		OnNewValue(&szBuffer[2]);
		Chipset.P = _totupper(szBuffer[2]) - _T('0');
		if (Chipset.P > 9) Chipset.P -= 7;
		PCHANGED;
		break;
	case IDC_REG_PC: // PC
		OnNewValue(&szBuffer[3]);
		_stscanf(&szBuffer[3],_T("%5X"),&Chipset.pc);
		break;
	case IDC_REG_OUT: // OUT
		OnNewValue(&szBuffer[4]);
		Chipset.out = (WORD) _tcstoul(&szBuffer[4],NULL,16);
		break;
	case IDC_REG_IN: // IN
		OnNewValue(&szBuffer[3]);
		Chipset.in = (WORD) _tcstoul(&szBuffer[3],NULL,16);
		break;
	case IDC_REG_ST: // ST
		OnNewValue(&szBuffer[3]);
		StrToReg(Chipset.ST,4,&szBuffer[3]);
		break;
	case IDC_REG_CY: // CY
		Chipset.carry = !Chipset.carry;
		break;
	case IDC_REG_MODE: // MODE
		Chipset.mode_dec = !Chipset.mode_dec;
		break;
	case IDC_REG_MP: // MP
		Chipset.HST ^= MP;
		break;
	case IDC_REG_SR: // SR
		Chipset.HST ^= SR;
		break;
	case IDC_REG_SB: // SB
		Chipset.HST ^= SB;
		break;
	case IDC_REG_XM: // XM
		Chipset.HST ^= XM;
		break;
	case IDC_MISC_INT: // interrupt status
		Chipset.inte = !Chipset.inte;
		UpdateMiscWnd(hDlg);				// update miscellaneous window
		break;
	case IDC_MISC_KEY: // 1ms keyboard scan
		Chipset.intk = !Chipset.intk;
		UpdateMiscWnd(hDlg);				// update miscellaneous window
		break;
	case IDC_MISC_BS: // Bank switcher setting
		OnNewValue(szBuffer);
		Chipset.Bank_FF = _tcstoul(szBuffer,NULL,16);
		Chipset.Bank_FF &= 0x7F;
		RomSwitch(Chipset.Bank_FF);			// update memory mapping

		UpdateCodeWnd(hDlg);				// update code window
		UpdateMemoryWnd(hDlg);				// update memory window
		UpdateMiscWnd(hDlg);				// update miscellaneous window
		break;
	}
	UpdateRegisterWnd(hDlg);				// update register
	return TRUE;
}

//
// double click in list box area
//
static BOOL OnDblClick(HWND hWnd, WORD wId)
{
	HWND  hDlg,hWndCode;
	TCHAR szBuffer[4];
	BYTE  byData;
	INT   i;
	DWORD dwAddress;

	hDlg = GetParent(hWnd);					// dialog window handle
	hWndCode = GetDlgItem(hDlg,IDC_DEBUG_CODE);

	if (wId == IDC_DEBUG_STACK)				// stack list box
	{
		// get cpu address of selected item
		i = (INT) SendMessage(hWnd,LB_GETCURSEL,0,0);
		dwAddress = (DWORD) SendMessage(hWnd,LB_GETITEMDATA,i,0);

		ViewCodeWnd(hWndCode,dwAddress);	// show code of this address
		return TRUE;
	}

	for (i = 0; i < MEMWNDMAX; ++i)			// scan all Id's
		if (nCol[i] == wId)					// found ID
			break;

	// not IDC_DEBUG_MEM window or module mode -> default handler
	if (i == MEMWNDMAX || ID_DEBUG_MEM_MAP != uIDMap)
		return FALSE;

	// calculate address of byte
	dwAddress = i * 2;
	i = (INT) SendMessage(hWnd,LB_GETCARETINDEX,0,0);
	dwAddress += MAXMEMITEMS * i + dwAdrMem;

	// enter new value
	SendMessage(hWnd,LB_GETTEXT,i,(LPARAM) szBuffer);
	OnNewValue(szBuffer);
	byData = (BYTE) _tcstoul(szBuffer,NULL,16);
	byData = (byData >> 4) | (byData << 4);	// change nibbles for writing

	Write2(dwAddress,byData);				// write data
	ViewCodeWnd(hWndCode,dwAdrLine[0]);		// update code window
	ViewMemWnd(hDlg,dwAdrMem);				// update memory window
	SendMessage(hWnd,LB_SETCURSEL,i,0);
	return FALSE;
}

//
// request for context menu
//
static VOID OnContextMenu(HWND hDlg, LPARAM lParam, WPARAM wParam)
{
	POINT pt;
	INT   nId;

	POINTSTOPOINT(pt,MAKEPOINTS(lParam));	// mouse position

	if (pt.x == -1 && pt.y == -1)			// VK_APPS
	{
		RECT rc;

		GetWindowRect((HWND) wParam,&rc);	// get position of active window
		pt.x = rc.left + 5;
		pt.y = rc.top + 5;
	}

	nId = GetDlgCtrlID((HWND) wParam);		// control ID of window

	switch(nId)
	{
	case IDC_DEBUG_CODE: // handle code window
		TrackPopupMenu(hMenuCode,0,pt.x,pt.y,0,hDlg,NULL);
		break;

	case IDC_DEBUG_MEM_COL0:
	case IDC_DEBUG_MEM_COL1:
	case IDC_DEBUG_MEM_COL2:
	case IDC_DEBUG_MEM_COL3:
	case IDC_DEBUG_MEM_COL4:
	case IDC_DEBUG_MEM_COL5:
	case IDC_DEBUG_MEM_COL6:
	case IDC_DEBUG_MEM_COL7: // handle memory window
		TrackPopupMenu(hMenuMem,0,pt.x,pt.y,0,hDlg,NULL);
		break;

	case IDC_DEBUG_STACK: // handle stack window
		TrackPopupMenu(hMenuStack,0,pt.x,pt.y,0,hDlg,NULL);
		break;
	}
	return;
}

//
// mouse setting for capturing window
//
static BOOL OnSetCursor(HWND hDlg)
{
	// debugger not active but cursor is over debugger window
	if (bActFollowsMouse && GetActiveWindow() != hDlg)
	{
		// force debugger window to foreground
		ForceForegroundWindow(GetLastActivePopup(hDlg));
	}
	return FALSE;
}

//################
//#
//#    Dialog handler
//#
//################

//
// handle right/left keys in memory window
//
static __inline BOOL OnKeyRightLeft(HWND hWnd, WPARAM wParam)
{
	HWND hWndNew;
	WORD wX;
	INT  nId;

	nId = GetDlgCtrlID(hWnd);				// control ID of window

	for (wX = 0; wX < MEMWNDMAX; ++wX)		// scan all Id's
		if (nCol[wX] == nId)				// found ID
			break;

	if (wX == MEMWNDMAX) return -1;			// not IDC_DEBUG_MEM window, default handler

	// new position
	wX = ((LOWORD(wParam) == VK_RIGHT) ? (wX + 1) : (wX + MEMWNDMAX - 1)) % MEMWNDMAX;

	// set new focus
	hWndNew = GetDlgItem(GetParent(hWnd),nCol[wX]);
	SendMessage(hWndNew,LB_SETCURSEL,HIWORD(wParam),0);
	SetFocus(hWndNew);
	return -2;
}

//
// handle (page) up/down keys in memory window
//
static __inline BOOL OnKeyUpDown(HWND hWnd, WPARAM wParam)
{
	INT wX, wY;
	INT nId;

	nId = GetDlgCtrlID(hWnd);				// control ID of window

	for (wX = 0; wX < MEMWNDMAX; ++wX)		// scan all Id's
		if (nCol[wX] == nId)				// found ID
			break;

	if (wX == MEMWNDMAX) return -1;			// not IDC_DEBUG_MEM window, default handler

	wY = HIWORD(wParam);					// get old focus

	switch(LOWORD(wParam))
	{
	case VK_NEXT:
		dwAdrMem = (dwAdrMem + MAXMEMITEMS * MAXMEMLINES) & GetMemDataMask();
		ViewMemWnd(GetParent(hWnd),dwAdrMem);
		SendMessage(hWnd,LB_SETCURSEL,wY,0);
		return -2;

	case VK_PRIOR:
		dwAdrMem = (dwAdrMem - MAXMEMITEMS * MAXMEMLINES) & GetMemDataMask();
		ViewMemWnd(GetParent(hWnd),dwAdrMem);
		SendMessage(hWnd,LB_SETCURSEL,wY,0);
		return -2;

	case VK_DOWN:
		if (wY+1 >= MAXMEMLINES)
		{
			dwAdrMem = (dwAdrMem + MAXMEMITEMS) & GetMemDataMask();
			ViewMemWnd(GetParent(hWnd),dwAdrMem);
			SendMessage(hWnd,LB_SETCURSEL,wY,0);
			return -2;
		}
		break;

	case VK_UP:
		if (wY == 0)
		{
			dwAdrMem = (dwAdrMem - MAXMEMITEMS) & GetMemDataMask();
			ViewMemWnd(GetParent(hWnd),dwAdrMem);
			SendMessage(hWnd,LB_SETCURSEL,wY,0);
			return -2;
		}
		break;
	}
	return -1;
}

//
// handle (page) +/- keys in memory window
//
static __inline BOOL OnKeyPlusMinus(HWND hWnd, WPARAM wParam)
{
	INT wX, wY;
	INT nId;

	nId = GetDlgCtrlID(hWnd);				// control ID of window

	for (wX = 0; wX < MEMWNDMAX; ++wX)		// scan all Id's
		if (nCol[wX] == nId)				// found ID
			break;

	if (wX == MEMWNDMAX) return -1;			// not IDC_DEBUG_MEM window, default handler

	wY = HIWORD(wParam);					// get focus

	if (LOWORD(wParam) == VK_ADD)			// move start address of memory view
		dwAdrMem++;
	else
		dwAdrMem--;
	dwAdrMem &= GetMemDataMask();

	ViewMemWnd(GetParent(hWnd),dwAdrMem);	// redraw memory view
	SendMessage(hWnd,LB_SETCURSEL,wY,0);	// set focus at old position
	return -1;
}

//
// handle keys in code window
//
static __inline BOOL OnKeyCodeWnd(HWND hDlg, WPARAM wParam)
{
	HWND hWnd  = GetDlgItem(hDlg,IDC_DEBUG_CODE);
	WORD wKey  = LOWORD(wParam);
	WORD wItem = HIWORD(wParam);

	// down key on last line
	if ((wKey == VK_DOWN || wKey == VK_NEXT) && wItem == MAXCODELINES - 1)
	{
		WORD i = ((dwAdrLine[0] & CODELABEL) != 0) ? 2 : 1;

		ViewCodeWnd(hWnd,dwAdrLine[i]);
		SendMessage(hWnd,LB_SETCURSEL,wItem-i,0);
	}
	// up key on first line
	if ((wKey == VK_UP || wKey == VK_PRIOR) && wItem == 0)
	{
		ViewCodeWnd(hWnd,dwAdrLine[0]-1);
	}

	if (wKey == _T('G')) return OnCodeGoAdr(GetParent(hWnd)); // goto new address

	return -1;								// process key
}

//
// handle drawing in code window
//
static __inline BOOL OnDrawCodeWnd(LPDRAWITEMSTRUCT lpdis)
{
	TCHAR    szBuf[64];
	COLORREF crBkColor;
	COLORREF crTextColor;
	HFONT    hFont;
	BOOL     bBrk,bPC,bLabel;

	if (lpdis->itemID == -1)				// no item in list box
		return TRUE;

	// get item text
	SendMessage(lpdis->hwndItem,LB_GETTEXT,lpdis->itemID,(LPARAM) szBuf);

	// line is a label
	bLabel = ((dwAdrLine[lpdis->itemID] & CODELABEL) != 0);

	if (!bLabel)
	{
		// check for codebreakpoint
		bBrk = CheckBreakpoint(dwAdrLine[lpdis->itemID],1,BP_EXEC);
		bPC = szBuf[5] != _T(' ');			// check if line of program counter

		crTextColor = COLOR_WHITE;			// standard text color

		if (lpdis->itemState & ODS_SELECTED) // cursor line
		{
			if (bPC)						// PC line
			{
				crBkColor = bBrk ? COLOR_DKGRAY : COLOR_TEAL;
			}
			else							// normal line
			{
				crBkColor = bBrk ? COLOR_PURPLE : COLOR_NAVY;
			}
		}
		else								// not cursor line
		{
			if (bPC)						// PC line
			{
				crBkColor = bBrk ? COLOR_OLIVE : COLOR_GREEN;
			}
			else							// normal line
			{
				if (bBrk)
				{
					crBkColor   = COLOR_MAROON;
				}
				else
				{
					crBkColor   = COLOR_WHITE;
					crTextColor = COLOR_BLACK;
				}
			}
		}
	}
	else									// label
	{
		crBkColor   = COLOR_WHITE;
		crTextColor = COLOR_NAVY;
		hFont = (HFONT) SelectObject(lpdis->hDC,hFontBold);
	}

	// write Text
	crBkColor   = SetBkColor(lpdis->hDC,crBkColor);
	crTextColor = SetTextColor(lpdis->hDC,crTextColor);

	ExtTextOut(lpdis->hDC,(int)(lpdis->rcItem.left)+2,(int)(lpdis->rcItem.top),
			   ETO_OPAQUE,(LPRECT)&lpdis->rcItem,szBuf,lstrlen(szBuf),NULL);

	SetBkColor(lpdis->hDC,crBkColor);
	SetTextColor(lpdis->hDC,crTextColor);

	if (bLabel) SelectObject(lpdis->hDC,hFont);

	if (lpdis->itemState & ODS_FOCUS)		// redraw focus
		DrawFocusRect(lpdis->hDC,&lpdis->rcItem);

	return TRUE;							// focus handled here
}

//
// detect changed register
//
static __inline BOOL OnCtlColorStatic(HWND hWnd)
{
	BOOL bError = FALSE;					// not changed

	int nId = GetDlgCtrlID(hWnd);
	if (nId >= REG_START && nId <= REG_STOP) // in register area
		bError = bRegUpdate[nId-REG_START];	// register changed?
	return bError;
}


//################
//#
//#    Public functions
//#
//################

//
// handle upper 32 bit of cpu cycle counter
//
VOID UpdateDbgCycleCounter(VOID)
{
	// update 64 bit cpu cycle counter
	if (Chipset.cycles < dwDbgRefCycles) ++Chipset.cycles_reserved;
	dwDbgRefCycles = (DWORD) (Chipset.cycles & 0xFFFFFFFF);
	return;
}

//
// check for breakpoints
//
BOOL CheckBreakpoint(DWORD dwAddr, DWORD dwRange, UINT nType)
{
	INT i;

	for (i = 0; i < wBreakpointCount; ++i)	// scan all breakpoints
	{
		// check address range and type
		if (   sBreakpoint[i].bEnable
			&& sBreakpoint[i].dwAddr >= dwAddr && sBreakpoint[i].dwAddr < dwAddr + dwRange
			&& (sBreakpoint[i].nType & nType) != 0)
			return TRUE;
	}
	return FALSE;
}

//
// notify debugger that emulation stopped
//
VOID NotifyDebugger(INT nType)				// update registers
{
	nRplBreak = nType;						// save breakpoint type
	FlushTrace();							// flush trace buffer
	_ASSERT(hDlgDebug);						// debug dialog box open
	PostMessage(hDlgDebug,WM_UPDATE,0,0);
	return;
}

//
// disable debugger
//
VOID DisableDebugger(VOID)
{
	if (hDlgDebug)							// debugger running
		DestroyWindow(hDlgDebug);			// then close debugger to renter emulation
	return;
}


//################
//#
//#    Debugger Message loop
//#
//################

//
// ID_TOOL_DEBUG
//
static __inline HWND CreateToolbar(HWND hWnd)
{
	HRSRC        hRes;
	HGLOBAL      hGlobal;
	CToolBarData *pData;
	TBBUTTON     *ptbb;
	INT          i,j;

	HWND hWndToolbar = NULL;				// toolbar window

	InitCommonControls();					// ensure that common control DLL is loaded

	if ((hRes = FindResource(hApp,MAKEINTRESOURCE(IDR_DEBUG_TOOLBAR),RT_TOOLBAR)) == NULL)
		goto quit;

	if ((hGlobal = LoadResource(hApp,hRes)) == NULL)
		goto quit;

	if ((pData = (CToolBarData*) LockResource(hGlobal)) == NULL)
		goto unlock;

	_ASSERT(pData->wVersion == 1);			// toolbar resource version

	// alloc memory for TBBUTTON stucture
	if (!(ptbb = (TBBUTTON *) malloc(pData->wItemCount*sizeof(TBBUTTON))))
		goto unlock;

	// fill TBBUTTON stucture with resource data
	for (i = j = 0; i < pData->wItemCount; ++i)
	{
		if (pData->aItems[i])
		{
			ptbb[i].iBitmap = j++;
			ptbb[i].fsStyle = TBSTYLE_BUTTON;
		}
		else
		{
			ptbb[i].iBitmap = 5;			// separator width
			ptbb[i].fsStyle = TBSTYLE_SEP;
		}
		ptbb[i].idCommand = pData->aItems[i];
		ptbb[i].fsState = TBSTATE_ENABLED;
		ptbb[i].dwData  = 0;
		ptbb[i].iString = j;
	}

	hWndToolbar = CreateToolbarEx(hWnd,WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS,
								  IDR_DEBUG_TOOLBAR,j,hApp,IDR_DEBUG_TOOLBAR,ptbb,pData->wItemCount,
								  pData->wWidth,pData->wHeight,pData->wWidth,pData->wHeight,
								  sizeof(TBBUTTON));

	free(ptbb);

unlock:
	FreeResource(hGlobal);
quit:
	return hWndToolbar;
}

static INT_PTR CALLBACK Debugger(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HMENU hMenuMainCode,hMenuMainMem,hMenuMainStack;

	WINDOWPLACEMENT wndpl;
	TEXTMETRIC tm;
	HDC   hDC;
	HFONT hFont;
	HMENU hSysMenu;
	HMENU hDbgMenu;
	INT   i;

	switch (message)
	{
	case WM_INITDIALOG:
		if (nDbgPosX != CW_USEDEFAULT)		// not default window position
		{
			SetWindowLocation(hDlg,nDbgPosX,nDbgPosY);
		}
		if (bAlwaysOnTop) SetWindowPos(hDlg,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE);
		SendMessage(hDlg,WM_SETICON,ICON_BIG,(LPARAM) LoadIcon(hApp,MAKEINTRESOURCE(IDI_EMU48)));

		bDbgTrace = FALSE;					// disable file trace

		// load file trace settings
		ReadSettingsString(_T("Debugger"),_T("TraceFile"),szTraceFilename,szTraceFilename,ARRAYSIZEOF(szTraceFilename));
		uTraceMode = ReadSettingsInt(_T("Debugger"),_T("TraceFileMode"),uTraceMode);
		bTraceReg  = ReadSettingsInt(_T("Debugger"),_T("TraceRegister"),bTraceReg);
		bTraceMmu  = ReadSettingsInt(_T("Debugger"),_T("TraceMMU"),bTraceMmu);
		bTraceOpc  = ReadSettingsInt(_T("Debugger"),_T("TraceOpcode"),bTraceOpc);

		// add Settings item to sysmenu
		_ASSERT((IDM_DEBUG_SETTINGS & 0xFFF0) == IDM_DEBUG_SETTINGS);
		_ASSERT(IDM_DEBUG_SETTINGS < 0xF000);
		if ((hSysMenu = GetSystemMenu(hDlg,FALSE)) != NULL)
		{
			VERIFY(AppendMenu(hSysMenu,MF_SEPARATOR,0,NULL));
			VERIFY(AppendMenu(hSysMenu,MF_STRING,IDM_DEBUG_SETTINGS,_T("Debugger Settings...")));
		}

		hDbgMenu = GetMenu(hDlg);			// menu of debugger dialog
		hWndToolbar = CreateToolbar(hDlg);	// add toolbar

		CheckMenuItem(hDbgMenu,ID_BREAKPOINTS_NOP3,  bDbgNOP3    ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(hDbgMenu,ID_BREAKPOINTS_DOCODE,bDbgCode    ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(hDbgMenu,ID_BREAKPOINTS_RPL,   bDbgRPL     ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(hDbgMenu,ID_INTR_STEPOVERINT,  bDbgSkipInt ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(hDbgMenu,ID_TRACE_ENABLE,      bDbgTrace   ? MF_CHECKED : MF_UNCHECKED);

		EnableMenuItem(hDbgMenu,ID_TRACE_SETTINGS,bDbgTrace ? MF_GRAYED : MF_ENABLED);

		hDlgDebug = hDlg;					// handle for debugger dialog
		hEventDebug = CreateEvent(NULL,FALSE,FALSE,NULL);
		if (hEventDebug == NULL)
		{
			AbortMessage(_T("Event creation failed !"));
			return TRUE;
		}
		hMenuMainCode = LoadMenu(hApp,MAKEINTRESOURCE(IDR_DEBUG_CODE));
		_ASSERT(hMenuMainCode);
		hMenuCode = GetSubMenu(hMenuMainCode, 0);
		_ASSERT(hMenuCode);
		hMenuMainMem = LoadMenu(hApp,MAKEINTRESOURCE(IDR_DEBUG_MEM));
		_ASSERT(hMenuMainMem);
		hMenuMem = GetSubMenu(hMenuMainMem, 0);
		_ASSERT(hMenuMem);
		hMenuMainStack = LoadMenu(hApp,MAKEINTRESOURCE(IDR_DEBUG_STACK));
		_ASSERT(hMenuMainStack);
		hMenuStack = GetSubMenu(hMenuMainStack, 0);
		_ASSERT(hMenuStack);

		// bold font for symbol labels in code window
		hDC = GetDC(hDlg);
		VERIFY(hFontBold = CreateFont(-MulDiv(8,GetDeviceCaps(hDC,LOGPIXELSY),72),0,0,0,FW_BOLD,0,0,0,ANSI_CHARSET,
									  OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,FF_DONTCARE,_T("Arial")));
		ReleaseDC(hDlg,hDC);

		// font settings
		SendDlgItemMessage(hDlg,IDC_STATIC_CODE,     WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),MAKELPARAM(FALSE,0));
		SendDlgItemMessage(hDlg,IDC_STATIC_REGISTERS,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),MAKELPARAM(FALSE,0));
		SendDlgItemMessage(hDlg,IDC_STATIC_MEMORY,   WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),MAKELPARAM(FALSE,0));
		SendDlgItemMessage(hDlg,IDC_STATIC_STACK,    WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),MAKELPARAM(FALSE,0));
		SendDlgItemMessage(hDlg,IDC_STATIC_MMU,      WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),MAKELPARAM(FALSE,0));
		SendDlgItemMessage(hDlg,IDC_STATIC_MISC,     WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),MAKELPARAM(FALSE,0));

		// init last instruction circular buffer
		pdwInstrArray = (DWORD *) malloc(wInstrSize*sizeof(*pdwInstrArray));
		wInstrWp = wInstrRp = 0;			// write/read pointer

		// init "Follow" menu entry in debugger "Memory" context menu
		CheckMenuItem(hMenuMem,uIDFol,MF_CHECKED);

		LoadSymbTable();					// load external rpl symbol table

		InitMemMap(hDlg);					// init memory mapping table
		InitBsArea(hDlg);					// init bank switcher list box
		DisableMenuKeys(hDlg);				// set debug menu keys into run state

		fnOutTrace = OutTrace;				// function for file trace
		RplReadNibble = GetMemNib;			// get nibble function for RPL object viewer

		dwDbgStopPC = -1;					// no stop address for goto cursor
		dwDbgRplPC = -1;					// no stop address for RPL breakpoint

		// init reference cpu cycle counter for 64 bit debug cycle counter
		dwDbgRefCycles = (DWORD) (Chipset.cycles & 0xFFFFFFFF);

		nDbgState = DBG_STEPINTO;			// state "step into"
		if (Chipset.Shutdn)					// cpu thread stopped
			SetEvent(hEventShutdn);			// goto debug session

		OldChipset = Chipset;				// save chipset values
		return TRUE;

	case WM_DESTROY:
		// SetHP48Time();					// update time & date
		nDbgState = DBG_OFF;				// debugger inactive
		StopTrace();						// finish trace
		bInterrupt = TRUE;					// exit opcode loop
		SetEvent(hEventDebug);
		if (pdwInstrArray)					// free last instruction circular buffer
		{
			EnterCriticalSection(&csDbgLock);
			{
				free(pdwInstrArray);
				pdwInstrArray = NULL;
			}
			LeaveCriticalSection(&csDbgLock);
		}
		CloseHandle(hEventDebug);
		wndpl.length = sizeof(wndpl);		// save debugger window position
		GetWindowPlacement(hDlg, &wndpl);
		nDbgPosX = wndpl.rcNormalPosition.left;
		nDbgPosY = wndpl.rcNormalPosition.top;

		// save file trace settings
		WriteSettingsString(_T("Debugger"),_T("TraceFile"),szTraceFilename);
		WriteSettingsInt(_T("Debugger"),_T("TraceFileMode"),uTraceMode);
		WriteSettingsInt(_T("Debugger"),_T("TraceRegister"),bTraceReg);
		WriteSettingsInt(_T("Debugger"),_T("TraceMMU"),bTraceMmu);
		WriteSettingsInt(_T("Debugger"),_T("TraceOpcode"),bTraceOpc);

		RplDeleteTable();					// delete rpl symbol table
		DeleteObject(hFontBold);			// delete bold font
		DestroyMenu(hMenuMainCode);
		DestroyMenu(hMenuMainMem);
		DestroyMenu(hMenuMainStack);
		hDlgDebug = NULL;					// debugger windows closed
		break;

	case WM_CLOSE:
		DestroyWindow(hDlg);
		break;

	case WM_UPDATE:
		OnUpdate(hDlg);
		return TRUE;

	case WM_SYSCOMMAND:
		if ((wParam & 0xFFF0) == IDM_DEBUG_SETTINGS)
		{
			return OnSettings(hDlg);
		}
		break;

	case WM_COMMAND:
		switch (HIWORD(wParam))
		{
		case LBN_DBLCLK:
			return OnDblClick((HWND) lParam, LOWORD(wParam));

		case LBN_SETFOCUS:
			i = (INT) SendMessage((HWND) lParam,LB_GETCARETINDEX,0,0);
			SendMessage((HWND) lParam,LB_SETCURSEL,i,0);
			return TRUE;

		case LBN_KILLFOCUS:
			SendMessage((HWND) lParam,LB_SETCURSEL,-1,0);
			return TRUE;
		}

		switch (LOWORD(wParam))
		{
		case ID_BREAKPOINTS_SETBREAK:     return OnKeyF2(hDlg);
		case ID_DEBUG_RUN:                return OnKeyF5(hDlg);
		case ID_DEBUG_RUNCURSOR:          return OnKeyF6(hDlg);
		case ID_DEBUG_STEP:               return OnKeyF7(hDlg);
		case ID_DEBUG_STEPOVER:           return OnKeyF8(hDlg);
		case ID_DEBUG_STEPOUT:            return OnKeyF9(hDlg);
		case ID_DEBUG_BREAK:              return OnKeyF11(hDlg);
		case ID_DEBUG_CODE_GOADR:         return OnCodeGoAdr(hDlg);
		case ID_DEBUG_CODE_GOPC:          return OnCodeGoPC(hDlg);
		case ID_DEBUG_CODE_SETPCTOSELECT: return OnCodeSetPcToSelection(hDlg);
		case ID_DEBUG_CODE_PREVPCO:       return OnCodeFindPCO(hDlg,dwAdrLine[0],-1);
		case ID_DEBUG_CODE_NEXTPCO:       return OnCodeFindPCO(hDlg,dwAdrLine[0],1);
		case ID_BREAKPOINTS_CODEEDIT:     return OnEditBreakpoint(hDlg);
		case ID_BREAKPOINTS_CLEARALL:     return OnClearAll(hDlg);
		case ID_BREAKPOINTS_NOP3:         return OnToggleMenuItem(hDlg,LOWORD(wParam),&bDbgNOP3);
		case ID_BREAKPOINTS_DOCODE:       return OnToggleMenuItem(hDlg,LOWORD(wParam),&bDbgCode);
		case ID_BREAKPOINTS_RPL:          return OnToggleMenuItem(hDlg,LOWORD(wParam),&bDbgRPL);
		case ID_TRACE_SETTINGS:           return OnTraceSettings(hDlg);
		case ID_TRACE_ENABLE:             return OnTraceEnable(hDlg);
		case ID_INFO_LASTINSTRUCTIONS:    return OnInfoIntr(hDlg);
		case ID_INFO_PROFILE:             return OnProfile(hDlg);
		case ID_INFO_WRITEONLYREG:        return OnInfoWoRegister(hDlg);
		case ID_INTR_STEPOVERINT:         return OnToggleMenuItem(hDlg,LOWORD(wParam),&bDbgSkipInt);
		case ID_DEBUG_MEM_GOADR:          return OnMemGoAdr(hDlg);
		case ID_DEBUG_MEM_GOPC:           return OnMemGoDx(hDlg,Chipset.pc);
		case ID_DEBUG_MEM_GOD0:           return OnMemGoDx(hDlg,Chipset.d0);
		case ID_DEBUG_MEM_GOD1:           return OnMemGoDx(hDlg,Chipset.d1);
		case ID_DEBUG_MEM_GOSTACK:        return OnMemGoDx(hDlg,Chipset.rstk[(Chipset.rstkp-1)&7]);
		case ID_DEBUG_MEM_FNONE:
		case ID_DEBUG_MEM_FADDR:
		case ID_DEBUG_MEM_FPC:
		case ID_DEBUG_MEM_FD0:
		case ID_DEBUG_MEM_FD1:            return OnMemFollow(hDlg,LOWORD(wParam));
		case ID_DEBUG_MEM_FIND:           return OnMemFind(hDlg);
		case ID_DEBUG_MEM_MAP:
		case ID_DEBUG_MEM_NCE1:
		case ID_DEBUG_MEM_NCE2:
		case ID_DEBUG_MEM_CE1:
		case ID_DEBUG_MEM_CE2:
		case ID_DEBUG_MEM_NCE3:           return OnMemMapping(hDlg,LOWORD(wParam));
		case ID_DEBUG_MEM_LOAD:           return OnMemLoadData(hDlg);
		case ID_DEBUG_MEM_SAVE:           return OnMemSaveData(hDlg);
		case ID_DEBUG_MEM_RPLVIEW:        return OnRplObjView(hDlg);
		case ID_DEBUG_STACK_PUSH:         return OnStackPush(hDlg);
		case ID_DEBUG_STACK_POP:          return OnStackPop(hDlg);
		case ID_DEBUG_STACK_MODIFY:       return OnStackModify(hDlg);
		case ID_DEBUG_CANCEL:             DestroyWindow(hDlg); return TRUE;
		}
		break;

	case WM_VKEYTOITEM:
		switch (LOWORD(wParam))				// always valid
		{
		case VK_F2:  return OnKeyF2(hDlg);	// toggle breakpoint
		case VK_F5:  return OnKeyF5(hDlg);	// key run
		case VK_F6:  return OnKeyF6(hDlg);	// key step cursor
		case VK_F7:  return OnKeyF7(hDlg);	// key step into
		case VK_F8:  return OnKeyF8(hDlg);	// key step over
		case VK_F9:  return OnKeyF9(hDlg);	// key step out
		case VK_F11: return OnKeyF11(hDlg);	// key break
		}

		switch(GetDlgCtrlID((HWND) lParam))	// calling window
		{
		// handle code window
		case IDC_DEBUG_CODE:
			return OnKeyCodeWnd(hDlg, wParam);

		// handle memory window
		case IDC_DEBUG_MEM_COL0:
		case IDC_DEBUG_MEM_COL1:
		case IDC_DEBUG_MEM_COL2:
		case IDC_DEBUG_MEM_COL3:
		case IDC_DEBUG_MEM_COL4:
		case IDC_DEBUG_MEM_COL5:
		case IDC_DEBUG_MEM_COL6:
		case IDC_DEBUG_MEM_COL7:
			switch (LOWORD(wParam))
			{
			case _T('G'):     return OnMemGoAdr(GetParent((HWND) lParam));
			case _T('F'):     return OnMemFind(GetParent((HWND) lParam));
			case VK_RIGHT:
			case VK_LEFT:     return OnKeyRightLeft((HWND) lParam, wParam);
			case VK_NEXT:
			case VK_PRIOR:
			case VK_DOWN:
			case VK_UP:       return OnKeyUpDown((HWND) lParam, wParam);
			case VK_ADD:
			case VK_SUBTRACT: return OnKeyPlusMinus((HWND) lParam, wParam);
			}
			break;
		}
		return -1;							// default action

	case WM_LBUTTONUP:
		return OnLButtonUp(hDlg, lParam);

	case WM_CONTEXTMENU:
		OnContextMenu(hDlg, lParam, wParam);
		break;

	case WM_SETCURSOR:
		return OnSetCursor(hDlg);

	case WM_CTLCOLORSTATIC:					// register color highlighting
		// highlight text?
		if (OnCtlColorStatic((HWND) lParam))
		{
			SetTextColor((HDC) wParam, COLOR_RED);
			SetBkColor((HDC) wParam, GetSysColor(COLOR_BTNFACE));
			return (INT_PTR) GetStockObject(NULL_BRUSH); // transparent brush
		}
		break;

	case WM_NOTIFY:
		// tooltip for toolbar
		if (((LPNMHDR) lParam)->code == TTN_GETDISPINFO)
		{
			((LPTOOLTIPTEXT) lParam)->hinst = hApp;
			((LPTOOLTIPTEXT) lParam)->lpszText = MAKEINTRESOURCE(((LPTOOLTIPTEXT) lParam)->hdr.idFrom);
			break;
		}
		break;

	case WM_DRAWITEM:
		if (wParam == IDC_DEBUG_CODE) return OnDrawCodeWnd((LPDRAWITEMSTRUCT) lParam);
		break;

	case WM_MEASUREITEM:
		hDC = GetDC(hDlg);

		// GetTextMetrics from "Courier New 8" font
		hFont = CreateFont(-MulDiv(8,GetDeviceCaps(hDC, LOGPIXELSY),72),0,0,0,FW_NORMAL,0,0,0,ANSI_CHARSET,
						   OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,FF_DONTCARE,_T("Courier New"));

		hFont = (HFONT) SelectObject(hDC,hFont);
		GetTextMetrics(hDC,&tm);
		hFont = (HFONT) SelectObject(hDC,hFont);
		DeleteObject(hFont);

		((LPMEASUREITEMSTRUCT) lParam)->itemHeight = tm.tmHeight;
		lCharWidth = tm.tmAveCharWidth;

		ReleaseDC(hDlg,hDC);
		return TRUE;
	}
	return FALSE;
}

LRESULT OnToolDebug(VOID)					// debugger dialogbox call
{
	if ((hDlgDebug = CreateDialog(hApp,MAKEINTRESOURCE(IDD_DEBUG),hWnd,
		(DLGPROC)Debugger)) == NULL)
		AbortMessage(_T("Debugger Dialog Box Creation Error !"));
	return 0;
}


//################
//#
//#    Find dialog box
//#
//################

static __inline BOOL OnFindOK(HWND hDlg,BOOL bASCII,DWORD *pdwAddrLast,INT nSearchDir)
{
	HWND  hWnd;
	BYTE  *lpbySearch;
	INT   i,j;
	DWORD dwCnt,dwAddr,dwMapDataMask;
	BOOL  bMatch;

	// searching upwards / downwards
	_ASSERT(nSearchDir == 1 || nSearchDir == -1);

	hWnd = GetDlgItem(hDlg,IDC_FIND_DATA);

	dwMapDataMask = GetMemDataMask();		// size mask of data mapping

	i = GetWindowTextLength(hWnd) + 1;		// text length incl. EOS
	j = bASCII ? 2 : sizeof(*(LPTSTR)0);	// buffer width

	// allocate search buffer (min 5 bytes for symbolic entry address)
	if ((lpbySearch = (LPBYTE)malloc(__max(5, i) * j)) != NULL)
	{
		BOOL bSymbolic;

		// get search text and real length
		i = GetWindowText(hWnd,(LPTSTR) lpbySearch,i);

		// add string to combo box
		if (SendMessage(hWnd,CB_FINDSTRINGEXACT,0,(LPARAM) lpbySearch) == CB_ERR)
			SendMessage(hWnd,CB_ADDSTRING,0,(LPARAM) lpbySearch);

		// is symbolic entry address
		bSymbolic = !bASCII && disassembler_symb
			&& i > 0 && *(LPCTSTR)lpbySearch == _T('=')
			&& !RplGetAddr(((LPCTSTR)lpbySearch)+1,&dwAddr);

		#if defined _UNICODE
		{
			// Unicode to byte translation
			LPTSTR szTmp = DuplicateString((LPCTSTR) lpbySearch);
			if (szTmp != NULL)
			{
				WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK,
									szTmp, -1,
									(LPSTR) lpbySearch, i+1, NULL, NULL);
				free(szTmp);
			}
		}
		#endif

		// convert input format to nibble based format
		if (bASCII)							// ASCII input
		{
			// convert ASCII to number
			for (j = i - 1; j >= 0; --j)
			{
				// order LSB, MSB
				lpbySearch[j * 2 + 1] = lpbySearch[j] >> 4;
				lpbySearch[j * 2]     = lpbySearch[j] & 0xF;
			}
			i *= 2;							// no. of nibbles
		}
		else								// symbolic entry or hex number input
		{
			if (bSymbolic)
			{
				i = 5;						// 5 nibbles to compare
				Nunpack(lpbySearch,dwAddr,i);
			}
			else
			{
				// convert HEX to number
				for (i = 0, j = 0; lpbySearch[j] != 0; ++j)
				{
					if (lpbySearch[j] == ' ') // skip space
						continue;

					if (isxdigit(lpbySearch[j]))
					{
						lpbySearch[i] = toupper(lpbySearch[j]) - '0';
						if (lpbySearch[i] > 9) lpbySearch[i] -= 'A' - '9' - 1;
					}
					else
					{
						i = 0;				// wrong format, no match
						break;
					}
					++i;					// inc, no. of nibbles
				}
			}
		}

		bMatch = FALSE;						// no match
		dwAddr = dwAdrMem;					// calculate search start address
		if (*pdwAddrLast == dwAddr)
			dwAddr += nSearchDir;

		// scan mapping/module until match
		for (dwCnt = 0; i && !bMatch && dwCnt <= dwMapDataMask; ++dwCnt)
		{
			BYTE byC;

			// i = no. of nibbles that have to match
			for (bMatch = TRUE, j = 0;bMatch && j < i; ++j)
			{
				GetMemPeek(&byC,(dwAddr + j) & dwMapDataMask,1);
				bMatch = (byC == lpbySearch[j]);
			}
			dwAddr += nSearchDir;
		}
		free(lpbySearch);

		// check match result
		if (bMatch)
		{
			// matching address
			dwAddr = (dwAddr - nSearchDir) & dwMapDataMask;

			// update memory window
			OnMemGoDx(GetParent(hDlg),dwAddr);

			// update rpl obj view dialog
			UpdateRplObjViewWnd(hDlgRplObjView,dwAddr);

			*pdwAddrLast = dwAddr;			// last matching address
		}
		else
		{
			MessageBox(hDlg,_T("Search string not found!"),_T("Find"),
					   MB_APPLMODAL|MB_OK|MB_ICONEXCLAMATION|MB_SETFOREGROUND);
		}
	}
	return TRUE;
}

//
// enter find dialog
//
static INT_PTR CALLBACK Find(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static DWORD dwAddrEntry;
	static BOOL  bASCII = FALSE;

	switch (message)
	{
	case WM_INITDIALOG:
		CheckDlgButton(hDlg,IDC_FIND_ASCII,bASCII);
		dwAddrEntry = -1;
		return TRUE;

	case WM_DESTROY:
		hDlgFind = NULL;
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_FIND_ASCII: bASCII = !bASCII; return TRUE;
		case IDC_FIND_PREV:  return OnFindOK(hDlg,bASCII,&dwAddrEntry,-1);
		case IDC_FIND_NEXT:  return OnFindOK(hDlg,bASCII,&dwAddrEntry,1);
		case IDCANCEL:       DestroyWindow(hDlg); return TRUE;
		}
		break;
	}
	return FALSE;
	UNREFERENCED_PARAMETER(lParam);
}

static BOOL OnMemFind(HWND hDlg)
{
	if (hDlgFind == NULL)					// no find dialog, create it
	{
		if ((hDlgFind = CreateDialog(hApp,MAKEINTRESOURCE(IDD_FIND),hDlg,
			(DLGPROC)Find)) == NULL)
			AbortMessage(_T("Find Dialog Box Creation Error !"));
	}
	else
	{
		SetFocus(hDlgFind);					// set focus on find dialog
	}
	return -1;								// call windows default handler
}


//################
//#
//#    Profile dialog box
//#
//################

//
// update profiler dialog content
//
static VOID UpdateProfileWnd(HWND hDlg)
{
	#define CPU_FREQ 524288					// base CPU frequency
	#define SX_RATE  0x0E
	#define GX_RATE  0x1B
	#define GP_RATE  0x1B*3 // CdB for HP: add high speed apples
	#define G2_RATE  0x1B*2 // CdB for HP: add low speed apples

	LPCTSTR pcUnit[] = { _T("s"),_T("ms"),_T("us"),_T("ns") };

	QWORD lVar;
	TCHAR szBuffer[64];
	UINT  i;
	DWORD dwFreq, dwEndFreq;

	if (hDlg == NULL) return;				// dialog not open

	// 64 bit cpu cycle counter
	lVar = *(QWORD *)&Chipset.cycles - *(QWORD *)&OldChipset.cycles;

	// cycle counts
	_sntprintf(szBuffer,ARRAYSIZEOF(szBuffer),_T("%I64u"),lVar);
	SetDlgItemText(hDlg,IDC_PROFILE_LASTCYCLES,szBuffer);

	// CPU frequency
	switch (cCurrentRomType) // CdB for HP: add apples speed selection
	{
	  case 'S': dwFreq= ((SX_RATE + 1) * CPU_FREQ / 4); break;
	  case 'X': case 'G': case 'E': case 'A': dwFreq= ((GX_RATE + 1) * CPU_FREQ / 4); break;
	  case 'P': case 'Q': dwFreq= ((GP_RATE + 1) * CPU_FREQ / 4); break;
	  case '2': dwFreq= ((G2_RATE + 1) * CPU_FREQ / 4); break;
	}
	dwEndFreq = ((999 * 2 - 1) * dwFreq) / (2 * 1000);

	// search for ENG unit
	for (i = 0; i < ARRAYSIZEOF(pcUnit) - 1 && lVar <= dwEndFreq; ++i)
	{
		lVar *= 1000;						// next ENG unit
	}

	// calculate rounded time
	lVar = (2 * lVar + dwFreq) / (2 * dwFreq);

	_ASSERT(i < ARRAYSIZEOF(pcUnit));
	_sntprintf(szBuffer,ARRAYSIZEOF(szBuffer),_T("%I64u %s"),lVar,pcUnit[i]);
	SetDlgItemText(hDlg,IDC_PROFILE_LASTTIME,szBuffer);
	return;
	#undef SX_CLK
	#undef GX_CLK
	#undef GP_RATE
	#undef G2_RATE
	#undef CPU_FREQ
}

//
// enter profile dialog
//
static INT_PTR CALLBACK Profile(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		UpdateProfileWnd(hDlg);
		return TRUE;

	case WM_DESTROY:
		hDlgProfile = NULL;
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL: DestroyWindow(hDlg); return TRUE;
		}
		break;
	}
	return FALSE;
	UNREFERENCED_PARAMETER(lParam);
}

static BOOL OnProfile(HWND hDlg)
{
	if (hDlgProfile == NULL)				// no profile dialog, create it
	{
		if ((hDlgProfile = CreateDialog(hApp,MAKEINTRESOURCE(IDD_PROFILE),hDlg,
			(DLGPROC)Profile)) == NULL)
			AbortMessage(_T("Profile Dialog Box Creation Error !"));
	}
	else
	{
		SetFocus(hDlgProfile);				// set focus on profile dialog
	}
	return -1;								// call windows default handler
}


//################
//#
//#    RPL object viewer dialog box
//#
//################

//
// update rpl obj view dialog content
//
static VOID UpdateRplObjViewWnd(HWND hDlg, DWORD dwAddr)
{
	LPTSTR szObjList;

	if (hDlg == NULL) return;				// dialog not open

	// show entry point name only in mapped mode
	bRplViewName = (GetMemMapType() == MEM_MMU);

	// create view string
	szObjList = RplCreateObjView(dwAddr,GetMemDataSize(),TRUE);
	SetDlgItemText(hDlg,IDC_RPLVIEW_DATA,szObjList);
	free(szObjList);
	return;
}

//
// enter RPL object viewer dialog
//
static INT_PTR CALLBACK RplObjView(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		switch (cCurrentRomType)
		{
		case 'S': // HP48SX
			dwRplPlatform = RPL_P3;			// Charlemagne platform
			break;
		case '6': // HP38G (64K RAM version)
		case 'A': // HP38G
		case 'G': // HP48GX
			dwRplPlatform = RPL_P4;			// Alcuin platform
			break;
		case 'E': // HP39G/40G
		case 'X': // HP49G
			dwRplPlatform = RPL_P5;			// V'ger platform
			break;
		default:
			_ASSERT(FALSE);
		}

		bRplViewAddr = TRUE;				// show address
		bRplViewBin  = TRUE;				// show binary data
		return TRUE;

	case WM_DESTROY:
		hDlgRplObjView = NULL;
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL: DestroyWindow(hDlg); return TRUE;
		}
		break;
	}
	return FALSE;
	UNREFERENCED_PARAMETER(lParam);
}

static BOOL OnRplObjView(HWND hDlg)
{
	// get cursor address of memory view
	DWORD dwAddr = GetMemCurAddr(hDlg);

	if (hDlgRplObjView == NULL)				// no rpl obj view dialog, create it
	{
		if ((hDlgRplObjView = CreateDialog(hApp,MAKEINTRESOURCE(IDD_RPLVIEW),hDlg,
			(DLGPROC)RplObjView)) == NULL)
			AbortMessage(_T("RPL Object View Dialog Box Creation Error !"));
	}

	UpdateRplObjViewWnd(hDlgRplObjView,dwAddr);
	return -1;								// call windows default handler
}


//################
//#
//#    Settings dialog box
//#
//################

//
// copy edit box content to current combox box selection
//
static VOID CopyEditToCombo(HWND hDlg,HWND hWndComboBox)
{
	TCHAR szSymbFilename[MAX_PATH];
	INT   i;

	// get current selection
	if ((i = (INT) SendMessage(hWndComboBox,CB_GETCURSEL,0,0)) != CB_ERR)
	{
		// delete associated name
		free((LPVOID) SendMessage(hWndComboBox,CB_GETITEMDATA,i,0));

		// append actual name
		GetDlgItemText(hDlg,IDC_DEBUG_SET_FILE,szSymbFilename,ARRAYSIZEOF(szSymbFilename));
		SendMessage(hWndComboBox,CB_SETITEMDATA,i,(LPARAM) DuplicateString(szSymbFilename));
	}
	return;
}

//
// copy edit box content to current combox box selection
//
static VOID CopyComboToEdit(HWND hDlg,HWND hWndComboBox)
{
	HWND hWnd;
	INT  i;

	// get current selection
	if ((i = (INT) SendMessage(hWndComboBox,CB_GETCURSEL,0,0)) != CB_ERR)
	{
		// update file edit box
		hWnd = GetDlgItem(hDlg,IDC_DEBUG_SET_FILE);
		SetWindowText(hWnd,(LPTSTR) SendMessage(hWndComboBox,CB_GETITEMDATA,i,0));
		SendMessage(hWnd,EM_SETSEL,0,-1);
	}
	return;
}

//
// settings browse dialog
//
static BOOL OnBrowseSettings(HWND hDlg, HWND hWndFilename)
{
	TCHAR szBuffer[MAX_PATH];
	OPENFILENAME ofn;

	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hDlg;
	ofn.lpstrFilter =
		_T("HP-Tools Linker File (*.O)\0*.O\0")
		_T("All Files (*.*)\0*.*\0");
	ofn.lpstrDefExt = _T("O");
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = szBuffer;
	ofn.lpstrFile[0] = 0;
	ofn.nMaxFile = ARRAYSIZEOF(szBuffer);
	ofn.Flags = OFN_EXPLORER|OFN_HIDEREADONLY|OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
	if (GetOpenFileName(&ofn))
	{
		SetWindowText(hWndFilename,szBuffer);
		SendMessage(hWndFilename,EM_SETSEL,0,-1);
	}
	return 0;
}

//
// enter settings dialog
//
static INT_PTR CALLBACK Settings(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND  hWnd;
	TCHAR szSymbFilename[MAX_PATH];
	TCHAR szItemname[16];
	INT   i,nMax;

	switch (message)
	{
	case WM_INITDIALOG:
		// set disassembler mode
		CheckDlgButton(hDlg,(disassembler_mode == HP_MNEMONICS) ? IDC_DISASM_HP : IDC_DISASM_CLASS,BST_CHECKED);
		// set symbolic enable check button
		CheckDlgButton(hDlg,IDC_DEBUG_SET_SYMB,disassembler_symb);
		// fill model combo box and corresponding file edit box
		{
			LPCTSTR lpszModels;
			TCHAR   cModel[] = _T(" ");

			// fill model combo box
			hWnd = GetDlgItem(hDlg,IDC_DEBUG_SET_MODEL);
			for (lpszModels = _T(MODELS); *lpszModels != 0; ++lpszModels)
			{
				cModel[0] = *lpszModels;		// string with model character
				i = (INT) SendMessage(hWnd,CB_ADDSTRING,0,(LPARAM) cModel);

				// get filename
				wsprintf(szItemname,_T("Symb%c"),cModel[0]);
				ReadSettingsString(_T("Disassembler"),szItemname,_T(""),szSymbFilename,ARRAYSIZEOF(szSymbFilename));

				// append filename to model
				SendMessage(hWnd,CB_SETITEMDATA,i,(LPARAM) DuplicateString(szSymbFilename));
			}

			// select for actual model
			cModel[0] = (TCHAR) cCurrentRomType;
			if ((i = (INT) SendMessage(hWnd,CB_SELECTSTRING,0,(LPARAM) cModel)) != CB_ERR)
			{
				LPTSTR lpszFilename = (LPTSTR) SendMessage(hWnd,CB_GETITEMDATA,i,0);

				// fill file edit box
				hWnd = GetDlgItem(hDlg,IDC_DEBUG_SET_FILE);
				SetWindowText(hWnd,lpszFilename);
				SendMessage(hWnd,EM_SETSEL,0,-1);
			}
		}
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_DEBUG_SET_MODEL:
			// combo box changing item
			if (HIWORD(wParam) == CBN_SETFOCUS)
			{
				// update associated name with file edit box
				CopyEditToCombo(hDlg,(HWND) lParam);
			}
			// new combo box item selected
			if (HIWORD(wParam) == CBN_SELENDOK)
			{
				// update file edit box with associated name
				CopyComboToEdit(hDlg,(HWND) lParam);
			}
			break;
		case IDC_DEBUG_SET_BROWSE:
			return OnBrowseSettings(hDlg,GetDlgItem(hDlg,IDC_DEBUG_SET_FILE));
		case IDOK:
			// set disassembler mode
			disassembler_mode = IsDlgButtonChecked(hDlg,IDC_DISASM_HP) ? HP_MNEMONICS : CLASS_MNEMONICS;
			// set symbolic enable check button
			disassembler_symb = IsDlgButtonChecked(hDlg,IDC_DEBUG_SET_SYMB);
			// update associated name with file edit box
			hWnd = GetDlgItem(hDlg,IDC_DEBUG_SET_MODEL);
			CopyEditToCombo(hDlg,hWnd);
			// write all symbol filenames to registry
			nMax = (INT) SendMessage(hWnd,CB_GETCOUNT,0,0);
			for (i = 0; i < nMax; ++i)
			{
				LPTSTR lpszFilename;

				SendMessage(hWnd,CB_GETLBTEXT,i,(LPARAM) szSymbFilename);
				wsprintf(szItemname,_T("Symb%c"),szSymbFilename[0]);
				lpszFilename = (LPTSTR) SendMessage(hWnd,CB_GETITEMDATA,i,0);
				if (*lpszFilename == 0)		// empty
				{
					DelSettingsKey(_T("Disassembler"),szItemname);
				}
				else
				{
					WriteSettingsString(_T("Disassembler"),szItemname,lpszFilename);
				}
			}
			RplDeleteTable();				// delete rpl symbol table
			LoadSymbTable();				// reload external rpl symbol table
			// redraw debugger code view
			ViewCodeWnd(GetDlgItem(GetParent(hDlg),IDC_DEBUG_CODE),dwAdrLine[0]);
			// no break
		case IDCANCEL:
			// free combo box items
			hWnd = GetDlgItem(hDlg,IDC_DEBUG_SET_MODEL);
			nMax = (INT) SendMessage(hWnd,CB_GETCOUNT,0,0);
			for (i = 0; i < nMax; ++i)
			{
				LPTSTR lpszFilename = (LPTSTR) SendMessage(hWnd,CB_GETITEMDATA,i,0);
				if (lpszFilename != NULL)
				{
					free(lpszFilename);
				}
			}
			EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
	}
	return FALSE;
	UNREFERENCED_PARAMETER(lParam);
}

static BOOL OnSettings(HWND hDlg)
{
	if (DialogBox(hApp, MAKEINTRESOURCE(IDD_DEBUG_SETTINGS), hDlg, (DLGPROC)Settings) == -1)
		AbortMessage(_T("Settings Dialog Box Creation Error !"));
	return 0;
}


//################
//#
//#    New Value dialog box
//#
//################

//
// enter new value dialog
//
static INT_PTR CALLBACK NewValue(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static LPTSTR lpszBuffer;				// handle of buffer
	static int    nBufferlen;				// length of buffer

	HWND  hWnd;
	TCHAR szBuffer[64];
	LONG  i;

	switch (message)
	{
	case WM_INITDIALOG:
		lpszBuffer = (LPTSTR) lParam;
		// length with zero string terminator
		nBufferlen = lstrlen(lpszBuffer)+1;
		_ASSERT(ARRAYSIZEOF(szBuffer) >= nBufferlen);
		SetDlgItemText(hDlg,IDC_NEWVALUE,lpszBuffer);
		return TRUE;
	case WM_COMMAND:
		wParam = LOWORD(wParam);
		switch(wParam)
		{
		case IDOK:
			hWnd = GetDlgItem(hDlg,IDC_NEWVALUE);
			GetWindowText(hWnd,szBuffer,nBufferlen);
			// test if valid hex address
			for (i = 0; i < (LONG) lstrlen(szBuffer); ++i)
			{
				if (_istxdigit((_TUCHAR) szBuffer[i]) == 0)
				{
					SendMessage(hWnd,EM_SETSEL,0,-1);
					SetFocus(hWnd);			// focus to edit control
					return FALSE;
				}
			}
			lstrcpy(lpszBuffer,szBuffer);	// copy valid value
			// no break
		case IDCANCEL:
			EndDialog(hDlg,wParam);
			return TRUE;
		}
	}
	return FALSE;
}

static INT_PTR OnNewValue(LPTSTR lpszValue)
{
	INT_PTR nResult;

	if ((nResult = DialogBoxParam(hApp,
								  MAKEINTRESOURCE(IDD_NEWVALUE),
								  hDlgDebug,
								  (DLGPROC)NewValue,
								  (LPARAM)lpszValue)
								 ) == -1)
		AbortMessage(_T("Input Dialog Box Creation Error !"));
	return nResult;
}


//################
//#
//#    Goto Address dialog box
//#
//################

//
// enter goto address dialog
//
static INT_PTR CALLBACK EnterAddr(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hDlg,GWLP_USERDATA,(LONG_PTR) lParam);
		return TRUE;
	case WM_COMMAND:
		wParam = LOWORD(wParam);
		switch(wParam)
		{
		case IDOK:
			if (!GetAddr(hDlg,
						 IDC_ENTERADR,
						 (DWORD *) GetWindowLongPtr(hDlg,GWLP_USERDATA),
						 0xFFFFFFFF,
						 disassembler_symb))
				return FALSE;
			// no break
		case IDCANCEL:
			EndDialog(hDlg,wParam);
			return TRUE;
		}
	}
	return FALSE;
}

static VOID OnEnterAddress(HWND hDlg, DWORD *dwValue)
{
	if (DialogBoxParam(hApp, MAKEINTRESOURCE(IDD_ENTERADR), hDlg, (DLGPROC)EnterAddr, (LPARAM)dwValue) == -1)
		AbortMessage(_T("Address Dialog Box Creation Error !"));
}


//################
//#
//#    Breakpoint dialog box
//#
//################

//
// enter breakpoint dialog
//
static INT_PTR CALLBACK EnterBreakpoint(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static BP_T *sBp;

	switch (message)
	{
	case WM_INITDIALOG:
		sBp = (BP_T *) lParam;
		sBp->bEnable = TRUE;
		sBp->nType   = BP_EXEC;
		SendDlgItemMessage(hDlg,IDC_BPCODE,BM_SETCHECK,1,0);
		return TRUE;
	case WM_COMMAND:
		wParam = LOWORD(wParam);
		switch(wParam)
		{
		case IDC_BPCODE:   sBp->nType = BP_EXEC;   return TRUE;
		case IDC_BPRPL:    sBp->nType = BP_RPL;    return TRUE;
		case IDC_BPACCESS: sBp->nType = BP_ACCESS; return TRUE;
		case IDC_BPREAD:   sBp->nType = BP_READ;   return TRUE;
		case IDC_BPWRITE:  sBp->nType = BP_WRITE;  return TRUE;
		case IDOK:
			if (!GetAddr(hDlg,IDC_ENTERADR,&sBp->dwAddr,0xFFFFF,disassembler_symb))
				return FALSE;
			// no break
		case IDCANCEL:
			EndDialog(hDlg,wParam);
			return TRUE;
		}
	}
	return FALSE;
}

static VOID OnEnterBreakpoint(HWND hDlg, BP_T *sValue)
{
	if (DialogBoxParam(hApp, MAKEINTRESOURCE(IDD_ENTERBREAK), hDlg, (DLGPROC)EnterBreakpoint, (LPARAM)sValue) == -1)
		AbortMessage(_T("Breakpoint Dialog Box Creation Error !"));
}


//################
//#
//#    Edit breakpoint dialog box
//#
//################

//
// handle drawing in breakpoint window
//
static __inline BOOL OnDrawBreakWnd(LPDRAWITEMSTRUCT lpdis)
{
	TCHAR    szBuf[64];
	COLORREF crBkColor,crTextColor;
	HDC      hdcMem;
	HBITMAP  hBmpOld;

	if (lpdis->itemID == -1)				// no item in list box
		return TRUE;

	crBkColor   = GetBkColor(lpdis->hDC);	// save actual color settings
	crTextColor = GetTextColor(lpdis->hDC);

	if (lpdis->itemState & ODS_SELECTED)	// cursor line
	{
		SetBkColor(lpdis->hDC,GetSysColor(COLOR_HIGHLIGHT));
		SetTextColor(lpdis->hDC,GetSysColor(COLOR_HIGHLIGHTTEXT));
	}

	// write Text
	SendMessage(lpdis->hwndItem,LB_GETTEXT,lpdis->itemID,(LPARAM) szBuf);
	ExtTextOut(lpdis->hDC,(int)(lpdis->rcItem.left)+17,(int)(lpdis->rcItem.top),
			   ETO_OPAQUE,(LPRECT)&lpdis->rcItem,szBuf,lstrlen(szBuf),NULL);

	SetBkColor(lpdis->hDC,crBkColor);		// restore color settings
	SetTextColor(lpdis->hDC,crTextColor);

	// draw checkbox
	hdcMem = CreateCompatibleDC(lpdis->hDC);
	_ASSERT(hBmpCheckBox);
	hBmpOld = (HBITMAP) SelectObject(hdcMem,hBmpCheckBox);

	BitBlt(lpdis->hDC,lpdis->rcItem.left+2,lpdis->rcItem.top+2,
		   11,lpdis->rcItem.bottom - lpdis->rcItem.top,
		   hdcMem,sBreakpoint[lpdis->itemData].bEnable ? 0 : 10,0,SRCCOPY);

	SelectObject(hdcMem,hBmpOld);
	DeleteDC(hdcMem);

	if (lpdis->itemState & ODS_FOCUS)		// redraw focus
		DrawFocusRect(lpdis->hDC,&lpdis->rcItem);

	return TRUE;							// focus handled here
}

//
// toggle breakpoint drawing
//
static BOOL ToggleBreakpointItem(HWND hWnd, INT nItem)
{
	RECT rc;

	// get breakpoint number
	INT i = (INT) SendMessage(hWnd,LB_GETITEMDATA,nItem,0);

	sBreakpoint[i].bEnable = !sBreakpoint[i].bEnable;
	// update region of toggled item
	SendMessage(hWnd,LB_GETITEMRECT,nItem,(LPARAM)&rc);
	InvalidateRect(hWnd,&rc,TRUE);
	return TRUE;
}

//
// draw breakpoint type
//
static VOID DrawBreakpoint(HWND hWnd, INT i)
{
	TCHAR *szText,szBuffer[32];
	INT   nItem;

	switch(sBreakpoint[i].nType)
	{
	case BP_EXEC:   // code breakpoint
		szText = _T("Code");
		break;
	case BP_RPL:   // RPL breakpoint
		szText = _T("RPL");
		break;
	case BP_READ:   // read memory breakpoint
		szText = _T("Memory Read");
		break;
	case BP_WRITE:  // write memory breakpoint
		szText = _T("Memory Write");
		break;
	case BP_ACCESS: // memory breakpoint
		szText = _T("Memory Access");
		break;
	default:        // unknown breakpoint type
		szText = _T("unknown");
		_ASSERT(0);
	}
	wsprintf(szBuffer,_T("%05X (%s)"),sBreakpoint[i].dwAddr,szText);
	nItem = (INT) SendMessage(hWnd,LB_ADDSTRING,0,(LPARAM) szBuffer);
	SendMessage(hWnd,LB_SETITEMDATA,nItem,i);
	return;
}

//
// enter edit breakpoint dialog
//
static INT_PTR CALLBACK EditBreakpoint(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	TEXTMETRIC tm;

	HWND  hWnd;
	HDC   hDC;
	HFONT hFont;
	BP_T  sBp;
	INT   i,nItem;

	switch (message)
	{
	case WM_INITDIALOG:
		// font settings
		SendDlgItemMessage(hDlg,IDC_STATIC_BREAKPOINT,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),MAKELPARAM(FALSE,0));
		SendDlgItemMessage(hDlg,IDC_BREAKEDIT_ADD,    WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),MAKELPARAM(FALSE,0));
		SendDlgItemMessage(hDlg,IDC_BREAKEDIT_DELETE, WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),MAKELPARAM(FALSE,0));
		SendDlgItemMessage(hDlg,IDCANCEL,             WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),MAKELPARAM(FALSE,0));

		hBmpCheckBox = LoadBitmap(hApp,MAKEINTRESOURCE(IDB_CHECKBOX));
		_ASSERT(hBmpCheckBox);

		hWnd = GetDlgItem(hDlg,IDC_BREAKEDIT_WND);
		SendMessage(hWnd,WM_SETREDRAW,FALSE,0);
		SendMessage(hWnd,LB_RESETCONTENT,0,0);
		for (i = 0; i < wBreakpointCount; ++i)
			DrawBreakpoint(hWnd,i);
		SendMessage(hWnd,WM_SETREDRAW,TRUE,0);
		return TRUE;

	case WM_DESTROY:
		DeleteObject(hBmpCheckBox);
		return TRUE;

	case WM_COMMAND:
		hWnd = GetDlgItem(hDlg,IDC_BREAKEDIT_WND);
		switch (HIWORD(wParam))
		{
		case LBN_DBLCLK:
			if (LOWORD(wParam) == IDC_BREAKEDIT_WND)
			{
				if ((nItem = (INT) SendMessage(hWnd,LB_GETCURSEL,0,0)) == LB_ERR)
					return FALSE;

				return ToggleBreakpointItem(hWnd,nItem);
			}
		}

		switch(LOWORD(wParam))
		{
		case IDC_BREAKEDIT_ADD:
			sBp.dwAddr = -1;				// no breakpoint given
			OnEnterBreakpoint(hDlg, &sBp);
			if (sBp.dwAddr != -1)
			{
				for (i = 0; i < wBreakpointCount; ++i)
				{
					if (sBreakpoint[i].dwAddr == sBp.dwAddr)
					{
						// tried to add used code breakpoint
						if (sBreakpoint[i].bEnable && (sBreakpoint[i].nType & sBp.nType & (BP_EXEC | BP_RPL)) != 0)
							return FALSE;

						// only modify memory breakpoints
						if (   (   sBreakpoint[i].bEnable == FALSE
								&& (sBreakpoint[i].nType & sBp.nType & (BP_EXEC | BP_RPL)) != 0)
							|| ((sBreakpoint[i].nType & BP_ACCESS) && (sBp.nType & BP_ACCESS)))
						{
							// replace breakpoint type
							sBreakpoint[i].bEnable = TRUE;
							sBreakpoint[i].nType   = sBp.nType;

							// redaw breakpoint list
							SendMessage(hWnd,WM_SETREDRAW,FALSE,0);
							SendMessage(hWnd,LB_RESETCONTENT,0,0);
							for (i = 0; i < wBreakpointCount; ++i)
								DrawBreakpoint(hWnd,i);
							SendMessage(hWnd,WM_SETREDRAW,TRUE,0);
							return FALSE;
						}
					}
				}

				// check for breakpoint buffer full
				if (wBreakpointCount >= MAXBREAKPOINTS)
				{
					AbortMessage(_T("Reached maximum number of breakpoints !"));
					return FALSE;
				}

				sBreakpoint[wBreakpointCount].bEnable = sBp.bEnable;
				sBreakpoint[wBreakpointCount].nType   = sBp.nType;
				sBreakpoint[wBreakpointCount].dwAddr  = sBp.dwAddr;

				DrawBreakpoint(hWnd,wBreakpointCount);

				++wBreakpointCount;
			}
			return TRUE;

		case IDC_BREAKEDIT_DELETE:
			// scan all breakpoints from top
			for (nItem = wBreakpointCount-1; nItem >= 0; --nItem)
			{
				// item selected
				if (SendMessage(hWnd,LB_GETSEL,nItem,0) > 0)
				{
					INT j;

					// get breakpoint index
					i = (INT) SendMessage(hWnd,LB_GETITEMDATA,nItem,0);
					SendMessage(hWnd,LB_DELETESTRING,nItem,0);
					--wBreakpointCount;

					// update remaining list box references
					for (j = 0; j < wBreakpointCount; ++j)
					{
						INT k = (INT) SendMessage(hWnd,LB_GETITEMDATA,j,0);
						if (k > i) SendMessage(hWnd,LB_SETITEMDATA,j,k-1);
					}

					// remove breakpoint from breakpoint table
					while (++i <= wBreakpointCount)
						sBreakpoint[i-1] = sBreakpoint[i];
				}
			}
			return TRUE;

		case IDCANCEL:
			EndDialog(hDlg,IDCANCEL);
			return TRUE;
		}

	case WM_VKEYTOITEM:
		if (LOWORD(wParam) == VK_SPACE)
		{
			hWnd = GetDlgItem(hDlg,IDC_BREAKEDIT_WND);
			for (nItem = 0; nItem < wBreakpointCount; ++nItem)
			{
				// item selected
				if (SendMessage(hWnd,LB_GETSEL,nItem,0) > 0)
					ToggleBreakpointItem(hWnd,nItem);
			}
			return -2;
		}
		return -1;							// default action

	case WM_DRAWITEM:
		if (wParam == IDC_BREAKEDIT_WND) return OnDrawBreakWnd((LPDRAWITEMSTRUCT) lParam);
		break;

	case WM_MEASUREITEM:
		hDC = GetDC(hDlg);

		// GetTextMetrics from "Courier New 8" font
		hFont = CreateFont(-MulDiv(8,GetDeviceCaps(hDC, LOGPIXELSY),72),0,0,0,FW_NORMAL,0,0,0,ANSI_CHARSET,
						   OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,FF_DONTCARE,_T("Courier New"));

		hFont = (HFONT) SelectObject(hDC,hFont);
		GetTextMetrics(hDC,&tm);
		hFont = (HFONT) SelectObject(hDC,hFont);
		DeleteObject(hFont);

		((LPMEASUREITEMSTRUCT) lParam)->itemHeight = tm.tmHeight;

		ReleaseDC(hDlg,hDC);
		return TRUE;
	}
	return FALSE;
	UNREFERENCED_PARAMETER(lParam);
}

static BOOL OnEditBreakpoint(HWND hDlg)
{
	if (DialogBox(hApp, MAKEINTRESOURCE(IDD_BREAKEDIT), hDlg, (DLGPROC)EditBreakpoint) == -1)
		AbortMessage(_T("Edit Breakpoint Dialog Box Creation Error !"));

	// update code window
	InvalidateRect(GetDlgItem(hDlg,IDC_DEBUG_CODE),NULL,TRUE);
	return -1;
}


//################
//#
//#    Last Instruction dialog box
//#
//################

//
// view last instructions
//
static INT_PTR CALLBACK InfoIntr(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND  hWnd;
	TCHAR szBuffer[64];
	LONG  lIndex;
	WORD  i,j;

	switch (message)
	{
	case WM_INITDIALOG:
		// font settings
		SendDlgItemMessage(hDlg,IDC_INSTR_TEXT,  WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),MAKELPARAM(FALSE,0));
		SendDlgItemMessage(hDlg,IDC_INSTR_CLEAR, WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),MAKELPARAM(FALSE,0));
		SendDlgItemMessage(hDlg,IDC_INSTR_COPY,  WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),MAKELPARAM(FALSE,0));
		SendDlgItemMessage(hDlg,IDCANCEL,        WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),MAKELPARAM(FALSE,0));

		lIndex = 0;							// init lIndex
		hWnd = GetDlgItem(hDlg,IDC_INSTR_CODE);
		SendMessage(hWnd,WM_SETREDRAW,FALSE,0);
		SendMessage(hWnd,LB_RESETCONTENT,0,0);
		for (i = wInstrRp; i != wInstrWp; i = (i + 1) % wInstrSize)
		{
			LPCTSTR lpszName;

			// entry has a name
			if (disassembler_symb && (lpszName = RplGetName(pdwInstrArray[i])) != NULL)
			{
				szBuffer[0] = _T('=');
				lstrcpyn(&szBuffer[1],lpszName,ARRAYSIZEOF(szBuffer)-1);
				SendMessage(hWnd,LB_ADDSTRING,0,(LPARAM) szBuffer);
			}

			j = wsprintf(szBuffer,_T("%05lX   "),pdwInstrArray[i]);
			disassemble(pdwInstrArray[i],&szBuffer[j]);
			lIndex = (LONG) SendMessage(hWnd,LB_ADDSTRING,0,(LPARAM) szBuffer);
		}
		SendMessage(hWnd,WM_SETREDRAW,TRUE,0);
		SendMessage(hWnd,LB_SETCARETINDEX,lIndex,TRUE);
		return TRUE;
	case WM_COMMAND:
		hWnd = GetDlgItem(hDlg,IDC_INSTR_CODE);
		switch(LOWORD(wParam))
		{
		case IDC_INSTR_COPY:
			CopyItemsToClipboard(hWnd);		// copy selected items to clipboard
			return TRUE;
		case IDC_INSTR_CLEAR:				// clear instruction buffer
			wInstrRp = wInstrWp;
			SendMessage(hWnd,LB_RESETCONTENT,0,0);
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg,IDCANCEL);
			return TRUE;
		}
	}
	return FALSE;
	UNREFERENCED_PARAMETER(lParam);
}

static BOOL OnInfoIntr(HWND hDlg)
{
	if (DialogBox(hApp, MAKEINTRESOURCE(IDD_INSTRUCTIONS), hDlg, (DLGPROC)InfoIntr) == -1)
		AbortMessage(_T("Last Instructions Dialog Box Creation Error !"));
	return 0;
}

//
// view write only I/O registers
//
static INT_PTR CALLBACK InfoWoRegister(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	TCHAR szBuffer[8];

	switch (message)
	{
	case WM_INITDIALOG:
		wsprintf(szBuffer,_T("%05X"),Chipset.start1);
		SetDlgItemText(hDlg,IDC_ADDR20_24,szBuffer);
		wsprintf(szBuffer,_T("%03X"),Chipset.loffset);
		SetDlgItemText(hDlg,IDC_ADDR25_27,szBuffer);
		wsprintf(szBuffer,_T("%02X"),Chipset.lcounter);
		SetDlgItemText(hDlg,IDC_ADDR28_29,szBuffer);
		wsprintf(szBuffer,_T("%05X"),Chipset.start2);
		SetDlgItemText(hDlg,IDC_ADDR30_34,szBuffer);
		return TRUE;
	case WM_COMMAND:
		if ((LOWORD(wParam) == IDCANCEL))
		{
			EndDialog(hDlg,IDCANCEL);
			return TRUE;
		}
	}
	return FALSE;
	UNREFERENCED_PARAMETER(lParam);
}

static BOOL OnInfoWoRegister(HWND hDlg)
{
	if (DialogBox(hApp, MAKEINTRESOURCE(IDD_WRITEONLYREG), hDlg, (DLGPROC)InfoWoRegister) == -1)
		AbortMessage(_T("Write-Only Register Dialog Box Creation Error !"));
	return 0;
}


//################
//#
//#    Breakpoint list operations
//#
//################

//
// load breakpoint list
//
VOID LoadBreakpointList(HANDLE hFile)		// NULL = clear breakpoint list
{
	DWORD lBytesRead = 0;

	// read number of breakpoints
	if (hFile) ReadFile(hFile, &wBreakpointCount, sizeof(wBreakpointCount), &lBytesRead, NULL);

	// breakpoints found
	if (lBytesRead == sizeof(wBreakpointCount) && wBreakpointCount < ARRAYSIZEOF(sBreakpoint))
	{
		WORD wBreakpointSize;

		// read size of one breakpoint
		ReadFile(hFile, &wBreakpointSize, sizeof(wBreakpointSize), &lBytesRead, NULL);
		if (lBytesRead == sizeof(wBreakpointSize) && wBreakpointSize == sizeof(sBreakpoint[0]))
		{
			// read breakpoints
			ReadFile(hFile, sBreakpoint, wBreakpointCount * sizeof(sBreakpoint[0]), &lBytesRead, NULL);
			_ASSERT(lBytesRead == wBreakpointCount * sizeof(sBreakpoint[0]));
		}
		else								// changed breakpoint structure
		{
			wBreakpointCount = 0;			// clear breakpoint list
		}
	}
	else									// no breakpoints or breakpoint buffer too small
	{
		wBreakpointCount = 0;				// clear breakpoint list
	}
	return;
}

//
// save breakpoint list
//
VOID SaveBreakpointList(HANDLE hFile)
{
	if (wBreakpointCount)					// defined breakpoints
	{
		DWORD lBytesWritten;

		WORD wBreakpointSize = sizeof(sBreakpoint[0]);

		_ASSERT(hFile);						// valid file pointer?

		// write number of breakpoints
		WriteFile(hFile, &wBreakpointCount, sizeof(wBreakpointCount), &lBytesWritten, NULL);
		_ASSERT(lBytesWritten == sizeof(wBreakpointCount));

		// write size of one breakpoint
		WriteFile(hFile, &wBreakpointSize, sizeof(wBreakpointSize), &lBytesWritten, NULL);
		_ASSERT(lBytesWritten == sizeof(wBreakpointSize));

		// write breakpoints
		WriteFile(hFile, sBreakpoint, wBreakpointCount * sizeof(sBreakpoint[0]), &lBytesWritten, NULL);
		_ASSERT(lBytesWritten == wBreakpointCount * sizeof(sBreakpoint[0]));
	}
	return;
}

//
// create a copy of the breakpoint list
//
VOID CreateBackupBreakpointList(VOID)
{
	_ASSERT(sizeof(sBackupBreakpoint) == sizeof(sBreakpoint));

	wBackupBreakpointCount = wBreakpointCount;

	if (wBreakpointCount > 0)				// list not empty
	{
		CopyMemory(sBackupBreakpoint,sBreakpoint,sizeof(sBackupBreakpoint));
	}
	return;
}

//
// restore the breakpoint list from the copy
//
VOID RestoreBackupBreakpointList(VOID)
{
	_ASSERT(sizeof(sBackupBreakpoint) == sizeof(sBreakpoint));

	wBreakpointCount = wBackupBreakpointCount;

	if (wBreakpointCount > 0)				// list not empty
	{
		CopyMemory(sBreakpoint,sBackupBreakpoint,sizeof(sBreakpoint));
	}
	return;
}


//################
//#
//#    Load/Save Memory Data
//#
//################

static BOOL OnBrowseLoadMem(HWND hDlg)
{
	TCHAR szBuffer[MAX_PATH];
	OPENFILENAME ofn;

	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hDlg;
	ofn.lpstrFilter =
		_T("Memory Dump Files (*.MEM)\0*.MEM\0")
		_T("All Files (*.*)\0*.*\0");
	ofn.lpstrDefExt = _T("MEM");
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = szBuffer;
	ofn.lpstrFile[0] = 0;
	ofn.nMaxFile = ARRAYSIZEOF(szBuffer);
	ofn.Flags = OFN_EXPLORER|OFN_HIDEREADONLY|OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
	if (GetOpenFileName(&ofn))
	{
		SetDlgItemText(hDlg,IDC_DEBUG_DATA_FILE,szBuffer);
	}
	return 0;
}

static BOOL OnBrowseSaveMem(HWND hDlg)
{
	TCHAR szBuffer[MAX_PATH];
	OPENFILENAME ofn;

	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hDlg;
	ofn.lpstrFilter =
		_T("Memory Dump Files (*.MEM)\0*.MEM\0")
		_T("All Files (*.*)\0*.*\0");
	ofn.lpstrDefExt = _T("MEM");
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = szBuffer;
	ofn.lpstrFile[0] = 0;
	ofn.nMaxFile = ARRAYSIZEOF(szBuffer);
	ofn.Flags = OFN_EXPLORER|OFN_HIDEREADONLY|OFN_CREATEPROMPT|OFN_OVERWRITEPROMPT;
	if (GetSaveFileName(&ofn))
	{
		SetDlgItemText(hDlg,IDC_DEBUG_DATA_FILE,szBuffer);
	}
	return 0;
}

//
// write file to memory
//
static BOOL LoadMemData(LPCTSTR lpszFilename,DWORD dwStartAddr,UINT uBitMode)
{
	HANDLE hFile;
	DWORD  dwFileSize,dwRead;
	LPBYTE pbyData;

	hFile = CreateFile(lpszFilename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
	if (hFile == INVALID_HANDLE_VALUE)		// error, couldn't create a new file
		return FALSE;

	dwFileSize = GetFileSize(hFile, NULL);

	if ((pbyData = (LPBYTE) malloc(dwFileSize)) != NULL)
	{
		ReadFile(hFile,pbyData,dwFileSize,&dwRead,NULL);

		if (uBitMode == 2)					// auto mode (0=8-bit, 1=4-bit, 2=auto)
		{
			BOOL bPacked = FALSE;			// data not packed
			DWORD dwIndex;

			for (dwIndex = 0; !bPacked && dwIndex < dwFileSize; ++dwIndex)
			{
				bPacked = ((pbyData[dwIndex] & 0xF0) != 0);
			}
			uBitMode = bPacked ? 0 : 1;		// 0=8-bit, 1=4-bit
		}

		if (uBitMode == 0)					// 0=8-bit
		{
			LPBYTE pbyDataNew = (LPBYTE) realloc(pbyData,2*dwFileSize);
			if (pbyDataNew)
			{
				LPBYTE pbySrc,pbyDest;
				pbyData = pbyDataNew;

				// source start address
				pbySrc = pbyData + dwFileSize;

				dwFileSize *= 2;			// new filesize

				// destination start address
				pbyDest = pbyData + dwFileSize;

				while (pbySrc != pbyDest)	// unpack source
				{
					CONST BYTE byValue = *(--pbySrc);
					*(--pbyDest) = byValue >> 4;
					*(--pbyDest) = byValue & 0xF;
				}
				_ASSERT(pbySrc == pbyData);
				_ASSERT(pbyDest == pbyData);
			}
			else
			{
				free(pbyData);
				pbyData = NULL;
			}
		}

		if (pbyData)						// have data to save
		{
			LPBYTE p = pbyData;

			// read data size or end of Saturn address space
			while (dwFileSize > 0 && dwStartAddr <= 0xFFFFF)
			{
				// write nibble in map mode
				Nwrite(p++,dwStartAddr++,1);
				--dwFileSize;
			}
		}
		free(pbyData);
	}
	CloseHandle(hFile);
	return pbyData != NULL;
}

//
// write memory data to file
//
static BOOL SaveMemData(LPCTSTR lpszFilename,DWORD dwStartAddr,DWORD dwEndAddr,UINT uBitMode)
{
	HANDLE hFile;
	DWORD  dwAddr,dwSend,dwWritten;
	BYTE   byData[2];

	hFile = CreateFile(lpszFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)		// error, couldn't create a new file
		return FALSE;

	for (dwAddr = dwStartAddr; dwAddr <= dwEndAddr; dwAddr += 2)
	{
		_ASSERT(dwAddr <= 0xFFFFF);
		Npeek(byData,dwAddr,2);				// read two nibble in map mode

		dwSend = 2;							// send 2 nibble
		if (uBitMode == 0)					// (0=8-bit, 1=4-bit)
		{
			byData[0] = byData[0]|(byData[1]<<4);
			dwSend = 1;						// send 1 byte
		}

		WriteFile(hFile,&byData,dwSend,&dwWritten,NULL);
	}

	CloseHandle(hFile);
	return TRUE;
}

//
// memory load data
//
static INT_PTR CALLBACK DebugMemLoad(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	TCHAR szFilename[MAX_PATH];
	DWORD dwStartAddr;
	int   nButton;
	UINT  uBitMode;

	switch (message)
	{
	case WM_INITDIALOG:
		CheckDlgButton(hDlg,IDC_DEBUG_DATA_LOAD_ABIT,BST_CHECKED);
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_DEBUG_DATA_BUT:
			return OnBrowseLoadMem(hDlg);

		case IDOK:
			dwStartAddr = -1;				// no address given

			// get filename
			GetDlgItemText(hDlg,IDC_DEBUG_DATA_FILE,szFilename,ARRAYSIZEOF(szFilename));

			// decode address field
			if (   !GetAddr(hDlg,IDC_DEBUG_DATA_STARTADDR,&dwStartAddr,0xFFFFF,FALSE)
				|| dwStartAddr == -1)
				return FALSE;

			_ASSERT(dwStartAddr <= 0xFFFFF);

			// load as 8-bit or 4-bit data (0=8-bit, 1=4-bit, 2=auto)
			for (nButton = IDC_DEBUG_DATA_LOAD_8BIT; nButton <= IDC_DEBUG_DATA_LOAD_ABIT; ++nButton)
			{
				if (IsDlgButtonChecked(hDlg,nButton) == BST_CHECKED)
					break;
			}
			uBitMode = (UINT) (nButton - IDC_DEBUG_DATA_LOAD_8BIT);

			// load memory dump file
			if (!LoadMemData(szFilename,dwStartAddr,uBitMode))
				return FALSE;

			// update memory window
			UpdateMemoryWnd(GetParent(hDlg));

			// no break
		case IDCANCEL:
			EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
	}
	return FALSE;
	UNREFERENCED_PARAMETER(lParam);
}

static BOOL OnMemLoadData(HWND hDlg)
{
	if (DialogBox(hApp, MAKEINTRESOURCE(IDD_DEBUG_MEMLOAD), hDlg, (DLGPROC)DebugMemLoad) == -1)
		AbortMessage(_T("DebugLoad Dialog Box Creation Error !"));
	return -1;
}

//
// memory save data
//
static INT_PTR CALLBACK DebugMemSave(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	TCHAR szFilename[MAX_PATH];
	DWORD dwStartAddr,dwEndAddr;
	UINT  uBitMode;

	switch (message)
	{
	case WM_INITDIALOG:
		CheckDlgButton(hDlg,IDC_DEBUG_DATA_SAVE_8BIT,BST_CHECKED);
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_DEBUG_DATA_BUT:
			return OnBrowseSaveMem(hDlg);

		case IDOK:
			dwStartAddr = dwEndAddr = -1;	// no address given

			// get filename
			GetDlgItemText(hDlg,IDC_DEBUG_DATA_FILE,szFilename,ARRAYSIZEOF(szFilename));

			// decode address fields
			if (   !GetAddr(hDlg,IDC_DEBUG_DATA_STARTADDR,&dwStartAddr,0xFFFFF,FALSE)
				|| dwStartAddr == -1)
				return FALSE;
			if (   !GetAddr(hDlg,IDC_DEBUG_DATA_ENDADDR,&dwEndAddr,0xFFFFF,FALSE)
				|| dwEndAddr == -1)
				return FALSE;

			_ASSERT(dwStartAddr <= 0xFFFFF);
			_ASSERT(dwEndAddr   <= 0xFFFFF);

			// save as 8-bit or 4-bit data (0=8-bit, 1=4-bit)
			uBitMode = IsDlgButtonChecked(hDlg,IDC_DEBUG_DATA_SAVE_4BIT);

			// save memory dump file
			if (!SaveMemData(szFilename,dwStartAddr,dwEndAddr,uBitMode))
				return FALSE;

			// no break
		case IDCANCEL:
			EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
	}
	return FALSE;
	UNREFERENCED_PARAMETER(lParam);
}

static BOOL OnMemSaveData(HWND hDlg)
{
	if (DialogBox(hApp, MAKEINTRESOURCE(IDD_DEBUG_MEMSAVE), hDlg, (DLGPROC)DebugMemSave) == -1)
		AbortMessage(_T("DebugSave Dialog Box Creation Error !"));
	return -1;
}


//################
//#
//#    Trace Log
//#
//################

static VOID StartTrace(VOID)
{
	if (hLogFile == NULL)
	{
		SetCurrentDirectory(szEmuDirectory);
		hLogFile = CreateFile(
			szTraceFilename,
			GENERIC_READ|GENERIC_WRITE,
			FILE_SHARE_READ,
			NULL,
			(uTraceMode == TRACE_FILE_NEW) ? CREATE_ALWAYS : OPEN_ALWAYS,
			0,
			NULL);
		SetCurrentDirectory(szCurrentDirectory);
		if (hLogFile == INVALID_HANDLE_VALUE)
		{
			InfoMessage(_T("Unable to create trace log file."));
			hLogFile = NULL;
			return;
		}

		// goto end of file
		SetFilePointer(hLogFile,0L,NULL,FILE_END);
	}
	return;
}

static VOID StopTrace(VOID)
{
	if (hLogFile != NULL)
	{
		CloseHandle(hLogFile);
	}
	hLogFile = NULL;
	return;
}

static VOID FlushTrace(VOID)
{
	if (hLogFile != NULL)
	{
		VERIFY(FlushFileBuffers(hLogFile));
	}
	return;
}

static __inline void __cdecl PrintTrace(LPCTSTR lpFormat, ...)
{
	TCHAR cOutput[1024];
	DWORD dwWritten, dwRead;
	va_list arglist;

	va_start(arglist,lpFormat);
	dwWritten = (DWORD) wvsprintf(cOutput,lpFormat,arglist);
	va_end(arglist);
	#if defined _UNICODE
	{
		// Unicode to byte translation
		LPTSTR szTmp = DuplicateString(cOutput);
		if (szTmp != NULL)
		{
			WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK,
								szTmp, -1,
								(LPSTR) cOutput, sizeof(cOutput), NULL, NULL);
			free(szTmp);
		}
	}
	#endif
	WriteFile(hLogFile,cOutput,dwWritten,&dwRead,NULL);
	return;
}

static VOID OutTrace(VOID)
{
	enum MEM_MAPPING eMapMode;
	LPCTSTR lpszName;
	LPTSTR  s,d;
	TCHAR   szBuffer[128];
	TCHAR   szOpc[8];
	DWORD   dwNxtAddr,dwOpcAddr;
	UINT    i;

	if (hLogFile != NULL)					// log file opened
	{
		if (bTraceReg)						// show regs
		{
			INT  nPos;

			nPos =  wsprintf(szBuffer,_T("\r\n  A=%s"),RegToStr(Chipset.A,16));
			nPos += wsprintf(&szBuffer[nPos],_T("  B=%s"),RegToStr(Chipset.B,16));
			nPos += wsprintf(&szBuffer[nPos],_T("  C=%s"),RegToStr(Chipset.C,16));
			wsprintf(&szBuffer[nPos],_T("  D=%s\r\n"),RegToStr(Chipset.D,16));
			PrintTrace(szBuffer);

			nPos =  wsprintf(szBuffer,_T("  R0=%s"),RegToStr(Chipset.R0,16));
			nPos += wsprintf(&szBuffer[nPos],_T(" R1=%s"),RegToStr(Chipset.R1,16));
			nPos += wsprintf(&szBuffer[nPos],_T(" R2=%s"),RegToStr(Chipset.R2,16));
			wsprintf(&szBuffer[nPos],_T(" R3=%s\r\n"),RegToStr(Chipset.R3,16));
			PrintTrace(szBuffer);

			PrintTrace(_T("  R4=%s D0=%05X  D1=%05X  P=%X  CY=%d  Mode=%c  OUT=%03X  IN=%04X\r\n"),
				RegToStr(Chipset.R4,16),Chipset.d0,Chipset.d1,Chipset.P,Chipset.carry,
				Chipset.mode_dec ? _T('D') : _T('H'),Chipset.out,Chipset.in);

			PrintTrace(_T("  ST=%s  MP=%d SR=%d SB=%d XM=%d  IntrEn=%d  KeyScan=%d  BS=%02X\r\n"),
				RegToStr(Chipset.ST,4),
				(Chipset.HST & MP) != 0,(Chipset.HST & SR) != 0,(Chipset.HST & SB) != 0,(Chipset.HST & XM) != 0,
				Chipset.inte,Chipset.intk,Chipset.Bank_FF & 0x7F);

			// hardware stack content
			PrintTrace(_T("  Stack="));
			for (i = 1; i <= ARRAYSIZEOF(Chipset.rstk); ++i)
			{
				PrintTrace(_T(" %05X"), Chipset.rstk[(Chipset.rstkp-i)&7]);
			}
			PrintTrace(_T("\r\n"));
		}

		if (bTraceMmu)						// show MMU
		{
			TCHAR szSize[8],szAddr[8];

			if (!bTraceReg)					// no regs
			{
				PrintTrace(_T("\r\n"));		// add separator line
			}

			wsprintf(szAddr, Chipset.IOCfig ? _T("%05X") : _T("-----"),Chipset.IOBase);
			PrintTrace(_T("  I/O=%s"),szAddr);

			wsprintf(szSize, Chipset.P0Cfg2 ? _T("%05X") : _T("-----"),(Chipset.P0Size^0xFF)<<12);
			wsprintf(szAddr, Chipset.P0Cfig ? _T("%05X") : _T("-----"),Chipset.P0Base<<12);
			PrintTrace(_T(" NCE2=%s/%s"),szSize,szAddr);

			if (cCurrentRomType=='S')
			{
				wsprintf(szSize, Chipset.P1Cfg2 ? _T("%05X") : _T("-----"),(Chipset.P1Size^0xFF)<<12);
				wsprintf(szAddr, Chipset.P1Cfig ? _T("%05X") : _T("-----"),Chipset.P1Base<<12);
			}
			else
			{
				wsprintf(szSize, Chipset.BSCfg2 ? _T("%05X") : _T("-----"),(Chipset.BSSize^0xFF)<<12);
				wsprintf(szAddr, Chipset.BSCfig ? _T("%05X") : _T("-----"),Chipset.BSBase<<12);
			}
			PrintTrace(_T(" CE1=%s/%s"),szSize,szAddr);

			if (cCurrentRomType=='S')
			{
				wsprintf(szSize, Chipset.P2Cfg2 ? _T("%05X") : _T("-----"),(Chipset.P2Size^0xFF)<<12);
				wsprintf(szAddr, Chipset.P2Cfig ? _T("%05X") : _T("-----"),Chipset.P2Base<<12);
			}
			else
			{
				wsprintf(szSize, Chipset.P1Cfg2 ? _T("%05X") : _T("-----"),(Chipset.P1Size^0xFF)<<12);
				wsprintf(szAddr, Chipset.P1Cfig ? _T("%05X") : _T("-----"),Chipset.P1Base<<12);
			}
			PrintTrace(_T(" CE2=%s/%s"),szSize,szAddr);

			if (cCurrentRomType=='S')
			{
				wsprintf(szSize, Chipset.BSCfg2 ? _T("%05X") : _T("-----"),(Chipset.BSSize^0xFF)<<12);
				wsprintf(szAddr, Chipset.BSCfig ? _T("%05X") : _T("-----"),Chipset.BSBase<<12);
			}
			else
			{
				wsprintf(szSize, Chipset.P2Cfg2 ? _T("%05X") : _T("-----"),(Chipset.P2Size^0xFF)<<12);
				wsprintf(szAddr, Chipset.P2Cfig ? _T("%05X") : _T("-----"),Chipset.P2Base<<12);
			}
			PrintTrace(_T(" NCE3=%s/%s\r\n"),szSize,szAddr);
		}

		// disassemble line
		eMapMode = GetMemMapType();			// get current map mode
		SetMemMapType(MEM_MMU);				// disassemble in mapped mode

		// entry has a name
		if (disassembler_symb && (lpszName = RplGetName(Chipset.pc)) != NULL)
		{
			PrintTrace(_T("=%s\r\n"),lpszName);	// print address as label
		}
		dwNxtAddr = disassemble(Chipset.pc,szBuffer);

		// in disassembly replace space characters
		// between Opcode and Modifier with one TAB
		if ((s = _tcschr(szBuffer,_T(' '))) != NULL)
		{
			// skip blanks
			for (d = s; *d == _T(' '); ++d) { }

			if (d == &szBuffer[8])			// on TAB position
			{
				*s++ = _T('\t');			// replace with TAB

				// move the opcode modifier
				while ((*s++ = *d++) != 0) { }
			}
		}

		if (bTraceOpc)						// show opcode nibbles
		{
			dwOpcAddr = Chipset.pc;			// init address

			// show opcode nibbles in a block of 5
			for (i = 0; i < 5 && dwOpcAddr < dwNxtAddr; ++i)
			{
				szOpc[i] = cHex[GetMemNib(&dwOpcAddr)];
			}

			if (i == 1)						// only 1 nibble written
			{
				szOpc[i++] = _T('\t');		// one additional TAB necessary
			}
			szOpc[i] = 0;					// EOS

			PrintTrace(_T("%05lX %s\t%s\r\n"),Chipset.pc,szOpc,szBuffer);

			while (dwOpcAddr < dwNxtAddr)	// decode rest of opcode
			{
				// show opcode nibbles in a block of 5
				for (i = 0; i < 5 && dwOpcAddr < dwNxtAddr; ++i)
				{
					szOpc[i] = cHex[GetMemNib(&dwOpcAddr)];
				}
				szOpc[i] = 0;				// EOS

				PrintTrace(_T("      %s\r\n"),szOpc);
			}
		}
		else								// without opcode nibbles 
		{
			PrintTrace(_T("%05lX\t%s\r\n"),Chipset.pc,szBuffer);
		}

		SetMemMapType(eMapMode);			// switch back to old map mode
	}
	return;
}

//
// trace settings dialog
//
static BOOL OnBrowseTraceSettings(HWND hDlg)
{
	TCHAR szBuffer[MAX_PATH];
	OPENFILENAME ofn;

	// get current content of file edit box
	GetDlgItemText(hDlg,IDC_TRACE_FILE,szBuffer,ARRAYSIZEOF(szBuffer));

	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hDlg;
	ofn.lpstrFilter =
		_T("Trace Log Files (*.log)\0*.log\0")
		_T("All Files (*.*)\0*.*\0");
	ofn.lpstrDefExt = _T("log");
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = szBuffer;
	ofn.nMaxFile = ARRAYSIZEOF(szBuffer);
	ofn.Flags = OFN_EXPLORER|OFN_HIDEREADONLY|OFN_CREATEPROMPT|OFN_OVERWRITEPROMPT;
	if (GetSaveFileName(&ofn))
	{
		SetDlgItemText(hDlg,IDC_TRACE_FILE,szBuffer);
	}
	return 0;
}

//
// trace settings
//
static INT_PTR CALLBACK TraceSettings(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hDlg,IDC_TRACE_FILE,szTraceFilename);
		CheckDlgButton(hDlg,(uTraceMode == TRACE_FILE_NEW) ? IDC_TRACE_NEW : IDC_TRACE_APPEND,BST_CHECKED);
		CheckDlgButton(hDlg,IDC_TRACE_REGISTER,bTraceReg);
		CheckDlgButton(hDlg,IDC_TRACE_MMU,bTraceMmu);
		CheckDlgButton(hDlg,IDC_TRACE_OPCODE,bTraceOpc);
		return TRUE;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_TRACE_BROWSE:
			return OnBrowseTraceSettings(hDlg);

		case IDOK:
			// get filename
			GetDlgItemText(hDlg,IDC_TRACE_FILE,szTraceFilename,ARRAYSIZEOF(szTraceFilename));

			// trace mode
			uTraceMode = IsDlgButtonChecked(hDlg,IDC_TRACE_NEW) ? TRACE_FILE_NEW : TRACE_FILE_APPEND;

			// trace content
			bTraceReg = IsDlgButtonChecked(hDlg,IDC_TRACE_REGISTER);
			bTraceMmu = IsDlgButtonChecked(hDlg,IDC_TRACE_MMU);
			bTraceOpc = IsDlgButtonChecked(hDlg,IDC_TRACE_OPCODE);

			// no break
		case IDCANCEL:
			EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
	}
	return FALSE;
	UNREFERENCED_PARAMETER(lParam);
}

static BOOL OnTraceSettings(HWND hDlg)
{
	if (DialogBox(hApp, MAKEINTRESOURCE(IDD_TRACE), hDlg, (DLGPROC)TraceSettings) == -1)
		AbortMessage(_T("TraceSettings Dialog Box Creation Error !"));
	return -1;
}

static BOOL OnTraceEnable(HWND hDlg)
{
	OnToggleMenuItem(hDlg,ID_TRACE_ENABLE,&bDbgTrace);
	bDbgTrace ? StartTrace() : StopTrace();
	EnableMenuItem(GetMenu(hDlg),ID_TRACE_SETTINGS,bDbgTrace ? MF_GRAYED : MF_ENABLED);
	return 0;
}
