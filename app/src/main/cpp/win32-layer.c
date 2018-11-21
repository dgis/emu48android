#include "pch.h"
//#include <mach/mach_time.h>
#include <fcntl.h>
#include <unistd.h>

HANDLE hWnd;
LPTSTR szTitle;

//static NSMutableDictionary *gEventLockDict;
static HANDLE gEventId;


BOOL SetCurrentDirectory(LPCTSTR path)
{
    if(path == NULL)
        return FALSE;

    return chdir(path);
}

int CreateFile(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPVOID lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, LPVOID hTemplateFile)
{
    int flags = O_RDWR;
    int fd = -1;
    struct flock lock;
    mode_t perm = S_IRUSR | S_IWUSR;

    if (GENERIC_READ == dwDesiredAccess)
        flags = O_RDONLY;
    else
    {
        if (GENERIC_WRITE == dwDesiredAccess)
            flags = O_WRONLY;
        else if (0 != ((GENERIC_READ|GENERIC_WRITE) & dwDesiredAccess))
            flags = O_RDWR;

        if (CREATE_ALWAYS == dwCreationDisposition)
            flags |= O_CREAT;
    }

    fd = open(lpFileName, flags, perm);
//    if (-1 != fd && 0 != dwShareMode)
//    {
//        // Not specifiying shared write means non-shared (exclusive) write
//        if (0 == (dwShareMode & FILE_SHARE_WRITE))
//            lock.l_type = F_WRLCK;
//        else if (0 != (dwShareMode & FILE_SHARE_READ))
//            lock.l_type = F_RDLCK;
//
//        // Lock entire file
//        lock.l_len = lock.l_start = 0;
//        lock.l_whence = SEEK_SET;
//
//        if (-1 == fcntl(fd, F_SETLK, &lock) && (EACCES == errno || EAGAIN == errno))
//        {
//            close(fd);
//            return -1;
//        }
//    }

    return fd;
}

BOOL ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped) {
    return FALSE;
}

DWORD SetFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod) {
    return 0;
}

DWORD GetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh) {
    return 0;
}

BOOL WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {
    return FALSE;
}

HANDLE CreateFileMapping(HANDLE hFile, LPSECURITY_ATTRIBUTES lpFileMappingAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCSTR lpName) {
    return NULL;
}

LPVOID MapViewOfFile(HANDLE hFileMappingObject,DWORD dwDesiredAccess, DWORD dwFileOffsetHigh,DWORD dwFileOffsetLow, SIZE_T dwNumberOfBytesToMap) {
    return NULL;
}

BOOL UnmapViewOfFile(LPCVOID lpBaseAddress) {
    return NULL;
}

BOOL SetEndOfFile(HANDLE hFile) {
    return FALSE;
}

