#pragma once

#include <stdint.h>
#include <pthread.h>
#include <wchar.h>
#include <stdio.h>
#include <string.h>
#include <jni.h>
#include <android/bitmap.h>
#include <android/asset_manager.h>

#ifndef __OBJC__
typedef signed char BOOL;   // deliberately same type as defined in objc
#endif
#define TRUE    1
#define FALSE   0
#define MAX_PATH PATH_MAX
#define INFINITE    0xFFFFFFFF //0
#define FAR
#define NEAR

typedef unsigned long ULONG;
//typedef	unsigned long ulong;	// ushort is already in types.h
typedef short SHORT;
typedef unsigned short USHORT;
typedef uint64_t ULONGLONG;
typedef int64_t LONGLONG;
typedef int64_t __int64;
typedef float FLOAT;

typedef unsigned char BYTE;
typedef BYTE byte;
typedef unsigned short WORD;
typedef WORD *LPWORD;
typedef uint32_t DWORD;
typedef DWORD *LPDWORD;
typedef BYTE *LPBYTE;
typedef uint16_t WORD;
typedef uint32_t UINT;
typedef int32_t INT;
typedef int INT_PTR, *PINT_PTR;
typedef char CHAR;
typedef void VOID;
typedef void *LPVOID;
typedef void *PVOID;
//typedef long LONG;
typedef int32_t LONG;
typedef LONG *PLONG;
typedef unsigned int UINT_PTR, *PUINT_PTR;
//typedef long LONG_PTR, *PLONG_PTR;
typedef LONG   LONG_PTR,  *PLONG_PTR;
typedef size_t SIZE_T;
typedef /*_W64*/ unsigned long ULONG_PTR, *PULONG_PTR;
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;

#define MAXLONG     0x7fffffff

#define CONST const

#define _ASSERT(expr)
//#define _ASSERT(expr) \
//	(void)(                                                                                     \
//		(!!(expr)) ||                                                                           \
//		(1 != _CrtDbgReportW(_CRT_ASSERT, _CRT_WIDE(__FILE__), __LINE__, NULL, L"%ls", NULL)) || \
//		(_CrtDbgBreak(), 0)                                                                     \
//	)


enum HANDLE_TYPE {
	HANDLE_TYPE_INVALID = 0,
	HANDLE_TYPE_FILE,
    HANDLE_TYPE_FILE_ASSET,
    HANDLE_TYPE_FILE_MAPPING,
    HANDLE_TYPE_FILE_MAPPING_ASSET,
    HANDLE_TYPE_EVENT,
    HANDLE_TYPE_THREAD,
};
typedef struct {
    enum HANDLE_TYPE handleType;

	int fileDescriptor;

    AAsset* fileAsset;

    size_t fileMappingSize;
    void* fileMappingAddress;

    pthread_t threadId;

    pthread_cond_t eventCVariable;
    pthread_mutex_t eventMutex;
    BOOL eventAutoReset;
    BOOL eventState;
} _HANDLE;
typedef _HANDLE * HANDLE;

typedef HANDLE HMENU;

#define MK_LBUTTON          0x0001


typedef void *TIMECALLBACK ;
#define CALLBACK
typedef void(*LPTIMECALLBACK)(UINT uEventId, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2) ;
//typedef TIMECALLBACK *LPTIMECALLBACK;
//typedef long LRESULT;
typedef UINT_PTR            WPARAM;
typedef LONG_PTR            LPARAM;
typedef LONG_PTR            LRESULT;
typedef struct _RECT {
	short left;
	short right;
	short top;
	short bottom;
} RECT;
typedef RECT *LPRECT;
typedef BYTE *LPCVOID;
typedef struct _RGBQUAD {
	BYTE rgbRed;
	BYTE rgbGreen;
	BYTE rgbBlue;
	BYTE rgbReserved;
} RGBQUAD;

#define LowPart d.LwPart
typedef union _LARGE_INTEGER {
	uint64_t QuadPart;
	struct {
#ifdef __LITTLE_ENDIAN__
		uint32_t LwPart;
		uint32_t HiPart;
#else
		uint32_t HiPart;
        uint32_t LwPart;
#endif
	}d;
} LARGE_INTEGER, *PLARGE_INTEGER;

typedef char *LPTSTR;
typedef char *LPSTR;
//typedef char LPCSTR[255];
typedef const char *LPCSTR;
typedef const char *LPCTSTR;
#ifdef UNICODE
typedef wchar_t TCHAR;
#else
typedef char TCHAR;
#endif // !UNICODE


typedef pthread_mutex_t CRITICAL_SECTION;
typedef HANDLE HINSTANCE;
typedef HANDLE HWND;
typedef void WNDCLASS;

typedef HANDLE HCURSOR;

struct FILETIME {
	DWORD dwLowDateTime;
	DWORD dwHighDateTime;
};

struct SYSTEMTIME
{
	DWORD wYear;
	DWORD wMonth;
	DWORD wDay;
	DWORD wHour;
	DWORD wMinute;
	DWORD wSecond;
	DWORD wMilliseconds;
};
#define CALLBACK
typedef int HDDEDATA; //ignored in this port
typedef int HCONV;//ignored in this port
typedef int HSZ;//ignored in this port

