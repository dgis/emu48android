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

#pragma once

#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>
#include <wchar.h>
#include <stdio.h>
#include <string.h>
#include <jni.h>
#include <sys/select.h>
#include <android/bitmap.h>
#include <android/asset_manager.h>
#include <android/log.h>
#define LOG_TAG "NDK_NativeEmuXX"
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))


#if defined DEBUG_ANDROID_TIMER
#   define TIMER_LOGD(...) LOGD(__VA_ARGS__)
#else
#   define TIMER_LOGD(...)
#endif

#if defined DEBUG_ANDROID_WAVE_OUT
#   define WAVE_OUT_LOGD(...) LOGD(__VA_ARGS__)
#else
#   define WAVE_OUT_LOGD(...)
#endif

#if defined DEBUG_ANDROID_PAINT
#   define PAINT_LOGD(...) LOGD(__VA_ARGS__)
#else
#   define PAINT_LOGD(...)
#endif

#if defined DEBUG_ANDROID_THREAD
#   define THREAD_LOGD(...) LOGD(__VA_ARGS__)
#else
#   define THREAD_LOGD(...)
#endif

#if defined DEBUG_ANDROID_FILE
#   define FILE_LOGD(...) LOGD(__VA_ARGS__)
#else
#   define FILE_LOGD(...)
#endif

#if defined DEBUG_ANDROID_SERIAL
#   define SERIAL_LOGD(...) LOGD(__VA_ARGS__)
#else
#   define SERIAL_LOGD(...)
#endif

#define _MSC_VER 1914
#define __unaligned
#define GetWindowLongPtr	GetWindowLong


#define TRUE    1
#define FALSE   0
#define MAX_PATH PATH_MAX
#define INFINITE    0xFFFFFFFF //0
#define FAR
#define NEAR

#define __forceinline __attribute__((always_inline))


#undef abs
extern int abs(int i);


typedef int BOOL;
typedef unsigned long ULONG;
typedef short SHORT;
typedef unsigned short USHORT;
typedef unsigned char UCHAR;
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
typedef UINT * LPUINT;
typedef int32_t INT;
typedef char CHAR;
typedef void VOID;
typedef void *LPVOID;
typedef void *PVOID;
typedef int32_t LONG;
typedef LONG *PLONG;

#if INTPTR_MAX == INT32_MAX
    // 32 bits environment
    typedef int INT_PTR, *PINT_PTR;
    typedef unsigned int UINT_PTR, *PUINT_PTR;
    typedef int LONG_PTR, *PLONG_PTR;
    typedef unsigned long ULONG_PTR, *PULONG_PTR;
#elif INTPTR_MAX == INT64_MAX
    // 64 bits environment
    typedef int64_t INT_PTR, *PINT_PTR;
    typedef uint64_t UINT_PTR, *PUINT_PTR;
    typedef int64_t LONG_PTR, *PLONG_PTR;
    typedef uint64_t ULONG_PTR, *PULONG_PTR;
#endif

typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;
//typedef size_t SIZE_T;
typedef ULONG_PTR SIZE_T, *PSIZE_T;


#define MAXLONG     0x7fffffff
#define MAXDWORD    0xffffffff


#define CONST const

#define _ASSERT(expr)
//#define _ASSERT(expr) \
//	(void)(                                                                                     \
//		(!!(expr)) ||                                                                           \
//		(1 != _CrtDbgReportW(_CRT_ASSERT, _CRT_WIDE(__FILE__), __LINE__, NULL, L"%ls", NULL)) || \
//		(_CrtDbgBreak(), 0)                                                                     \
//	)

#define MK_LBUTTON          0x0001


typedef void *TIMECALLBACK ;
#define CALLBACK
typedef void(*LPTIMECALLBACK)(UINT uEventId, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2) ;

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
typedef const char *LPCSTR;
typedef const char *LPCTSTR;
#ifdef UNICODE
typedef wchar_t TCHAR;
typedef wchar_t _TUCHAR;
#else
typedef char TCHAR;
typedef unsigned char _TUCHAR;
#endif // !UNICODE