/*
OSErr ReadFile(FSPtr &file, void *buf, SInt64 size, void *, void *)
{
  OSErr e;
  short filenum = *file;
  if(filenum==0)	// file needs to be open first
    return -1;

  SInt64 tot_read = 0;
  ByteCount bytesread;
  // Align read on 4K boundary if possible
  const SInt64 bufsize = (size<4096) ? size : 4096;
  while(noErr==(e = FSReadFork(filenum, fsAtMark|noCacheMask, 0, bufsize, buf+tot_read, &bytesread))) {
    tot_read += bytesread;
    if(tot_read>=size)
      break;
  }

  if(e==eofErr)
    tot_read += bytesread;

  if(tot_read != size)
    return -1;

  // EOF is not an error, just a sign that the file was read successfully
  return (e==eofErr) ? noErr : e;
}


OSErr WriteFile(FSPtr &file, void *buf, SInt64 size, void *, void *)
{
  short filenum = *file;
  if(filenum==0)	// file needs to be open first
    return -1;

  FSRef fref = *file;
  FSPtr tempfile = FSPtr(new FSFile());
  UInt32 currentTime;
  ProcessSerialNumber pid;	// 64-bit data
  DCFStringPtr tempFileName;
  FSRef tempfolder;
  OSErr e;

  FSCatalogInfo inf;

  // If destination is different volume, save the temp file there
  // (since FSExchange only works inside the same volume)
  if((e=FSFindFolder(kLocalDomain, kTemporaryFolderType, TRUE, &tempfolder))==noErr) {
    if((e=FSGetCatalogInfo(&fref, kFSCatInfoVolume, &inf, NULL, NULL, NULL))==noErr) {
      FSVolumeRefNum origvol = inf.volume;
      // Get the actual vol number of the temp folder
      e = FSGetCatalogInfo(&tempfolder, kFSCatInfoVolume, &inf, NULL, NULL, NULL);
      if(e==noErr) {
        if(origvol != inf.volume)// on different volumes
          tempfolder = file->parent;	// force use of same volume
      }
    }
  }

  if(e!=noErr)
    tempfolder = file->parent;	// try a sensible alternative

  // Generate tempfile name based on current time and process pid
  GetDateTime(&currentTime);
  e = GetCurrentProcess(&pid);
  tempFileName = MakeCFObject(
    (e==noErr) ?
      CFStringCreateWithFormat(NULL, NULL, CFSTR("%lu%lu%@%lu"), pid.highLongOfPSN, pid.lowLongOfPSN, file->filename, currentTime) :
      CFStringCreateWithFormat(NULL, NULL, CFSTR("JFCd%@%d"), file->filename, currentTime)
    );
  if(tempFileName.get()==NULL)
    return -1;

  tempfile->parent = tempfolder;
  tempfile->filename = CFRetain(*tempFileName);
  e = tempfile->open(fsRdWrPerm, false, FOUR_CHAR_CODE('trsh'));

  if(e==noErr) {
    // write data here
    SInt64 tot_written = 0;
    ByteCount bytes_written;
    short temprefnum = *tempfile;
    const SInt64 bufsize = (size<4096) ? size : 4096;
    while(noErr==(e = FSWriteFork(temprefnum,
        fsAtMark|noCacheMask, // os keeps track of file pointer
        0, // ignored
        bufsize, buf+tot_written, &bytes_written))) {
      tot_written += bytes_written;
      if(tot_written>=size)
        break;
    }

    if(tot_written < size) {	// disk full? couldn't complete write
      FSRef tempref = *tempfile;
      tempfile.reset();
      FSDeleteObject(&tempref);
      return (e==noErr) ? -1 : e;	// error no matter what
    }
  }

  if(e == noErr) {
    FSRef tempref = *tempfile;
    FSRef finalsrc, finaldest;
    // Must close the files early before deleting
    tempfile.reset(); // calls ~FSFile(), which calls FSCloseFork()
    file.reset();
    e = FSExchangeObjectsCompat(&tempref, &fref, &finalsrc, &finaldest);
    if(e == noErr)
      FSDeleteObject(&finalsrc);
  }

  return e;
}


UInt64 GetFileSize(FSPtr &file, void *)
{
  FSRef fref = *file;
  FSCatalogInfo info;

  OSErr e = FSGetCatalogInfo(
              &fref,
              kFSCatInfoDataSizes,
              &info,
              NULL, NULL, NULL);

  return (e==noErr) ? info.dataLogicalSize : 0;
}


int CompareFileTime(FILETIME *a, FILETIME *b)
{
  int result = 0;
  if(a->dwHighDateTime > b->dwHighDateTime)	// guaranteed a > b
    result = 1;
  else if(a->dwHighDateTime < b->dwHighDateTime)	// guaranteed a < b
    result = -1;
  else {
    if(a->dwLowDateTime > b->dwLowDateTime)
      result = 1;
    else if(a->dwLowDateTime < b->dwLowDateTime)
      result = -1;
  }

  return result;
}


long SetFilePointer(FSPtr &file, long offset, void *, FilePointerType startpoint)
{
  SInt16 forknum = *file;
  SInt64 updatedpos = 0;

  UInt16 mode;
  switch(startpoint) {
  case FILE_BEGIN: mode = fsFromStart; break;	// offset must be +ve
  case FILE_CURRENT: mode = fsFromMark; break;
  case FILE_END: mode = fsFromLEOF; break;	// offset must be -ve
  default:
    return INVALID_SET_FILE_POINTER;
  }

  OSErr e = FSSetForkPosition(forknum, mode, offset);
  // Duplicate behaviour of Win32 call by returning the updated position
  if(e==noErr)
    e = FSGetForkPosition(forknum, &updatedpos);

  return (e==noErr) ? (long)updatedpos : INVALID_SET_FILE_POINTER;
}


void SetTimer(void *, TimerType id, int msec, void *)
{
  switch(id) {
  case TIME_SHOW:
    doc_manager->getCurrent()->SetShowTimer(msec);
    break;
  case TIME_NEXT:
    doc_manager->getCurrent()->SetNextTimer(msec);
    break;
  }
}
*/