struct TIMECAPS
{
	UINT wPeriodMin;
	UINT wPeriodMax;
};

struct DCB
{
	DWORD DCBlength;
	DWORD BaudRate;
	BOOL fBinary,
			fParity,
			fOutxCtsFlow,
			fOutxDsrFlow,
			fOutX,
			fErrorChar,
			fNull,
			fAbortOnError;

	WORD fDtrControl,
			fDsrSensitivity,
			fRtsControl,
			ByteSize,
			Parity,
			StopBits;
};
enum
{
	DTR_CONTROL_DISABLE,
	RTS_CONTROL_DISABLE,
	NOPARITY,
	ONESTOPBIT
};
struct COMMTIMEOUTS
{
	DWORD a,b,c,d,e;
};
/*
struct OVERLAPPED
{
	HANDLE hEvent;
};
*/

typedef struct _SYSTEM_POWER_STATUS
{
	BYTE  ACLineStatus;
	BYTE  BatteryFlag;
	BYTE  BatteryLifePercent;
	BYTE  Reserved1;
	DWORD BatteryLifeTime;
	DWORD BatteryFullLifeTime;
} SYSTEM_POWER_STATUS, *LPSYSTEM_POWER_STATUS;

#define __cdecl
#define WINAPI


enum
{
	GENERIC_READ   = 1,
	GENERIC_WRITE  = 2,
	FILE_SHARE_READ  = 1,
	FILE_SHARE_WRITE = 2,
	OPEN_EXISTING = 121,
	FILE_ATTRIBUTE_NORMAL = 1,
	FILE_FLAG_OVERLAPPED  = 2,
	FILE_FLAG_SEQUENTIAL_SCAN = 4,
	PAGE_READONLY,
	CREATE_ALWAYS,
	PAGE_READWRITE,
	PAGE_WRITECOPY,
	FILE_MAP_COPY,

	//errors
			INVALID_HANDLE_VALUE = -1,
	ERROR_ALREADY_EXISTS = 1
};

#define FILE_MAP_WRITE            0x0002
#define FILE_MAP_READ             0x0004

enum MsgBoxFlagType {
	IDOK     = 0,
	IDYES    = 1,
	IDNO     = 2,
	IDCANCEL = 3,
	MB_OK              = 1,
	MB_YESNO           = 1 << 1,
	MB_YESNOCANCEL     = 1 << 2,
	MB_ICONWARNING     = 1 << 3,
	MB_ICONERROR       = 1 << 4,
	MB_ICONSTOP        = 1 << 5,
	MB_ICONINFORMATION = 1 << 6,
	MB_ICONQUESTION    = 1 << 7,
	MB_ICONEXCLAMATION = 1 << 8,
	MB_DEFBUTTON2      = 1 << 9
};

#define MB_APPLMODAL        0
#define MB_SETFOREGROUND    0

// These are passed to tab control callbacks
enum {
	WM_INITDIALOG,	// fills in tab controls when first shown
	WM_COMMAND,	// respond to validation/dimming commands
	WM_GETDLGVALUES	// read off the control settings
};
#define WM_SYSCOMMAND                   0x0112

// Constants for the msg parameter in SendDlgItemMessage()
enum {
	CB_ERR = -1,
	CB_GETCURSEL = 1,
	CB_GETLBTEXT,
	CB_RESETCONTENT,
	CB_SETCURSEL,
	CB_ADDSTRING,
	WMU_SETWINDOWVALUE
};

// Constants for SetFilePointer
enum FilePointerType {
	INVALID_SET_FILE_POINTER = -1,
	FILE_BEGIN = 1,
	FILE_CURRENT,
	FILE_END
};

// Supposedly specifies that Windows use default window coords
enum { CW_USEDEFAULT };

enum {
	AC_LINE_OFFLINE,
	AC_LINE_ONLINE,
	AC_LINE_UNKNOWN = 255
};

enum {
	BATTERY_FLAG_HIGH = 1,
	BATTERY_FLAG_LOW = 2,
	BATTERY_FLAG_CRITICAL = 4,
	BATTERY_FLAG_CHARGING = 8,
	BATTERY_FLAG_NO_BATTERY = 128,
	BATTERY_FLAG_UNKNOWN = 255
};


// Macros
#ifdef _T
#undef _T
//#define _T(x)	@x
#endif
#define _T(x)   x

#define TEXT(x)	@x
#define LOWORD(x)	x
#define MAKELONG(a, b)      ((LONG)(((WORD)(((DWORD_PTR)(a)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(b)) & 0xffff))) << 16))