typedef pthread_mutex_t CRITICAL_SECTION;
typedef void WNDCLASS;

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
#define WM_QUIT                         0x0012
#define WM_INITDIALOG                   0x0110
#define WM_COMMAND                      0x0111
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
#define INVALID_FILE_SIZE ((DWORD)0xFFFFFFFF)
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#define NO_ERROR 0L                 // dderror
#define ERROR_IO_PENDING 997L		// dderror
extern DWORD GetLastError(VOID);

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
#define MAKEWORD(a, b)      ((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)(((DWORD_PTR)(a)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(b)) & 0xffff))) << 16))

#define HIBYTE(i)   ((i)>>8)
#define LOBYTE(i)   (i)
#define _istdigit(c)    ((c)>='0' && (c)<='9')
#define lstrlen(s)      strlen(s)
#define lstrcpy(d,s)    strcpy(d,s)
#define _tcsspn(s,k)    strspn(s,k)
#define wsprintf(s,f,...)   sprintf(s,f, ## __VA_ARGS__)
#define sprintf_s(s,l,f,...) sprintf((const char *)s,f, ## __VA_ARGS__)
#define HeapAlloc(h,v,s)    malloc(s)
#define HeapFree(h,v,p)     do{free(p);(p)=v;}while(0)
#define ZeroMemory(p,s)     memset(p,0,s)
#define FillMemory(p,n,v)   memset(p,v,n*sizeof(*(p)))
#define CopyMemory(d,src,s) memcpy(d,src,s)


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
typedef DWORD   COLORREF;
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
	/*CONST*/ BITMAPINFOHEADER * bitmapInfoHeader;
	CONST VOID *bitmapBits;

	// HGDIOBJ_TYPE_BRUSH
	COLORREF brushColor;
} _HGDIOBJ;
typedef _HGDIOBJ * HGDIOBJ;
//typedef void * HGDIOBJ;
typedef HGDIOBJ HPALETTE;
typedef HGDIOBJ HBITMAP;
typedef HGDIOBJ HFONT;
typedef HGDIOBJ HBRUSH;

extern int GetObject(HGDIOBJ h, int c, LPVOID pv);
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
struct _HDC {
	enum HDC_TYPE handleType;
	HDC hdcCompatible;
	enum DC_TYPE dcType;
	HBITMAP selectedBitmap;
	HPALETTE selectedPalette;
	HPALETTE realizedPalette;
	HBRUSH selectedBrushColor;
	BOOL isBackgroundColorSet;
	COLORREF backgroundColor;
	int windowOriginX;
	int windowOriginY;
};

// Comm

typedef struct _DCB {
	DWORD DCBlength;
	DWORD BaudRate;
	DWORD fBinary: 1;
	DWORD fParity: 1;
	DWORD fOutxCtsFlow:1;
	DWORD fOutxDsrFlow:1;
	DWORD fDtrControl:2;
	DWORD fDsrSensitivity:1;
	DWORD fTXContinueOnXoff: 1;
	DWORD fOutX: 1;
	DWORD fInX: 1;
	DWORD fErrorChar: 1;
	DWORD fNull: 1;
	DWORD fRtsControl:2;
	DWORD fAbortOnError:1;
	DWORD fDummy2:17;
	WORD wReserved;
	WORD XonLim;
	WORD XoffLim;
	BYTE ByteSize;
	BYTE Parity;
	BYTE StopBits;
	char XonChar;
	char XoffChar;
	char ErrorChar;
	char EofChar;
	char EvtChar;
	WORD wReserved1;
} DCB, *LPDCB;

// Handle

struct _HANDLE;
typedef struct _HANDLE * HWND;

typedef struct tagPOINT
{
    LONG  x;
    LONG  y;
} POINT, *PPOINT, NEAR *NPPOINT, FAR *LPPOINT;

typedef struct tagMSG {
    HWND        hwnd;
    UINT        message;
    WPARAM      wParam;
    LPARAM      lParam;
    DWORD       time;
    POINT       pt;
#ifdef _MAC
    DWORD       lPrivate;
#endif
} MSG, *PMSG, NEAR *NPMSG, FAR *LPMSG;


enum HANDLE_TYPE {
    HANDLE_TYPE_INVALID = 0,
    HANDLE_TYPE_FILE,
    HANDLE_TYPE_FILE_ASSET,
    HANDLE_TYPE_FILE_MAPPING,
	HANDLE_TYPE_FILE_MAPPING_CONTENT,
	HANDLE_TYPE_FILE_MAPPING_ASSET,
    HANDLE_TYPE_EVENT,
	HANDLE_TYPE_THREAD,
	HANDLE_TYPE_WINDOW,
	HANDLE_TYPE_ICON,
	HANDLE_TYPE_COM,
};
struct _HANDLE {
    enum HANDLE_TYPE handleType;
	union {
		struct {
    // HANDLE_TYPE_FILE*
    int fileDescriptor;
    BOOL fileOpenFileFromContentResolver;