BOOL GetOpenFileName(LPOPENFILENAME openFilename) {
    //TODO
    return FALSE;
}
BOOL GetSaveFileName(LPOPENFILENAME openFilename) {
    //TODO
    return FALSE;
}

HANDLE LoadImage(HINSTANCE hInst, LPCSTR name, UINT type, int cx, int cy, UINT fuLoad) {
    //TODO
    return NULL;
}

LRESULT SendMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    //TODO
    return NULL;
}


int MessageBox(HANDLE h, LPCTSTR szMessage, LPCTSTR title, int flags)
{
    int result = IDOK;
//#if !TARGET_OS_IPHONE
//    NSAlert *alert = [[NSAlert alloc] init];
//    [alert setMessageText: NSLocalizedString([NSString stringWithUTF8String: szMessage],@"")];
//    if (0 != (flags & MB_OK))
//    {
//        [alert addButtonWithTitle: NSLocalizedString(@"OK",@"")];
//    }
//    else if (0 != (flags & MB_YESNO))
//    {
//        [alert addButtonWithTitle: NSLocalizedString(@"Yes",@"")];
//        [alert addButtonWithTitle: NSLocalizedString(@"No",@"")];
//    }
//    else if (0 != (flags & MB_YESNOCANCEL))
//    {
//        [alert addButtonWithTitle: NSLocalizedString(@"Yes",@"")];
//        [alert addButtonWithTitle: NSLocalizedString(@"Cancel",@"")];
//        [alert addButtonWithTitle: NSLocalizedString(@"No",@"")];
//    }
//
//    if (0 != (flags & MB_ICONSTOP))
//    [alert setAlertStyle: NSAlertStyleCritical];
//    else if (0 != (flags & MB_ICONINFORMATION))
//    [alert setAlertStyle: NSAlertStyleInformational];
//
//    result = (int)[alert runModal];
//    [alert release];
//
//    if (0 != (flags & MB_OK))
//        result = IDOK;
//    else if (0 != (flags & MB_YESNO))
//        result = NSAlertFirstButtonReturn ? IDYES : IDNO;
//    else if (0 != (flags & MB_YESNOCANCEL))
//        result = NSAlertFirstButtonReturn ? IDYES :
//                 NSAlertSecondButtonReturn ? IDCANCEL : IDNO;
//#endif
    return result;
}

BOOL QueryPerformanceFrequency(PLARGE_INTEGER l)
{
//    static struct mach_timebase_info timebase = { 0, 0 };
//    if (0 == timebase.denom)
//        mach_timebase_info(&timebase);
////    l->LowPart  = 1e9 * timebase.denom / timebase.numer;
//    l->QuadPart=1000000;
    return TRUE;
}

BOOL QueryPerformanceCounter(PLARGE_INTEGER l)
{
//    l->QuadPart = mach_absolute_time() / 1000;
    return TRUE;
}

DWORD timeGetTime(void)
{
    time_t t = time(NULL);
    return (DWORD)(t * 1000);
}

void EnterCriticalSection(CRITICAL_SECTION *lock)
{
    pthread_mutex_lock(lock);
}

void LeaveCriticalSection(CRITICAL_SECTION *lock)
{
    pthread_mutex_unlock(lock);
}