#define HIBYTE(i)   ((i)>>8)
#define LOBYTE(i)   (i)
#define _istdigit(c)    ((c)>='0' && (c)<='9')
#define lstrlen(s)      strlen(s)
#define lstrcpy(d,s)    strcpy(d,s)
#define _tcsspn(s,k)    strspn(s,k)
#define wsprintf(s,f,...)   sprintf(s,f, ## __VA_ARGS__)
#define HeapAlloc(h,v,s)    malloc(s)
#define HeapFree(h,v,p)     do{free(p);(p)=v;}while(0)
#define ZeroMemory(p,s)     memset(p,0,s)
#define FillMemory(p,n,v)   memset(p,v,n*sizeof(*(p)))
#define CopyMemory(d,src,s) memcpy(d,src,s)

typedef struct _OVERLAPPED {
/*
    ULONG_PTR Internal;
    ULONG_PTR InternalHigh;
    union {
        struct {
            DWORD Offset;
            DWORD OffsetHigh;
        } DUMMYSTRUCTNAME;
        PVOID Pointer;
    } DUMMYUNIONNAME;
*/

    HANDLE  hEvent;
} OVERLAPPED, *LPOVERLAPPED;


extern VOID OutputDebugString(LPCSTR lpOutputString);

extern DWORD GetCurrentDirectory(DWORD nBufferLength, LPTSTR lpBuffer);
extern BOOL SetCurrentDirectory(LPCTSTR);	// returns NO if fails
extern HANDLE CreateFile(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPVOID lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, LPVOID hTemplateFile);
extern BOOL ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
extern DWORD SetFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod);
extern DWORD GetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh);
extern BOOL WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
typedef struct _SECURITY_ATTRIBUTES {
    DWORD nLength;
    LPVOID lpSecurityDescriptor;
    BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;
extern HANDLE CreateFileMapping(HANDLE hFile, LPSECURITY_ATTRIBUTES lpFileMappingAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCSTR lpName);
extern LPVOID MapViewOfFile(HANDLE hFileMappingObject,DWORD dwDesiredAccess, DWORD dwFileOffsetHigh,DWORD dwFileOffsetLow, SIZE_T dwNumberOfBytesToMap);
extern BOOL UnmapViewOfFile(LPCVOID lpBaseAddress);
extern BOOL SetEndOfFile(HANDLE hFile);

typedef UINT_PTR (CALLBACK *LPOFNHOOKPROC) (HWND, UINT, WPARAM, LPARAM);
typedef struct tagOFNA {
    DWORD        lStructSize;
    HWND         hwndOwner;
    HINSTANCE    hInstance;
    LPCSTR       lpstrFilter;
    LPSTR        lpstrCustomFilter;
    DWORD        nMaxCustFilter;
    DWORD        nFilterIndex;
    LPSTR        lpstrFile;
    DWORD        nMaxFile;
    LPSTR        lpstrFileTitle;
    DWORD        nMaxFileTitle;
    LPCSTR       lpstrInitialDir;
    LPCSTR       lpstrTitle;
    DWORD        Flags;
    WORD         nFileOffset;
    WORD         nFileExtension;
    LPCSTR       lpstrDefExt;
    LPARAM       lCustData;
    LPOFNHOOKPROC lpfnHook;
    LPCSTR       lpTemplateName;
#ifdef _MAC
    LPEDITMENU   lpEditInfo;
   LPCSTR       lpstrPrompt;
#endif
#if (_WIN32_WINNT >= 0x0500)
    void *        pvReserved;
   DWORD        dwReserved;
   DWORD        FlagsEx;
#endif // (_WIN32_WINNT >= 0x0500)
} OPENFILENAMEA, *LPOPENFILENAMEA;
typedef OPENFILENAMEA OPENFILENAME;
typedef LPOPENFILENAMEA LPOPENFILENAME;
#define OFN_READONLY                 0x00000001
#define OFN_OVERWRITEPROMPT          0x00000002
#define OFN_HIDEREADONLY             0x00000004
#define OFN_PATHMUSTEXIST            0x00000800
#define OFN_FILEMUSTEXIST            0x00001000
#define OFN_CREATEPROMPT             0x00002000
#define OFN_EXPLORER                 0x00080000     // new look commdlg
extern BOOL GetOpenFileName(LPOPENFILENAME);
extern BOOL GetSaveFileName(LPOPENFILENAME);
#define IMAGE_BITMAP        0
#define IMAGE_ICON          1
#define LR_LOADFROMFILE     0x00000010
#define LR_DEFAULTSIZE      0x00000040
#define LR_SHARED           0x00008000
extern HANDLE LoadImage(HINSTANCE hInst, LPCSTR name, UINT type, int cx, int cy, UINT fuLoad);

#define WM_USER                         0x0400
#define WM_SETICON                      0x0080
#define ICON_SMALL          0
#define ICON_BIG            1
// message from browser
#define BFFM_INITIALIZED        1
#define BFFM_SELCHANGED         2
#define BFFM_SETSTATUSTEXTA     (WM_USER + 100)
#define BFFM_SETSELECTIONA      (WM_USER + 102)
#define BFFM_SETSTATUSTEXT  BFFM_SETSTATUSTEXTA
#define BFFM_SETSELECTION   BFFM_SETSELECTIONA
#define CB_GETITEMDATA              0x0150
#define CB_SETITEMDATA              0x0151
#define TBM_GETPOS              (WM_USER)
#define TBM_SETPOS              (WM_USER+5)
#define TBM_SETRANGE            (WM_USER+6)
#define TBM_SETTICFREQ          (WM_USER+20)
#define BST_CHECKED        0x0001
extern LRESULT SendMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
extern BOOL PostMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
extern int MessageBox(HANDLE, LPCTSTR szMessage, LPCTSTR szTitle, int flags);
extern BOOL QueryPerformanceFrequency(PLARGE_INTEGER l);
extern BOOL QueryPerformanceCounter(PLARGE_INTEGER l);
extern DWORD timeGetTime(void);

#define WAIT_OBJECT_0   0
#define WAIT_TIMEOUT    0x00000102
#define WAIT_FAILED     0xFFFFFFFF
extern DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);