    AAsset* fileAsset;

    // HANDLE_TYPE_FILE_MAPPING*
    off_t fileMappingOffset;
    size_t fileMappingSize;
    void* fileMappingAddress;
	DWORD fileMappingProtect;
		};

		struct {
	// HANDLE_TYPE_THREAD
    pthread_t threadId;
    DWORD (*threadStartAddress)(LPVOID);
    LPVOID threadParameter;
    struct _HANDLE * threadEventMessage;
    struct tagMSG threadMessage;
	int threadIndex;
		};

		struct {
	// HANDLE_TYPE_EVENT
    pthread_cond_t eventCVariable;
    pthread_mutex_t eventMutex;
    BOOL eventAutoReset;
    BOOL eventState;
		};

	    struct {
    // HANDLE_TYPE_WINDOW
    HDC windowDC;
	    };

	    struct {
    // HANDLE_TYPE_ICON
    HBITMAP icon;
};

	    struct {
		    // HANDLE_TYPE_COM
		    LPDCB commState;
		    struct _HANDLE * commEvent;
		    int commIndex;
		    int commId;
		    int commEventMask;
	    };
    };
};
typedef struct _HANDLE * HANDLE;

typedef HANDLE HMENU;
typedef HANDLE HINSTANCE;
typedef HANDLE HCURSOR;
typedef HANDLE HLOCAL; //TODO