// https://github.com/NeoSmart/PEvents
HANDLE CreateEvent(WORD attr, BOOL is_manual_reset, BOOL is_signaled, WORD name)
{
//    if (nil == gEventLockDict)
//    {
//        gEventLockDict = [[NSMutableDictionary alloc] init];
//    }
//    ++gEventId;
//    NSNumber *key = [[NSNumber alloc] initWithInt: gEventId];
//    NSConditionLock *lock = [[NSConditionLock alloc] initWithCondition: 0];
//    [gEventLockDict setObject:lock forKey:key];
//    [lock release];
//    [key release];
////    if (NULL == gEventLock)
////    {
////        gEventLock = [[NSConditionLock alloc] initWithCondition: 0];
////    }

    return gEventId;
}

void SetEvent(HANDLE eventId)
{
//    NSNumber *key = [[NSNumber alloc] initWithInt: eventId];
//    NSConditionLock *lock = [gEventLockDict objectForKey: key];
//    [key release];
//    if (lock)
//    {
//        [lock lock];
//        [lock unlockWithCondition: eventId];
//    }
}

BOOL ResetEvent(HANDLE eventId)
{
//    NSNumber *key = [[NSNumber alloc] initWithInt: eventId];
//    NSConditionLock *lock = [gEventLockDict objectForKey: key];
//    [key release];
//    if (lock)
//    {
//        [lock lock];
//        [lock unlockWithCondition: 0];
//        return TRUE;
//    }
    return FALSE;
}

void DestroyEvent(HANDLE eventId)
{
//    NSNumber *key = [[NSNumber alloc] initWithInt: eventId];
//    NSConditionLock *lock = [gEventLockDict objectForKey: key];
//    if (lock)
//    {
//        [gEventLockDict removeObjectForKey: key];
//    }
//    [key release];
}

DWORD WaitForSingleObject(HANDLE eventId, int timeout)
{
    DWORD result = WAIT_OBJECT_0;
//    NSNumber *key = [[NSNumber alloc] initWithInt: eventId];
//    NSConditionLock *lock = [gEventLockDict objectForKey: key];
//    [key release];
//
//    if (nil == lock)
//        return WAIT_FAILED;
//
//    if (timeout > 0)
//    {
//        NSDate *timeoutDate = [[NSDate alloc] initWithTimeIntervalSinceNow: (timeout/1000.0)];
//        if (![lock lockWhenCondition:eventId beforeDate:timeoutDate])
//        result = WAIT_TIMEOUT;
//        [timeoutDate release];
//    }
//    else
//    {
//        [lock lockWhenCondition: eventId];
//        [lock unlockWithCondition: 0];
//    }
    return result;
}

void Sleep(int ms)
{
//    [[NSRunLoop currentRunLoop] runUntilDate: [NSDate dateWithTimeIntervalSinceNow: (ms / 1000.0)]];
////    [NSThread sleepUntilDate: [NSDate dateWithTimeIntervalSinceNow: (ms / 1000.0)]];
}


BOOL GetSystemPowerStatus(LPSYSTEM_POWER_STATUS status)
{
    status->ACLineStatus = AC_LINE_ONLINE;
    return TRUE;
}


BOOL GetWindowPlacement(HWND hWnd, WINDOWPLACEMENT *lpwndpl) { return 0; }


HANDLE WINAPI CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId) {
    //TODO
    return NULL;
}

BOOL WINAPI CloseHandle(HANDLE hObject) {
    //TODO
    // Can be a thread/event/file hande!
    return 0;
}

BOOL WINAPI MessageBeep(UINT uType) {
    //TODO System beep
    return 1;
}

HGLOBAL WINAPI GlobalAlloc(UINT uFlags, SIZE_T dwBytes) {
    //TODO
    return NULL;
}
LPVOID WINAPI GlobalLock (HGLOBAL hMem) {
    //TODO
    return NULL;
}

BOOL WINAPI GlobalUnlock(HGLOBAL hMem) {
    //TODO
    return 0;
}

BOOL WINAPI OpenClipboard(HWND hWndNewOwner) {
    //TODO
    return 0;
}
BOOL WINAPI CloseClipboard(VOID) {
    //TODO
    return 0;
}

BOOL WINAPI EmptyClipboard(VOID) {
    //TODO
    return 0;
}

HANDLE WINAPI SetClipboardData(UINT uFormat,HANDLE hMem) {
    //TODO
    return NULL;
}

BOOL WINAPI IsClipboardFormatAvailable(UINT format) {
    //TODO
    return 0;
}