extern void Sleep(int);
#define UNREFERENCED_PARAMETER(a)

extern BOOL GetSystemPowerStatus(LPSYSTEM_POWER_STATUS l);

// Event

extern HANDLE CreateEvent(LPVOID lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCTSTR name);
extern BOOL SetEvent(HANDLE hEvent);
extern BOOL ResetEvent(HANDLE hEvent);

// Thread

typedef DWORD (*PTHREAD_START_ROUTINE)(
        LPVOID lpThreadParameter
);
typedef PTHREAD_START_ROUTINE LPTHREAD_START_ROUTINE;
#define CREATE_SUSPENDED                  0x00000004
extern HANDLE CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);
extern DWORD ResumeThread(HANDLE hThread);
extern BOOL CloseHandle(HANDLE hObject);

extern void EnterCriticalSection(CRITICAL_SECTION *);
extern void LeaveCriticalSection(CRITICAL_SECTION *);

// GDI
typedef struct __attribute__((packed)) tagBITMAP
{
    LONG        bmType;
    LONG        bmWidth;
    LONG        bmHeight;
    LONG        bmWidthBytes;
    WORD        bmPlanes;
    WORD        bmBitsPixel;
    LPVOID      bmBits;
} BITMAP, *PBITMAP, NEAR *NPBITMAP, FAR *LPBITMAP;
/* constants for the biCompression field */
#define BI_RGB        0L
#define BI_RLE8       1L
#define BI_RLE4       2L
#define BI_BITFIELDS  3L
#define BI_JPEG       4L
#define BI_PNG        5L
typedef struct __attribute__((packed)) tagBITMAPFILEHEADER {
    WORD    bfType;
    DWORD   bfSize;
    WORD    bfReserved1;
    WORD    bfReserved2;
    DWORD   bfOffBits;
} BITMAPFILEHEADER, FAR *LPBITMAPFILEHEADER, *PBITMAPFILEHEADER;
typedef struct __attribute__((packed)) tagBITMAPINFOHEADER{
    DWORD      biSize;
    LONG       biWidth;
    LONG       biHeight;
    WORD       biPlanes;
    WORD       biBitCount;
    DWORD      biCompression;
    DWORD      biSizeImage;
    LONG       biXPelsPerMeter;
    LONG       biYPelsPerMeter;
    DWORD      biClrUsed;
    DWORD      biClrImportant;
} BITMAPINFOHEADER, FAR *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;
typedef struct __attribute__((packed)) tagBITMAPINFO {
    BITMAPINFOHEADER    bmiHeader;
    RGBQUAD             bmiColors[1];
} BITMAPINFO, FAR *LPBITMAPINFO, *PBITMAPINFO;
typedef struct __attribute__((packed)) tagPALETTEENTRY {
    BYTE        peRed;
    BYTE        peGreen;
    BYTE        peBlue;
    BYTE        peFlags;
} PALETTEENTRY, *PPALETTEENTRY, FAR *LPPALETTEENTRY;
typedef struct __attribute__((packed)) tagLOGPALETTE {
    WORD        palVersion;
    WORD        palNumEntries;
    PALETTEENTRY        palPalEntry[1];
} LOGPALETTE, *PLOGPALETTE, NEAR *NPLOGPALETTE, FAR *LPLOGPALETTE;
enum HGDIOBJ_TYPE {
	HGDIOBJ_TYPE_INVALID = 0,
	HGDIOBJ_TYPE_PEN,
	HGDIOBJ_TYPE_BRUSH,
	HGDIOBJ_TYPE_FONT,
	HGDIOBJ_TYPE_BITMAP,
	HGDIOBJ_TYPE_REGION,
	HGDIOBJ_TYPE_PALETTE
};
typedef struct {
	enum HGDIOBJ_TYPE handleType;

	// HGDIOBJ_TYPE_PALETTE
	PLOGPALETTE paletteLog;

	// HGDIOBJ_TYPE_BITMAP
	CONST BITMAPINFO *bitmapInfo;
	CONST BITMAPINFOHEADER * bitmapInfoHeader;
	CONST VOID *bitmapBits;
} _HGDIOBJ;
typedef _HGDIOBJ * HGDIOBJ;
//typedef void * HGDIOBJ;
typedef HGDIOBJ HPALETTE;
typedef HGDIOBJ HBITMAP;
typedef HGDIOBJ HFONT;

