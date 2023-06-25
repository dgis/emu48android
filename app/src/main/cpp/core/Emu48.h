/*
 *   Emu48.h
 *
 *   This file is part of Emu48
 *
 *   Copyright (C) 1995 Sebastien Carlier
 *
 */
#include "types.h"

#define	_KB(a)			((a)*2*1024)

#define HARDWARE		"Yorke"				// emulator hardware
#define MODELS			"26AEGPQSX"			// valid calculator models
#define APPLEHARD		"2PQ"				// Apple platform calculator models

#define	ARRAYSIZEOF(a)	(sizeof(a) / sizeof(a[0]))

// cards status
#define PORT1_PRESENT	((cCurrentRomType=='S')?P1C:P2C)
#define PORT1_WRITE		((cCurrentRomType=='S')?P1W:P2W)
#define PORT2_PRESENT	((cCurrentRomType=='S')?P2C:P1C)
#define PORT2_WRITE		((cCurrentRomType=='S')?P2W:P1W)

#define BINARYHEADER48	"HPHP48-W"
#define BINARYHEADER49	"HPHP49-W"

#define BIN_FILTER		"Port Data File (*.BIN)\0*.BIN\0All Files (*.*)\0*.*\0"
#define HP_FILTER		"HP Binary Object (*.HP;*.LIB)\0*.HP;*.LIB\0All Files (*.*)\0*.*\0"

#define CF_HPOBJ		"CF_HPOBJ"			// clipboard format for DDE

#define SM_RUN			0					// states of cpu emulation thread
#define SM_INVALID		1
#define SM_RETURN		2
#define SM_SLEEP		3

#define S_ERR_NO		0					// stack errorcodes
#define S_ERR_OBJECT	1
#define S_ERR_BINARY	2
#define S_ERR_ASCII		3

#define BAD_OB			(0xFFFFFFFF)		// bad object

#define NO_SERIAL		"disabled"			// port not open

#define HP_MNEMONICS	FALSE				// disassembler mnenomics mode
#define CLASS_MNEMONICS	TRUE

#define MACRO_OFF	0						// macro recorder off
#define MACRO_NEW	1
#define MACRO_PLAY	2

#define DISP_POINTER	0x01				// defines for display area
#define DISP_MAIN		0x02
#define DISP_MENUE		0x04

#define ROMPAGESIZE		(1<<12)				// ROM dirty page size in nibbles

// window styles
#define STYLE_TITLE		(WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_OVERLAPPED)
#define STYLE_NOTITLE	(WS_POPUP|WS_SYSMENU|WS_MINIMIZEBOX|WS_CLIPSIBLINGS)

// WM_COPYDATA identifier
#define CDID_FILENAME	1					// send file name

// macro to check for valid calculator model
#define isModelValid(m)	(m != 0 && strchr(MODELS,m) != NULL)
#define isModelApple(m) (m != 0 && strchr(APPLEHARD,m) != NULL)

// values for mapping area
enum MMUMAP { M_IO, M_ROM, M_RAM, M_P1, M_P2, M_BS };

// values for disassembler memory mapping modes
enum MEM_MAPPING { MEM_MMU, MEM_NCE1, MEM_NCE2, MEM_CE1, MEM_CE2, MEM_NCE3 };

// Kermit CRC calculation
static __inline WORD UpCRC(WORD wCRC, BYTE nib) { return (wCRC >> 4) ^ (((wCRC ^ nib) & 0xf) * 0x1081); }