typedef struct _OVERLAPPED {
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

// This is not a Win32 function
extern BOOL SaveMapViewToFile(LPCVOID lpBaseAddress);

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
#define LB_ADDSTRING            0x0180
#define LB_GETCOUNT             0x018B
#define LB_GETSEL               0x0187
#define LB_GETSELCOUNT          0x0190
#define LB_GETITEMDATA          0x0199
#define LB_SETITEMDATA          0x019A

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
#define THREAD_BASE_PRIORITY_MAX    2
#define THREAD_PRIORITY_HIGHEST         THREAD_BASE_PRIORITY_MAX
#define THREAD_PRIORITY_ABOVE_NORMAL    (THREAD_PRIORITY_HIGHEST-1)
extern BOOL SetThreadPriority(HANDLE hThread, int nPriority);

extern void InitializeCriticalSection(CRITICAL_SECTION * lpCriticalSection);
extern void EnterCriticalSection(CRITICAL_SECTION *);
extern void LeaveCriticalSection(CRITICAL_SECTION *);
extern void DeleteCriticalSection(CRITICAL_SECTION * lpCriticalSection);

// DC

extern HDC CreateCompatibleDC(HDC hdc);
extern HDC GetDC(HWND hWnd);
extern int ReleaseDC(HWND hWnd, HDC hDC);
extern BOOL DeleteDC(HDC hdc);
extern HGDIOBJ SelectObject(HDC hdc, HGDIOBJ h);
extern HGDIOBJ GetCurrentObject(HDC hdc, UINT type);

HBRUSH  CreateSolidBrush(COLORREF color);

extern BOOL MoveToEx(HDC hdc, int x, int y, LPPOINT lppt);
extern BOOL LineTo(HDC hdc, int x, int y);
#define SRCCOPY             (DWORD)0x00CC0020 /* dest = source                   */
#define SRCAND              (DWORD)0x008800C6 /* dest = source AND dest          */
#define PATCOPY             (DWORD)0x00F00021 /* dest = pattern                  */
#define DSTINVERT           (DWORD)0x00550009 /* dest = (NOT dest)               */
#define BLACKNESS           (DWORD)0x00000042 /* dest = BLACK                    */
#define WHITENESS           (DWORD)0x00FF0062 /* dest = WHITE                    */
extern BOOL PatBlt(HDC hdc, int x, int y, int w, int h, DWORD rop);
extern BOOL BitBlt(HDC hdc, int x, int y, int cx, int cy, HDC hdcSrc, int x1, int y1, DWORD rop);
#define HALFTONE                     4
extern int SetStretchBltMode(HDC hdc, int mode);
extern BOOL StretchBlt(HDC hdcDest, int xDest, int yDest, int wDest, int hDest, HDC hdcSrc, int xSrc, int ySrc, int wSrc, int hSrc, DWORD rop);
extern void StretchBltInternal(int xDest, int yDest, int wDest, int hDest, const void *pixelsDestination,
                               int destinationBitCount, int destinationStride, int destinationWidth,
                               int destinationHeight, int xSrc, int ySrc, int hSrc, int wSrc,
                               const void *pixelsSource, UINT sourceBitCount, int sourceStride,
                               int sourceWidth, int sourceHeight, DWORD rop, BOOL sourceTopDown, BOOL destinationTopDown,
                               const PALETTEENTRY *palPalEntry, COLORREF brushColor,
                               COLORREF backgroundColor);
extern UINT SetDIBColorTable(HDC  hdc, UINT iStart, UINT cEntries, CONST RGBQUAD *prgbq);
/* constants for CreateDIBitmap */
#define CBM_INIT        0x04L   /* initialize bitmap */
/* DIB color table identifiers */
#define DIB_RGB_COLORS      0 /* color table in RGBs */
#define DIB_PAL_COLORS      1 /* color table in palette indices */
extern HBITMAP CreateBitmap( int nWidth, int nHeight, UINT nPlanes, UINT nBitCount, CONST VOID *lpBits);
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
extern int SetDIBits(HDC hdc, HBITMAP hbm, UINT start, UINT cLines, const VOID *lpBits, const BITMAPINFO *lpbmi, UINT ColorUse);
extern COLORREF GetPixel(HDC hdc, int x ,int y);
extern HPALETTE SelectPalette(HDC hdc, HPALETTE hPal, BOOL bForceBkgd);
extern UINT RealizePalette(HDC hdc);
extern COLORREF SetBkColor(HDC hdc, COLORREF color);
/* GetRegionData/ExtCreateRegion */
#define RDH_RECTANGLES  1
extern BOOL SetRect(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom);
extern BOOL SetRectEmpty(LPRECT lprc);
extern BOOL IsRectEmpty(CONST RECT *lprc);
extern BOOL UnionRect(LPRECT dest, CONST RECT *src1, CONST RECT *src2);

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


// Sound / Wave API

typedef UINT        MMRESULT;   /* error return code, 0 means no error */
#define MMSYSERR_NOERROR      0                    /* no error */
#define MMSYSERR_ERROR        1  /* unspecified error */
#define MM_WOM_DONE         0x3BD

/* wave data block header */
typedef struct wavehdr_tag {
	LPSTR       lpData;                 /* pointer to locked data buffer */
	DWORD       dwBufferLength;         /* length of data buffer */
	DWORD       dwBytesRecorded;        /* used for input only */
	DWORD_PTR   dwUser;                 /* for client's use */
	DWORD       dwFlags;                /* assorted flags (see defines) */
	DWORD       dwLoops;                /* loop control counter */
	struct wavehdr_tag FAR *lpNext;     /* reserved for driver */
	DWORD_PTR   reserved;               /* reserved for driver */
} WAVEHDR, *PWAVEHDR, NEAR *NPWAVEHDR, FAR *LPWAVEHDR;

typedef UINT        MMVERSION;  /* major (high byte), minor (low byte) */
#define MAXPNAMELEN      32     /* max product name length (including NULL) */
typedef struct tagWAVEOUTCAPS {
	WORD    wMid;                  /* manufacturer ID */
	WORD    wPid;                  /* product ID */
	MMVERSION vDriverVersion;      /* version of the driver */
	TCHAR    szPname[MAXPNAMELEN];  /* product name (NULL terminated string) */
	DWORD   dwFormats;             /* formats supported */
	WORD    wChannels;             /* number of sources supported */
	WORD    wReserved1;            /* packing */
	DWORD   dwSupport;             /* functionality supported by driver */
} WAVEOUTCAPS, *PWAVEOUTCAPS, *NPWAVEOUTCAPS, *LPWAVEOUTCAPS;

#define WAVE_FORMAT_4M08       0x00000100       /* 44.1   kHz, Mono,   8-bit  */
/*
 *  extended waveform format structure used for all non-PCM formats. this
 *  structure is common to all non-PCM formats.
 */
typedef struct tWAVEFORMATEX
{
	WORD        wFormatTag;         /* format type */
	WORD        nChannels;          /* number of channels (i.e. mono, stereo...) */
	DWORD       nSamplesPerSec;     /* sample rate */
	DWORD       nAvgBytesPerSec;    /* for buffer estimation */
	WORD        nBlockAlign;        /* block size of data */
	WORD        wBitsPerSample;     /* number of bits per sample of mono data */
	WORD        cbSize;             /* the count in bytes of the size of */
	/* extra information (after cbSize) */
} WAVEFORMATEX, *PWAVEFORMATEX, NEAR *NPWAVEFORMATEX, FAR *LPWAVEFORMATEX;
typedef const WAVEFORMATEX FAR *LPCWAVEFORMATEX;
#define WAVE_FORMAT_PCM     1
#define CALLBACK_TASK       0x00020000l    /* dwCallback is a HTASK */
#define CALLBACK_THREAD     (CALLBACK_TASK)/* thread ID replaces 16 bit task */


#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

struct _HWAVEOUT;
typedef struct _HWAVEOUT * HWAVEOUT;
typedef HWAVEOUT * LPHWAVEOUT;
struct _HWAVEOUT{
    WAVEFORMATEX * pwfx;
    UINT uDeviceID;

    // engine interfaces
    SLObjectItf engineObject;
    SLEngineItf engineEngine;

// output mix interfaces
    SLObjectItf outputMixObject;

// buffer queue player interfaces
    SLObjectItf bqPlayerObject;
    SLPlayItf bqPlayerPlay;
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
    SLVolumeItf bqPlayerVolume;

    pthread_mutex_t  audioEngineLock;

    sem_t waveOutLock;
#if defined(NEW_WIN32_SOUND_ENGINE)
    // Linked list of buffers to read (take advantage of the field lpNext!)
	LPWAVEHDR pWaveHeaderNextToRead;
	// Last element of the linked list of buffers to read.
	// The next buffer can be wrote easily.
	LPWAVEHDR pWaveHeaderNextToWrite;

	// Mutex to lock the sound
	//pthread_mutex_t waveOutLock;
#else
	LPWAVEHDR pWaveHeaderNext;
    timer_t playerDoneTimer;
    struct itimerspec playerDoneTimerSetTimes;
#endif
};

extern MMRESULT waveOutPrepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh);
extern MMRESULT waveOutUnprepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh);
extern MMRESULT waveOutWrite(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh);
extern MMRESULT waveOutGetDevCaps(UINT_PTR uDeviceID, LPWAVEOUTCAPS pwoc, UINT cbwoc);
extern MMRESULT waveOutGetID(HWAVEOUT hwo, LPUINT puDeviceID);
extern MMRESULT waveOutOpen(LPHWAVEOUT phwo, UINT uDeviceID, LPCWAVEFORMATEX pwfx, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen);
extern MMRESULT waveOutReset(HWAVEOUT hwo);
extern MMRESULT waveOutClose(HWAVEOUT hwo);