extern int GetObject(HANDLE h, int c, LPVOID pv);
extern BOOL DeleteObject(HGDIOBJ ho);

#define OBJ_BITMAP          7

#define WHITE_PEN           6
#define BLACK_PEN           7
extern HGDIOBJ GetStockObject(int i);
extern HPALETTE CreatePalette(CONST LOGPALETTE * plpal);

// DC

enum DC_TYPE {
    DC_TYPE_INVALID = 0,
    DC_TYPE_MEMORY,
    DC_TYPE_DISPLAY
};
enum HDC_TYPE {
    HDC_TYPE_INVALID = 0,
    HDC_TYPE_DC
};
struct _HDC;
typedef struct _HDC * HDC;
struct _HDC{
	enum HDC_TYPE handleType;
	HDC hdcCompatible;
    enum DC_TYPE dcType;
	HBITMAP selectedBitmap;
	HPALETTE selectedPalette;
};
//typedef HANDLE HDC;

extern HDC CreateCompatibleDC(HDC hdc);
extern BOOL DeleteDC(HDC hdc);
extern HGDIOBJ SelectObject(HDC hdc, HGDIOBJ h);
extern HGDIOBJ GetCurrentObject(HDC hdc, UINT type);
typedef struct tagPOINT
{
    LONG  x;
    LONG  y;
} POINT, *PPOINT, NEAR *NPPOINT, FAR *LPPOINT;
extern BOOL MoveToEx(HDC hdc, int x, int y, LPPOINT lppt);
extern BOOL LineTo(HDC hdc, int x, int y);
#define SRCCOPY             (DWORD)0x00CC0020 /* dest = source                   */
#define DSTINVERT           (DWORD)0x00550009 /* dest = (NOT dest)               */
#define BLACKNESS           (DWORD)0x00000042 /* dest = BLACK                    */
extern BOOL PatBlt(HDC hdc, int x, int y, int w, int h, DWORD rop);
extern BOOL BitBlt(HDC hdc, int x, int y, int cx, int cy, HDC hdcSrc, int x1, int y1, DWORD rop);
#define HALFTONE                     4
extern int SetStretchBltMode(HDC hdc, int mode);
extern BOOL StretchBlt(HDC hdcDest, int xDest, int yDest, int wDest, int hDest, HDC hdcSrc, int xSrc, int ySrc, int wSrc, int hSrc, DWORD rop);
extern UINT SetDIBColorTable(HDC  hdc, UINT iStart, UINT cEntries, CONST RGBQUAD *prgbq);
/* constants for CreateDIBitmap */
#define CBM_INIT        0x04L   /* initialize bitmap */
/* DIB color table identifiers */
#define DIB_RGB_COLORS      0 /* color table in RGBs */
#define DIB_PAL_COLORS      1 /* color table in palette indices */
extern HBITMAP CreateDIBitmap( HDC hdc, CONST BITMAPINFOHEADER *pbmih, DWORD flInit, CONST VOID *pjBits, CONST BITMAPINFO *pbmi, UINT iUsage);
extern HBITMAP CreateDIBSection(HDC hdc, CONST BITMAPINFO *pbmi, UINT usage, VOID **ppvBits, HANDLE hSection, DWORD offset);
extern HBITMAP CreateCompatibleBitmap( HDC hdc, int cx, int cy);
typedef struct _RGNDATAHEADER {
    DWORD   dwSize;
    DWORD   iType;
    DWORD   nCount;
    DWORD   nRgnSize;
    RECT    rcBound;
} RGNDATAHEADER, *PRGNDATAHEADER;
typedef struct _RGNDATA {
    RGNDATAHEADER   rdh;
    char            Buffer[1];
} RGNDATA, *PRGNDATA, NEAR *NPRGNDATA, FAR *LPRGNDATA;
extern int GetDIBits(HDC hdc, HBITMAP hbm, UINT start, UINT cLines, LPVOID lpvBits, LPBITMAPINFO lpbmi, UINT usage);
extern HPALETTE SelectPalette(HDC hdc, HPALETTE hPal, BOOL bForceBkgd);
extern UINT RealizePalette(HDC hdc);
/* GetRegionData/ExtCreateRegion */
#define RDH_RECTANGLES  1
extern BOOL SetRect(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom);

struct HRGN__ { int unused; };
typedef struct HRGN__ *HRGN;
typedef struct  tagXFORM
{
    FLOAT   eM11;
    FLOAT   eM12;
    FLOAT   eM21;
    FLOAT   eM22;
    FLOAT   eDx;
    FLOAT   eDy;
} XFORM, *PXFORM, FAR *LPXFORM;
extern int SetWindowRgn(HWND hWnd, HRGN hRgn, BOOL bRedraw);
extern HRGN ExtCreateRegion(CONST XFORM * lpx, DWORD nCount, CONST RGNDATA * lpData);
extern BOOL GdiFlush(void);
typedef struct tagPAINTSTRUCT {
    HDC         hdc;
    BOOL        fErase;
    RECT        rcPaint;
    BOOL        fRestore;
    BOOL        fIncUpdate;
    BYTE        rgbReserved[32];
} PAINTSTRUCT, *PPAINTSTRUCT, *NPPAINTSTRUCT, *LPPAINTSTRUCT;
extern HDC BeginPaint(HWND hWnd, LPPAINTSTRUCT lpPaint);
extern BOOL EndPaint(HWND hWnd, CONST PAINTSTRUCT *lpPaint);


