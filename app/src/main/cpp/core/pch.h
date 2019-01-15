//
//	PCH.H
//

#define _WIN32_IE 0x0200
#define _CRT_SECURE_NO_DEPRECATE
#define _CRTDBG_MAP_ALLOC
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <windows.h>
#include <tchar.h>
#include <shellapi.h>
#include <commctrl.h>
#include <shlobj.h>
#include <stdlib.h>
#include <malloc.h>
#include <stddef.h>
#include <ctype.h>
#include <stdio.h>
#include <direct.h>
#include <conio.h>
#include <crtdbg.h>

#if !defined VERIFY
#if defined _DEBUG
#define VERIFY(f)			_ASSERT(f)
#else // _DEBUG
#define VERIFY(f)			((VOID)(f))
#endif // _DEBUG
#endif // _VERIFY

#if !defined INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#endif

#if !defined INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif

#if !defined GWLP_USERDATA
#define GWLP_USERDATA       GWL_USERDATA
#endif

#if !defined GCLP_HCURSOR
#define GCLP_HCURSOR        GCL_HCURSOR
#endif

#if !defined IDC_HAND // Win2k specific definition
#define IDC_HAND            MAKEINTRESOURCE(32649)
#endif

#if _MSC_VER <= 1200 // missing type definition in the MSVC6.0 SDK and earlier
#define SetWindowLongPtr	SetWindowLong
#define GetWindowLongPtr	GetWindowLong
#define SetClassLongPtr		SetClassLong
#define GetClassLongPtr		GetClassLong
typedef SIZE_T DWORD_PTR, *PDWORD_PTR;
typedef ULONG  ULONG_PTR, *PULONG_PTR;
typedef LONG   LONG_PTR,  *PLONG_PTR;
#endif

#if _MSC_VER >= 1400 // valid for VS2005 and later
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\" \
                        type='win32' \
                        name='Microsoft.Windows.Common-Controls' \
                        version='6.0.0.0' processorArchitecture='x86' \
                        publicKeyToken='6595b64144ccf1df' \
                        language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\" \
                        type='win32' \
                        name='Microsoft.Windows.Common-Controls' \
                        version='6.0.0.0' processorArchitecture='ia64' \
                        publicKeyToken='6595b64144ccf1df' \
                        language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\" \
                        type='win32' \
                        name='Microsoft.Windows.Common-Controls' \
                        version='6.0.0.0' processorArchitecture='amd64' \
                        publicKeyToken='6595b64144ccf1df' \
                        language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\" \
                        type='win32' \
                        name='Microsoft.Windows.Common-Controls' \
                        version='6.0.0.0' processorArchitecture='*' \
                        publicKeyToken='6595b64144ccf1df' \
                        language='*'\"")
#endif
#endif