// Window

extern BOOL GetMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
extern BOOL PostThreadMessage(DWORD idThread, UINT Msg, WPARAM wParam, LPARAM lParam);

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

extern HWND CreateWindow();
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
#define RGB(r,g,b)          ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))
#define CLR_INVALID     0xFFFFFFFF

extern BOOL MessageBeep(UINT uType);

typedef HANDLE              HGLOBAL;
#define GMEM_MOVEABLE       0x0002
extern HGLOBAL GlobalAlloc(UINT uFlags, SIZE_T dwBytes);
extern LPVOID GlobalLock (HGLOBAL hMem);
extern BOOL GlobalUnlock(HGLOBAL hMem);
extern HGLOBAL GlobalFree(HGLOBAL hMem);

#define CF_TEXT             1
extern BOOL OpenClipboard(HWND hWndNewOwner);
extern BOOL CloseClipboard(VOID);
extern BOOL EmptyClipboard(VOID);
extern HANDLE SetClipboardData(UINT uFormat,HANDLE hMem);
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
extern ULONGLONG GetTickCount64(VOID);
extern DWORD GetTickCount(VOID);

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
extern INT_PTR DialogBoxParam(HINSTANCE hInstance, LPCTSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
#define DialogBox(hInstance, lpTemplate, hWndParent, lpDialogFunc) \
    DialogBoxParam(hInstance, lpTemplate, hWndParent, lpDialogFunc, 0L)

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

// Locale info
#define LOCALE_USER_DEFAULT            1024
#define LOCALE_SDECIMAL               0x0000000E   // decimal separator, eg "." for 1,234.00
typedef DWORD LCID;
typedef DWORD LCTYPE;
extern int GetLocaleInfo(LCID Locale, LCTYPE LCType, LPSTR lpLCData, int cchData);


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
#define _tcsncpy    wcsncpy
#define _tcscat     wcscat
#define _tcsstr     wcsstr
#define _tcsrchr    wcsrchr


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
#define _tcsncpy    strncpy
#define _tcscat     strcat
#define _tcsstr     strstr
#define _tcsrchr    strrchr


#endif // !UNICODE



#define __max(a,b) (((a) > (b)) ? (a) : (b))
#define __min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

/* device ID for wave device mapper */
#define WAVE_MAPPER     ((UINT)-1)





extern void mainViewUpdateCallback();
extern void mainViewResizeCallback(int x, int y);
extern int openFileFromContentResolver(const TCHAR * fileURL, int writeAccess);
extern int openFileInFolderFromContentResolver(const TCHAR * filename, const TCHAR * folderURL, int writeAccess);
extern int closeFileFromContentResolver(int fd);
extern int openSerialPort(const TCHAR * serialPort);
extern int closeSerialPort(int serialPortId);
extern int setSerialPortParameters(int serialPortId, int baudRate);
extern int readSerialPort(int serialPortId, LPBYTE buffer, int nNumberOfBytesToRead);
extern int writeSerialPort(int serialPortId, LPBYTE buffer, int bufferSize);
extern int serialPortPurgeComm(int serialPortId, int dwFlags);
extern int serialPortSetBreak(int serialPortId);
extern int serialPortClearBreak(int serialPortId);
extern int showAlert(const TCHAR * messageText, int flags);
extern void sendMenuItemCommand(int menuItem);
extern TCHAR szCurrentKml[MAX_PATH];
extern TCHAR szChosenCurrentKml[MAX_PATH];
extern LPBYTE pbyRomBackup;
enum ChooseKmlMode {
    ChooseKmlMode_UNKNOWN,
    ChooseKmlMode_FILE_NEW,
    ChooseKmlMode_FILE_OPEN,
	ChooseKmlMode_FILE_OPEN_WITH_FOLDER,
	ChooseKmlMode_CHANGE_KML //TODO To remove
};
extern enum ChooseKmlMode chooseCurrentKmlMode;
enum DialogBoxMode {
	DialogBoxMode_UNKNOWN,
	DialogBoxMode_GET_USRPRG32,
	DialogBoxMode_SET_USRPRG32,
	DialogBoxMode_GET_USRPRG42,
	DialogBoxMode_SET_USRPRG42,
	DialogBoxMode_OPEN_MACRO,
	DialogBoxMode_SAVE_MACRO
};
extern enum DialogBoxMode currentDialogBoxMode;
extern BOOL securityExceptionOccured;
extern BOOL kmlFileNotFound;
#define MAX_LABEL_SIZE 5000
extern TCHAR labels[MAX_LABEL_SIZE];
#define MAX_ITEMDATA 100
extern int selItemDataIndex[MAX_ITEMDATA];
extern int selItemDataCount;
extern TCHAR getSaveObjectFilenameResult[MAX_PATH];
BOOL getFirstKMLFilenameForType(BYTE chipsetType);
void clipboardCopyText(const TCHAR * text);
const TCHAR * clipboardPasteText();
void performHapticFeedback();
void sendByteUdp(unsigned char byteSent);
void setKMLIcon(int imageWidth, int imageHeight, LPBYTE buffer, int bufferSize);

// IO

#define EV_RXCHAR           0x0001
#define EV_TXEMPTY          0x0004
#define EV_ERR              0x0080

#define CE_OVERRUN          0x0002
#define CE_FRAME            0x0008
#define CE_BREAK            0x0010

#define PURGE_TXABORT       0x0001
#define PURGE_TXCLEAR       0x0004

#define DTR_CONTROL_DISABLE    0x00
#define RTS_CONTROL_DISABLE    0x00

#define NOPARITY            0
#define TWOSTOPBITS         2

typedef struct _COMSTAT {
	DWORD fCtsHold : 1;
	DWORD fDsrHold : 1;
	DWORD fRlsdHold : 1;
	DWORD fXoffHold : 1;
	DWORD fXoffSent : 1;
	DWORD fEof : 1;
	DWORD fTxim : 1;
	DWORD fReserved : 25;
	DWORD cbInQue;
	DWORD cbOutQue;
} COMSTAT, *LPCOMSTAT;

typedef struct _COMMTIMEOUTS {
	DWORD ReadIntervalTimeout;
	DWORD ReadTotalTimeoutMultiplier;
	DWORD ReadTotalTimeoutConstant;
	DWORD WriteTotalTimeoutMultiplier;
	DWORD WriteTotalTimeoutConstant;
} COMMTIMEOUTS,*LPCOMMTIMEOUTS;

extern BOOL GetOverlappedResult(HANDLE hFile, LPOVERLAPPED lpOverlapped, LPDWORD lpNumberOfBytesTransferred, BOOL bWait);
extern void commEvent(int commId, int eventMask);
extern BOOL WaitCommEvent(HANDLE hFile, LPDWORD lpEvtMask, LPOVERLAPPED lpOverlapped);
extern BOOL ClearCommError(HANDLE hFile, LPDWORD lpErrors, LPCOMSTAT lpStat);
extern BOOL SetCommTimeouts(HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts);
extern BOOL SetCommMask(HANDLE hFile, DWORD dwEvtMask);
extern BOOL SetCommState(HANDLE hFile, LPDCB lpDCB);
extern BOOL PurgeComm(HANDLE hFile, DWORD dwFlags);
extern BOOL SetCommBreak(HANDLE hFile);
extern BOOL ClearCommBreak(HANDLE hFile);
extern BOOL serialPortSlowDown;

// TCP

typedef int SOCKET;
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)