// Window



// disrpl.c
//wvsprintf(cOutput,lpFormat,arglist);

//_tcschr(_T(" \t\n\r"),str->szBuffer[str->dwPos-1]) == NULL)
//_tcschr(lpszStart,_T('\n')

//lstrcmp(lpszObject,ObjDecode[i].lpszName)
//lstrcmp(lpszObject,_T("::"))

//_tcsncmp(lpszObject,_T("DIR\n"),4)
//_tcsncmp(lpszStart,_T("ENDDIR"),lpszEnd-lpszStart)


// file.c
typedef struct tagWINDOWPLACEMENT {
	UINT  length;
	UINT  flags;
	UINT  showCmd;
	POINT ptMinPosition;
	POINT ptMaxPosition;
	RECT  rcNormalPosition;
#ifdef _MAC
	RECT  rcDevice;
#endif
} WINDOWPLACEMENT;
typedef WINDOWPLACEMENT *PWINDOWPLACEMENT, *LPWINDOWPLACEMENT;

extern BOOL DestroyWindow(HWND hWnd);
extern BOOL GetWindowPlacement(HWND hWnd, WINDOWPLACEMENT *lpwndpl);
extern BOOL SetWindowPlacement(HWND hWnd, CONST WINDOWPLACEMENT *lpwndpl);
extern BOOL InvalidateRect(HWND hWnd, CONST RECT *lpRect, BOOL bErase);
#define GWL_STYLE           (-16)
extern BOOL AdjustWindowRect(LPRECT lpRect, DWORD dwStyle, BOOL bMenu);
extern LONG GetWindowLong(HWND hWnd, int nIndex);
#define MF_BYCOMMAND        0x00000000L
#define MF_BYPOSITION       0x00000400L
#define MF_SEPARATOR        0x00000800L
#define MF_ENABLED          0x00000000L
#define MF_GRAYED           0x00000001L
#define MF_DISABLED         0x00000002L
#define MF_UNCHECKED        0x00000000L
#define MF_CHECKED          0x00000008L
#define MF_STRING           0x00000000L
extern HMENU GetMenu(HWND hWnd);
extern int GetMenuItemCount(HMENU hMenu);
extern UINT GetMenuItemID(HMENU hMenu, int nPos);
extern HMENU GetSubMenu(HMENU hMenu, int nPos);
extern int GetMenuString(HMENU hMenu, UINT uIDItem, LPTSTR lpString,int cchMax, UINT flags);
extern BOOL DeleteMenu(HMENU hMenu, UINT uPosition, UINT uFlags);
extern BOOL InsertMenu(HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCTSTR lpNewItem);

#define HWND_TOP        ((HWND)0)
#define HWND_BOTTOM     ((HWND)1)
#define HWND_TOPMOST    ((HWND)-1)
#define HWND_NOTOPMOST  ((HWND)-2)
#define SWP_NOSIZE          0x0001
#define SWP_NOMOVE          0x0002
#define SWP_NOZORDER        0x0004
extern BOOL SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);
extern BOOL IsRectEmpty(CONST RECT *lprc);
extern BOOL WINAPI SetWindowOrgEx(HDC hdc, int x, int y, LPPOINT lppt);

#define _MAX_PATH   260 // max. length of full pathname
#define _MAX_DRIVE  3   // max. length of drive component
#define _MAX_DIR    256 // max. length of path component
#define _MAX_FNAME  256 // max. length of file name component
#define _MAX_EXT    256 // max. length of extension component

typedef wchar_t WCHAR;    // wc,   16-bit UNICODE character
typedef CONST WCHAR *LPCWSTR, *PCWSTR;
typedef WCHAR *NWPSTR, *LPWSTR, *PWSTR;
extern BOOL GetClientRect(HWND hWnd, LPRECT lpRect);

typedef char *PSZ;
typedef DWORD   COLORREF;

extern BOOL MessageBeep(UINT uType);

typedef HANDLE              HGLOBAL;
#define GMEM_MOVEABLE       0x0002
extern HGLOBAL GlobalAlloc(UINT uFlags, SIZE_T dwBytes);
extern LPVOID GlobalLock (HGLOBAL hMem);
extern BOOL GlobalUnlock(HGLOBAL hMem);

#define CF_TEXT             1
extern BOOL OpenClipboard(HWND hWndNewOwner);
extern BOOL CloseClipboard(VOID);
extern BOOL EmptyClipboard(VOID);
extern HANDLE SetClipboardData(UINT uFormat,HANDLE hMem);
extern HGLOBAL GlobalFree(HGLOBAL hMem);
extern BOOL IsClipboardFormatAvailable(UINT format);
extern HANDLE GetClipboardData(UINT uFormat);