// Emu48.c
extern HPALETTE			hPalette;
extern HPALETTE			hOldPalette;
extern HANDLE			hEventShutdn;
extern LPTSTR			szAppName;
extern LPTSTR			szTopic;
extern LPTSTR			szTitle;
extern CRITICAL_SECTION	csGDILock;
extern CRITICAL_SECTION csLcdLock;
extern CRITICAL_SECTION	csKeyLock;
extern CRITICAL_SECTION	csIOLock;
extern CRITICAL_SECTION	csT1Lock;
extern CRITICAL_SECTION	csT2Lock;
extern CRITICAL_SECTION csTxdLock;
extern CRITICAL_SECTION	csRecvLock;
extern CRITICAL_SECTION csSlowLock;
extern CRITICAL_SECTION csDbgLock;
extern INT				nArgc;
extern LPCTSTR			*ppArgv;
extern LARGE_INTEGER	lFreq;
extern LARGE_INTEGER	lAppStart;
extern DWORD			idDdeInst;
extern UINT				uCF_HpObj;
extern HINSTANCE		hApp;
extern HWND				hWnd;
extern HWND				hDlgDebug;
extern HWND				hDlgFind;
extern HWND				hDlgProfile;
extern HWND				hDlgRplObjView;
extern HDC				hWindowDC;
extern DWORD			dwTColor;
extern DWORD			dwTColorTol;
extern HRGN				hRgn;
extern HCURSOR			hCursorArrow;
extern HCURSOR			hCursorHand;
extern UINT				uWaveDevId;
extern DWORD			dwWakeupDelay;
extern BOOL				bAutoSave;
extern BOOL				bAutoSaveOnExit;
extern BOOL				bSaveDefConfirm;
extern BOOL				bStartupBackup;
extern BOOL				bAlwaysDisplayLog;
extern BOOL				bLoadObjectWarning;
extern BOOL				bShowTitle;
extern BOOL				bShowMenu;
extern BOOL				bAlwaysOnTop;
extern BOOL				bActFollowsMouse;
extern BOOL				bClientWinMove;
extern BOOL				bSingleInstance;
extern HANDLE			hThread;
extern VOID				SetWindowTitle(LPCTSTR szString);
extern VOID				ForceForegroundWindow(HWND hWnd);
extern VOID				CopyItemsToClipboard(HWND hWnd);

// mru.c
extern BOOL    MruInit(UINT nNum);
extern VOID    MruCleanup(VOID);
extern VOID    MruAdd(LPCTSTR lpszEntry);
extern VOID    MruRemove(UINT nIndex);
extern VOID    MruMoveTop(UINT nIndex);
extern UINT    MruEntries(VOID);
extern UINT    MruID(LPCTSTR lpszEntry);
extern VOID    MruFilename(UINT nIndex, LPTSTR szFilename, UINT nBuffersize);
extern VOID    MruUpdateMenu(HMENU hMenu);
extern VOID    MruWriteList(VOID);
extern VOID    MruReadList(VOID);

// Settings.c
extern VOID ReadSettings(VOID);
extern VOID WriteSettings(VOID);
extern VOID ReadLastDocument(LPTSTR szFileName, DWORD nSize);
extern VOID WriteLastDocument(LPCTSTR szFilename);
extern VOID ReadSettingsString(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpDefault, LPTSTR lpData, DWORD dwSize);
extern VOID WriteSettingsString(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPTSTR lpData);
extern INT  ReadSettingsInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, INT nDefault);
extern VOID WriteSettingsInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, INT nValue);
extern VOID DelSettingsKey(LPCTSTR lpszSection, LPCTSTR lpszEntry);

// Display.c
extern BOOL bGrayscale;
extern UINT nBackgroundX;
extern UINT nBackgroundY;
extern UINT nBackgroundW;
extern UINT nBackgroundH;
extern UINT nLcdX;
extern UINT nLcdY;
extern UINT nLcdZoom;
extern UINT nGdiXZoom;
extern UINT nGdiYZoom;
extern HDC  hLcdDC;
extern HDC  hMainDC;
extern HDC  hAnnunDC;
extern BYTE (*GetLineCounter)(VOID);
extern VOID (*StartDisplay)(BYTE byInitial);
extern VOID (*StopDisplay)(VOID);
extern VOID UpdateContrast(BYTE byContrast);
extern VOID SetLcdColor(UINT nId, UINT nRed, UINT nGreen, UINT nBlue);
extern VOID SetLcdMode(BOOL bMode);
extern VOID CreateLcdBitmap(VOID);
extern VOID DestroyLcdBitmap(VOID);
extern BOOL CreateMainBitmap(LPCTSTR szFilename);
extern VOID DestroyMainBitmap(VOID);
extern BOOL CreateAnnunBitmap(LPCTSTR szFilename);
extern VOID DestroyAnnunBitmap(VOID);
extern VOID UpdateDisplayPointers(VOID);
extern VOID UpdateMainDisplay(VOID);
extern VOID UpdateMenuDisplay(VOID);
extern VOID RefreshDisp0();    // CdB for HP: add apples display management
extern VOID WriteToMainDisplay(LPBYTE a, DWORD d, UINT s);
extern VOID WriteToMenuDisplay(LPBYTE a, DWORD d, UINT s);
extern VOID UpdateAnnunciators(DWORD dwUpdateMask);
extern VOID ResizeWindow(VOID);