#define WSAEINTR 10004L
#define WSAEWOULDBLOCK 10035L
extern int WSAGetLastError();

typedef struct WSAData {
    WORD                    wVersion;
    WORD                    wHighVersion;
    //...
} WSADATA, * LPWSADATA;


extern int WSAStartup(WORD wVersionRequested, LPWSADATA lpWSAData);
extern int WSACleanup();

typedef struct addrinfo ADDRINFO;

extern int closesocket(SOCKET s);

/* DDE */

#define DDE_FACK                0x8000
#define DDE_FNOTPROCESSED       0x0000

#define     XTYPF_NOBLOCK            0x0002

#define     XCLASS_BOOL              0x1000
#define     XCLASS_DATA              0x2000
#define     XCLASS_FLAGS             0x4000

#define     XTYP_CONNECT            (0x0060 | XCLASS_BOOL | XTYPF_NOBLOCK)
#define     XTYP_POKE               (0x0090 | XCLASS_FLAGS         )
#define     XTYP_REQUEST            (0x00B0 | XCLASS_DATA          )

DWORD DdeQueryString(DWORD idInst, HSZ hsz, LPSTR psz, DWORD cchMax, int iCodePage);
LPBYTE DdeAccessData(HDDEDATA hData, LPDWORD pcbDataSize);
BOOL DdeUnaccessData(HDDEDATA hData);
HDDEDATA DdeCreateDataHandle(DWORD idInst, LPBYTE pSrc, DWORD cb, DWORD cbOff, HSZ hszItem, UINT wFmt, UINT afCmd);

typedef HDDEDATA (CALLBACK *PFNCALLBACK)(UINT wType, UINT wFmt, HCONV hConv,
                                         HSZ hsz1, HSZ hsz2, HDDEDATA hData, ULONG_PTR dwData1, ULONG_PTR dwData2);
#define     APPCLASS_STANDARD            0x00000000L

#define     CBF_FAIL_ADVISES             0x00004000
#define     CBF_FAIL_EXECUTES            0x00008000

#define     CBF_SKIP_REGISTRATIONS       0x00080000
#define     CBF_SKIP_UNREGISTRATIONS     0x00100000

#define DNS_REGISTER        0x0001
#define DNS_UNREGISTER      0x0002

UINT DdeInitialize(LPDWORD pidInst, PFNCALLBACK pfnCallback, DWORD afCmd, DWORD ulRes);
UINT RegisterClipboardFormat(LPCSTR lpszFormat);
HSZ DdeCreateStringHandle(DWORD idInst, LPCSTR psz, int iCodePage);
HDDEDATA DdeNameService(DWORD idInst, HSZ hsz1, HSZ hsz2, UINT afCmd);