extern DWORD GetPrivateProfileString(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpDefault, LPTSTR lpReturnedString, DWORD nSize, LPCTSTR lpFileName);
extern UINT GetPrivateProfileInt(LPCTSTR lpAppName, LPCTSTR lpKeyName, INT nDefault, LPCTSTR lpFileName);
extern BOOL WritePrivateProfileString(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpString, LPCTSTR lpFileName);

/* flags for fuEvent parameter of timeSetEvent() function */
#define TIME_ONESHOT    0x0000   /* program timer for single event */
#define TIME_PERIODIC   0x0001   /* program for continuous periodic event */
typedef UINT        MMRESULT;   /* error return code, 0 means no error */
extern MMRESULT timeSetEvent(UINT uDelay, UINT uResolution, LPTIMECALLBACK fptc, DWORD_PTR dwUser, UINT fuEvent);
extern MMRESULT timeKillEvent(UINT uTimerID);
typedef struct _SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;
/* timer device capabilities data structure */
typedef struct timecaps_tag {
    UINT    wPeriodMin;     /* minimum period supported  */
    UINT    wPeriodMax;     /* maximum period supported  */
} TIMECAPS, *PTIMECAPS, *NPTIMECAPS, *LPTIMECAPS;
extern MMRESULT timeGetDevCaps(LPTIMECAPS ptc, UINT cbtc);
/* timer error return values */
#define TIMERR_NOERROR        (0)                  /* no error */
#define TIMERR_NOCANDO        (TIMERR_BASE+1)      /* request not completed */
#define TIMERR_STRUCT         (TIMERR_BASE+33)     /* time struct size */
extern MMRESULT timeBeginPeriod(UINT uPeriod);
extern MMRESULT timeEndPeriod(UINT uPeriod);
extern VOID GetLocalTime(LPSYSTEMTIME lpSystemTime);
extern WORD GetTickCount(VOID);

extern BOOL EnableWindow(HWND hWnd, BOOL bEnable);
extern HWND GetDlgItem(HWND hDlg, int nIDDlgItem);
extern UINT GetDlgItemTextA(HWND hDlg, int nIDDlgItem, LPSTR lpString,int cchMax);
#define GetDlgItemText  GetDlgItemTextA
extern BOOL SetDlgItemText(HWND hDlg, int nIDDlgItem, LPCSTR lpString);
extern LRESULT SendDlgItemMessage(HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam);
extern BOOL CheckDlgButton(HWND hDlg, int nIDButton, UINT uCheck);
extern UINT IsDlgButtonChecked(HWND hDlg, int nIDButton);
extern BOOL EndDialog(HWND hDlg, INT_PTR nResult);
typedef INT_PTR (CALLBACK* DLGPROC)(HWND, UINT, WPARAM, LPARAM);
//extern INT_PTR DialogBoxParam(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
extern INT_PTR DialogBoxParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
#define DialogBoxA(hInstance, lpTemplate, hWndParent, lpDialogFunc) \
    DialogBoxParamA(hInstance, lpTemplate, hWndParent, lpDialogFunc, 0L)
#define DialogBox  DialogBoxA
#define DialogBoxParam  DialogBoxParamA