// Engine.c
extern BOOL    bInterrupt;
extern UINT    nState;
extern UINT    nNextState;
extern BOOL    bEnableSlow;
extern BOOL    bRealSpeed;
extern BOOL    bKeySlow;
extern BOOL    bSoundSlow;
extern UINT    nOpcSlow;
extern CHIPSET Chipset;
extern TCHAR   szSerialWire[16];
extern TCHAR   szSerialIr[16];
extern DWORD   dwSXCycles;
extern DWORD   dwGXCycles;
extern DWORD   dwGPCycles;    // CdB for HP: add apples speed
extern DWORD   dwG2Cycles;    // CdB for HP: add apples speed
extern DWORD   dwT2Cycles;
extern HANDLE  hEventDebug;
extern BOOL    bDbgAutoStateCtrl;
extern INT     nDbgState;
extern BOOL    bDbgNOP3;
extern BOOL    bDbgCode;
extern BOOL    bDbgRPL;
extern BOOL    bDbgSkipInt;
extern DWORD   dwDbgStopPC;
extern DWORD   dwDbgRplPC;
extern DWORD   dwDbgRstkp;
extern DWORD   dwDbgRstk;
extern DWORD   *pdwInstrArray;
extern WORD    wInstrSize;
extern WORD    wInstrWp;
extern WORD    wInstrRp;
extern VOID    (*fnOutTrace)(VOID);
extern VOID    SuspendDebugger(VOID);
extern VOID    ResumeDebugger(VOID);
extern VOID    CheckSerial(VOID);
extern VOID    InitAdjustSpeed(VOID);
extern VOID    AdjKeySpeed(VOID);
extern VOID    SetSpeed(BOOL bAdjust);
extern VOID    UpdateKdnBit(VOID);
extern BOOL    WaitForSleepState(VOID);
extern UINT    SwitchToState(UINT nNewState);
extern UINT    WorkerThread(LPVOID pParam);

// Fetch.c
extern VOID    EvalOpcode(LPBYTE I);

// Files.c
extern TCHAR   szEmuDirectory[MAX_PATH];
extern TCHAR   szRomDirectory[MAX_PATH];
extern TCHAR   szCurrentDirectory[MAX_PATH];
extern TCHAR   szCurrentKml[MAX_PATH];
extern TCHAR   szBackupKml[MAX_PATH];
extern TCHAR   szCurrentFilename[MAX_PATH];
extern TCHAR   szBackupFilename[MAX_PATH];
extern TCHAR   szBufferFilename[MAX_PATH];
extern TCHAR   szPort2Filename[MAX_PATH];
extern BOOL    bDocumentAvail;
extern BYTE    cCurrentRomType;
extern UINT    nCurrentClass;
extern LPBYTE  Port0;
extern LPBYTE  Port1;
extern LPBYTE  Port2;
extern LPBYTE  pbyRom;
extern BOOL    bRomWriteable;
extern DWORD   dwRomSize;
extern LPBYTE  pbyRomDirtyPage;
extern DWORD   dwRomDirtyPageSize;
extern WORD    wRomCrc;
extern BOOL    bRomCrcCorrection;
extern LPBYTE  pbyPort2;
extern BOOL    bPort2Writeable;
extern BOOL    bPort2IsShared;
extern DWORD   dwPort2Size;
extern DWORD   dwPort2Mask;
extern WORD    wPort2Crc;
extern BOOL    bBackup;
extern VOID    SetWindowLocation(HWND hWnd,INT nPosX,INT nPosY);
extern DWORD   GetCutPathName(LPCTSTR szFileName,LPTSTR szBuffer,DWORD dwBufferLength,INT nCutLength);
extern VOID    SetWindowPathTitle(LPCTSTR szFileName);
extern BOOL    CheckForBeepPatch(VOID);
extern BOOL    PatchNibble(DWORD dwAddress, BYTE byPatch);
extern VOID    UpdatePatches(BOOL bPatch);
extern BOOL    PatchRom(LPCTSTR szFilename);
extern BOOL    CrcRom(WORD *pwChk);
extern BOOL    MapRom(LPCTSTR szFilename);
extern VOID    UnmapRom(VOID);
extern BOOL    MapPort2(LPCTSTR szFilename);
extern VOID    UnmapPort2(VOID);
extern VOID    ResetDocument(VOID);
extern BOOL    NewDocument(VOID);
extern BOOL    OpenDocument(LPCTSTR szFilename);
extern BOOL    SaveDocument(VOID);
extern BOOL    SaveDocumentAs(LPCTSTR szFilename);
extern BOOL    SaveBackup(VOID);
extern BOOL    RestoreBackup(VOID);
extern BOOL    ResetBackup(VOID);
extern BOOL    GetOpenFilename(VOID);
extern BOOL    GetSaveAsFilename(VOID);
extern BOOL    GetLoadObjectFilename(LPCTSTR lpstrFilter,LPCTSTR lpstrDefExt);
extern BOOL    GetSaveObjectFilename(LPCTSTR lpstrFilter,LPCTSTR lpstrDefExt);
extern WORD    WriteStack(UINT nStkLevel,LPBYTE lpBuf,DWORD dwSize);
extern BOOL    LoadObject(LPCTSTR szFilename);
extern BOOL    SaveObject(LPCTSTR szFilename);
extern BOOL    LoadIconFromFile(LPCTSTR szFilename);
extern VOID    LoadIconDefault(VOID);
extern HBITMAP LoadBitmapFile(LPCTSTR szFilename,BOOL bPalette);
extern HRGN    CreateRgnFromBitmap(HBITMAP hBmp,COLORREF color,DWORD dwTol);