HANDLE WINAPI GetClipboardData(UINT uFormat) {
    //TODO
    return NULL;
}

HGLOBAL WINAPI GlobalFree(HGLOBAL hMem) {
    //TODO
    return NULL;
}

DWORD GetPrivateProfileString(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpDefault, LPTSTR lpReturnedString, DWORD nSize, LPCTSTR lpFileName) {
    //TODO
#ifdef UNICODE
    wcsncpy(lpReturnedString, lpDefault, nSize);
#else
    strncpy(lpReturnedString, lpDefault, nSize);
#endif
    return 0;
}
UINT GetPrivateProfileInt(LPCTSTR lpAppName, LPCTSTR lpKeyName, INT nDefault, LPCTSTR lpFileName) {
    //TODO
    return nDefault;
}
BOOL WritePrivateProfileString(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpString, LPCTSTR lpFileName) {
    //TODO
    return 0;
}

MMRESULT timeSetEvent(UINT uDelay, UINT uResolution, LPTIMECALLBACK fptc, DWORD_PTR dwUser, UINT fuEvent) {
    //TODO
    return 0; //No error
}
MMRESULT timeKillEvent(UINT uTimerID) {
    //TODO
    return 0; //No error
}
MMRESULT timeGetDevCaps(LPTIMECAPS ptc, UINT cbtc) {
    //TODO
    return 0; //No error
}
MMRESULT timeBeginPeriod(UINT uPeriod) {
    //TODO
    return 0; //No error
}
MMRESULT timeEndPeriod(UINT uPeriod) {
    //TODO
    return 0; //No error
}
VOID GetLocalTime(LPSYSTEMTIME lpSystemTime) {
    //TODO
    return;
}



#ifdef UNICODE


int WINAPI wvsprintf(LPWSTR, LPCWSTR, va_list arglist) {
    return vswprintf(arg1, MAX_PATH, arg2, arglist);
}
DWORD GetFullPathName(LPCWSTR lpFileName, DWORD nBufferLength, LPWSTR lpBuffer, LPWSTR* lpFilePart) { return 0; }
LPWSTR lstrcpyn(LPWSTR lpString1, LPCWSTR lpString2,int iMaxLength) {
    return strcpy(lpString1, lpString2);
}
LPWSTR lstrcat(LPWSTR lpString1, LPCWSTR lpString2) {
    return NULL;
}
void __cdecl _wsplitpath(wchar_t const* _FullPath, wchar_t* _Drive, wchar_t* _Dir, wchar_t* _Filename, wchar_t* _Ext) {}
int WINAPI lstrcmp(LPCWSTR lpString1, LPCWSTR lpString2) {
    return wcscmp(lpString1, lpString2);
}
int lstrcmpi(LPCWSTR lpString1, LPCWSTR lpString2) {
    return wcscasecmp(lpString1, lpString2);
}
#else

int WINAPI wvsprintf(LPSTR arg1, LPCSTR arg2, va_list arglist) {
    return vsprintf(arg1, arg2, arglist);
}
DWORD GetFullPathName(LPCSTR lpFileName, DWORD nBufferLength, LPSTR lpBuffer, LPSTR* lpFilePart) { return 0; }
LPSTR lstrcpyn(LPSTR lpString1, LPCSTR lpString2,int iMaxLength) {
    return strcpy(lpString1, lpString2);
}
LPSTR lstrcat(LPSTR lpString1, LPCSTR lpString2) {
    return NULL;
}
void __cdecl _splitpath(char const* _FullPath, char* _Drive, char* _Dir, char* _Filename, char* _Ext) {}
int WINAPI lstrcmp(LPCSTR lpString1, LPCSTR lpString2) {
    return strcmp(lpString1, lpString2);
}
int lstrcmpi(LPCSTR lpString1, LPCSTR lpString2) {
    return strcasecmp(lpString1, lpString2);
}



#endif // !UNICODE

void _wmakepath(wchar_t _Buffer, wchar_t const* _Drive, wchar_t const* _Dir, wchar_t const* _Filename, wchar_t const* _Ext)
{
}


BOOL GetClientRect(HWND hWnd, LPRECT lpRect)
{
    return 0;
}