#define MAKEINTRESOURCE(i) ((LPSTR)((ULONG_PTR)((WORD)(i))))
typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;
typedef struct _WIN32_FIND_DATAA {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    CHAR   cFileName[ MAX_PATH ];
    CHAR   cAlternateFileName[ 14 ];
#ifdef _MAC
    DWORD dwFileType;
    DWORD dwCreatorType;
    WORD  wFinderFlags;
#endif
} WIN32_FIND_DATAA, *PWIN32_FIND_DATAA, *LPWIN32_FIND_DATAA;
typedef WIN32_FIND_DATAA WIN32_FIND_DATA;
extern HANDLE  FindFirstFileA(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData);
#define FindFirstFile  FindFirstFileA
extern BOOL FindNextFileA(HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData);
#define FindNextFile  FindNextFileA
extern BOOL FindClose(HANDLE hFindFile);
typedef struct _SHITEMID
{
    USHORT cb;
    BYTE abID[ 1 ];
} 	SHITEMID;
typedef struct _ITEMIDLIST
{
    SHITEMID mkid;
} 	ITEMIDLIST;
typedef ITEMIDLIST *LPITEMIDLIST;
typedef const ITEMIDLIST *LPCITEMIDLIST;
#define PCIDLIST_ABSOLUTE LPCITEMIDLIST
extern BOOL SHGetPathFromIDListA(PCIDLIST_ABSOLUTE pidl, LPSTR pszPath);
#define SHGetPathFromIDList  SHGetPathFromIDListA
typedef int (CALLBACK* BFFCALLBACK)(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
// Browsing for directory.
#define BIF_RETURNONLYFSDIRS    0x00000001
#define BIF_DONTGOBELOWDOMAIN   0x00000002
#define BIF_STATUSTEXT          0x00000004
typedef struct _browseinfoA {
    HWND        hwndOwner;
    PCIDLIST_ABSOLUTE pidlRoot;
    LPSTR        pszDisplayName;
    LPCSTR       lpszTitle;
    UINT         ulFlags;
    BFFCALLBACK  lpfn;
    LPARAM       lParam;
    int          iImage;
} BROWSEINFOA, *PBROWSEINFOA, *LPBROWSEINFOA;
#define BROWSEINFO      BROWSEINFOA
typedef struct IMallocVtbl
{
    ULONG (*Release )(void * This);
    void (*Free )(void * This, void *pv);
} IMallocVtbl;
typedef struct {
    struct IMallocVtbl *lpVtbl;
} IMalloc;
typedef IMalloc *LPMALLOC;
typedef long HRESULT;
HRESULT SHGetMalloc(IMalloc **ppMalloc);   // CoGetMalloc(MEMCTX_TASK,ppMalloc)
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define FAILED(hr) (((HRESULT)(hr)) < 0)
#define PIDLIST_ABSOLUTE         LPITEMIDLIST
extern PIDLIST_ABSOLUTE SHBrowseForFolderA(LPBROWSEINFOA lpbi);
#define SHBrowseForFolder   SHBrowseForFolderA
#define LANG_NEUTRAL 0x00
#define SUBLANG_NEUTRAL 0x00
#define PRIMARYLANGID(lgid)    ((WORD  )(lgid) & 0x3ff)
#define SUBLANGID(lgid)        ((WORD  )(lgid) >> 10)

#define RGB(r,g,b)          ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))
extern HCURSOR SetCursor(HCURSOR hCursor);
extern int MulDiv(int nNumber, int nNumerator, int nDenominator);

#define KL_NAMELENGTH 9
extern BOOL GetKeyboardLayoutName(LPSTR pwszKLID);

extern void DragAcceptFiles(HWND hWnd, BOOL fAccept);

struct HDROP__{int unused;}; typedef struct HDROP__ *HDROP;

typedef struct tagCOPYDATASTRUCT {
    ULONG_PTR dwData;
    DWORD cbData;
    PVOID lpData;
} COPYDATASTRUCT, *PCOPYDATASTRUCT;

#ifdef UNICODE

extern int wvsprintf(LPWSTR, LPCWSTR, va_list arglist);
extern DWORD GetFullPathName(LPCWSTR lpFileName, DWORD nBufferLength, LPWSTR lpBuffer, LPWSTR* lpFilePart);
extern LPWSTR lstrcpyn(LPWSTR lpString1, LPCWSTR lpString2,int iMaxLength);
extern LPWSTR lstrcat(LPWSTR lpString1, LPCWSTR lpString2);

extern void __cdecl _wsplitpath(wchar_t const* _FullPath, wchar_t* _Drive, wchar_t* _Dir, wchar_t* _Filename, wchar_t* _Ext);
#define _tsplitpath     _wsplitpath
#define _tcschr         wcschr
extern void _wmakepath(wchar_t _Buffer, wchar_t const* _Drive, wchar_t const* _Dir, wchar_t const* _Filename, wchar_t const* _Ext);
#define _tmakepath      _wmakepath
extern int lstrcmp(LPCWSTR lpString1, LPCWSTR lpString2);
#define _tcsncmp        wcsncmp

extern int lstrcmpi(LPCWSTR lpString1, LPCWSTR lpString2);
#define _tcstoul    wcstoul
#define _tcscmp     wcscmp
#define _tcslen     wcslen
#define _tcscpy     wcscpy
#define _tcscat     wcscat
#define _tcsstr     wcsstr

#else

extern int wvsprintf(LPSTR, LPCSTR, va_list arglist);
extern DWORD GetFullPathName(LPCSTR lpFileName, DWORD nBufferLength, LPSTR lpBuffer, LPSTR* lpFilePart);
extern LPSTR lstrcpyn(LPSTR lpString1, LPCSTR lpString2,int iMaxLength);
extern LPSTR lstrcat(LPSTR lpString1, LPCSTR lpString2);

extern void __cdecl _splitpath(char const* _FullPath, char* _Drive, char* _Dir, char* _Filename, char* _Ext);
#define _tsplitpath     _splitpath
#define _tcschr         strchr
extern void _makepath(char _Buffer, char const* _Drive, char const* _Dir, char const* _Filename, char const* _Ext);
#define _tmakepath      _makepath
extern int lstrcmp(LPCSTR lpString1, LPCSTR lpString2);
#define _tcsncmp        strncmp
extern int lstrcmpi(LPCSTR lpString1, LPCSTR lpString2);
#define _tcstoul    strtoul
#define _tcscmp     strcmp
#define _tcslen     strlen
#define _tcscpy     strcpy
#define _tcscat     strcat
#define _tcsstr     strstr


#endif // !UNICODE



#define __max(a,b) (((a) > (b)) ? (a) : (b))
#define __min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

/* device ID for wave device mapper */
#define WAVE_MAPPER     ((UINT)-1)