// Romcrc.c
extern VOID RebuildRomCrc(VOID);

// Timer.c
extern VOID  SetHP48Time(VOID);
extern VOID  StartTimers(VOID);
extern VOID  StopTimers(VOID);
extern DWORD ReadT2(VOID);
extern VOID  SetT2(DWORD dwValue);
extern BYTE  ReadT1(VOID);
extern VOID  SetT1(BYTE byValue);

// Mops.c
extern BOOL        bFlashRomArray;
extern BYTE        disp;
extern LPBYTE      RMap[256];
extern LPBYTE      WMap[256];
extern DWORD       FlashROMAddr(DWORD d);
extern VOID        Map(BYTE a, BYTE b);
extern VOID        RomSwitch(DWORD adr);
extern VOID        Config(VOID);
extern VOID        Uncnfg(VOID);
extern VOID        Reset(VOID);
extern VOID        C_Eq_Id(VOID);
extern enum MMUMAP MapData(DWORD d);
extern VOID        CpuReset(VOID);
extern VOID        Npeek(BYTE *a, DWORD d, UINT s);
extern VOID        Nread(BYTE *a, DWORD d, UINT s);
extern VOID        Nwrite(BYTE *a, DWORD d, UINT s);
extern BYTE        Read2(DWORD d);
extern DWORD       Read5(DWORD d);
extern VOID        Write5(DWORD d, DWORD n);
extern VOID        Write2(DWORD d, BYTE n);
extern VOID        IOBit(DWORD d, BYTE b, BOOL s);
extern VOID        ReadIO(BYTE *a, DWORD b, DWORD s, BOOL bUpdate);
extern VOID        WriteIO(BYTE *a, DWORD b, DWORD s);

// Lowbat.c
extern BOOL bLowBatDisable;
extern VOID StartBatMeasure(VOID);
extern VOID StopBatMeasure(VOID);
extern VOID GetBatteryState(BOOL *pbLBI, BOOL *pbVLBI);

// Keyboard.c
extern DWORD dwKeyMinDelay;
extern VOID  ScanKeyboard(BOOL bActive, BOOL bReset);
extern VOID  KeyboardEvent(BOOL bPress, UINT out, UINT in);

// Keymacro.c
extern INT     nMacroState;
extern INT     nMacroTimeout;
extern BOOL    bMacroRealSpeed;
extern DWORD   dwMacroMinDelay;
extern VOID    KeyMacroRecord(BOOL bPress, UINT out, UINT in);
extern LRESULT OnToolMacroNew(VOID);
extern LRESULT OnToolMacroPlay(VOID);
extern LRESULT OnToolMacroStop(VOID);
extern LRESULT OnToolMacroSettings(VOID);

// Redeye.c
extern VOID IrPrinter(BYTE c);

// Udp.c
extern TCHAR szUdpServer[1024];
extern WORD  wUdpPort;
extern VOID  ResetUdp(VOID);
extern BOOL  SendByteUdp(BYTE byData);

// Stack.c
extern BOOL    bDetectClpObject;
extern LRESULT OnStackCopy(VOID);
extern LRESULT OnStackPaste(VOID);

// RPL.c
extern BOOL    RPL_GetSystemFlag(INT nFlag);
extern DWORD   RPL_SkipOb(DWORD d);
extern DWORD   RPL_ObjectSize(BYTE *o,DWORD s);
extern DWORD   RPL_CreateTemp(DWORD l,BOOL bGarbageCol);
extern UINT    RPL_Depth(VOID);
extern DWORD   RPL_Pick(UINT l);
extern VOID    RPL_Replace(DWORD n);
extern VOID    RPL_Push(UINT l,DWORD n);

// SndEnum.c
extern VOID SetSoundDeviceList(HWND hWnd,UINT uDeviceID);

// Sound.c
extern DWORD dwWaveVol;
extern DWORD dwWaveTime;
extern BOOL  SoundAvailable(UINT uDeviceID);
extern BOOL  SoundGetDeviceID(UINT *puDeviceID);
extern BOOL  SoundOpen(UINT uDeviceID);
extern VOID  SoundClose(VOID);
extern VOID  SoundOut(CHIPSET* w, WORD wOut);
extern VOID  SoundBeep(DWORD dwFrequency, DWORD dwDuration);

// DDEserv.c
extern HDDEDATA CALLBACK DdeCallback(UINT, UINT, HCONV, HSZ, HSZ, HDDEDATA, DWORD, DWORD);

// Dismem.c
extern BOOL             SetMemRomType(BYTE cCurrentRomType);
extern BOOL             SetMemMapType(enum MEM_MAPPING eType);
extern enum MEM_MAPPING GetMemMapType(VOID);
extern BOOL             GetMemAvail(enum MEM_MAPPING eType);
extern DWORD            GetMemDataSize(VOID);
extern DWORD            GetMemDataMask(VOID);
extern BYTE             GetMemNib(DWORD *p);
extern VOID             GetMemPeek(BYTE *a, DWORD d, UINT s);

// Disasm.c
extern BOOL  disassembler_mode;
extern BOOL  disassembler_symb;
extern DWORD disassemble(DWORD addr, LPTSTR out);

// Symbfile.c
extern BOOL    RplTableEmpty(VOID);
extern BOOL    RplLoadTable(LPCTSTR lpszFilename);
extern VOID    RplDeleteTable(VOID);
extern LPCTSTR RplGetName(DWORD dwAddr);
extern BOOL    RplGetAddr(LPCTSTR lpszName, DWORD *pdwAddr);

// Serial.c
extern BOOL CommIsOpen(VOID);
extern BOOL CommOpen(LPTSTR strWirePort,LPTSTR strIrPort);
extern VOID CommClose(VOID);
extern VOID CommSetBaud(VOID);
extern BOOL UpdateUSRQ(VOID);
extern VOID CommTxBRK(VOID);
extern VOID CommTransmit(VOID);
extern VOID CommReceive(VOID);

// Cursor.c
extern HCURSOR CreateHandCursor(VOID);

#if defined _USRDLL							// DLL version
// Emu48dll.c
extern VOID (CALLBACK *pEmuDocumentNotify)(LPCTSTR lpszFilename);
extern BOOL DLLCreateWnd(LPCTSTR lpszFilename, LPCTSTR lpszPort2Name);
extern BOOL DLLDestroyWnd(VOID);

// Symbfile.c
#define RplGetName(a)		NULL			// for linking
#endif

// Message Boxes
static __inline int InfoMessage(LPCTSTR szMessage)  {return MessageBox(hWnd, szMessage, szTitle, MB_APPLMODAL|MB_OK|MB_ICONINFORMATION|MB_SETFOREGROUND);}
static __inline int AbortMessage(LPCTSTR szMessage) {return MessageBox(hWnd, szMessage, szTitle, MB_APPLMODAL|MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);}
static __inline int YesNoMessage(LPCTSTR szMessage) {return MessageBox(hWnd, szMessage, szTitle, MB_APPLMODAL|MB_YESNO|MB_ICONEXCLAMATION|MB_SETFOREGROUND);}
static __inline int YesNoCancelMessage(LPCTSTR szMessage,UINT uStyle) {return MessageBox(hWnd, szMessage, szTitle, MB_APPLMODAL|MB_YESNOCANCEL|MB_ICONEXCLAMATION|MB_SETFOREGROUND|uStyle);}

// Missing Win32 API calls
static __inline LPTSTR DuplicateString(LPCTSTR szString)
{
	UINT   uLength = lstrlen(szString) + 1;
	LPTSTR szDup   = (LPTSTR) malloc(uLength*sizeof(szDup[0]));
	lstrcpy(szDup,szString);
	return szDup;
}

#define SCREENHEIGHT (cCurrentRomType=='Q' ? 80 : 64)   // CdB for HP: add apples display management
#define SCREENHEIGHTREAL ((cCurrentRomType=='Q') ? (80-Chipset.d0size) : 64)
#define MAINSCREENHEIGHT	(((Chipset.lcounter) == 0) ? SCREENHEIGHTREAL : SCREENHEIGHTREAL-64+((Chipset.lcounter)+1))
#define MENUHEIGHT (Chipset.lcounter==0?0:64-(Chipset.lcounter+1))
