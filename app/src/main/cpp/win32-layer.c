#include "core/pch.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <pthread.h>
#include <android/bitmap.h>
#include "core/resource.h"
#include "win32-layer.h"
#include "core/Emu48.h"

extern JavaVM *java_machine;
extern jobject bitmapMainScreen;
extern AndroidBitmapInfo androidBitmapInfo;
extern UINT nBackgroundW;
extern UINT nBackgroundH;

HANDLE hWnd;
LPTSTR szTitle;
LPTSTR szCurrentDirectorySet = NULL;
const TCHAR * assetsPrefix = _T("assets/"),
        assetsPrefixLength = 7;
AAssetManager * assetManager;
//static HDC mainPaintDC = NULL;
struct timerEvent {
    BOOL valid;
    int timerId;
    LPTIMECALLBACK fptc;
    DWORD_PTR dwUser;
    UINT fuEvent;
    timer_t timer;
};

#define MAX_TIMER 10
struct timerEvent timerEvents[MAX_TIMER];

#define MAX_FILE_MAPPING_HANDLE 10
static HANDLE fileMappingHandles[MAX_FILE_MAPPING_HANDLE];

void win32Init() {
    for (int i = 0; i < MAX_TIMER; ++i) {
        timerEvents[i].valid = FALSE;
    }
    for (int i = 0; i < MAX_FILE_MAPPING_HANDLE; ++i) {
        fileMappingHandles[i] = NULL;
    }
}

VOID OutputDebugString(LPCSTR lpOutputString) {
    LOGD("%s", lpOutputString);
}

DWORD GetCurrentDirectory(DWORD nBufferLength, LPTSTR lpBuffer) {
    if(getcwd(lpBuffer, nBufferLength)) {
        return nBufferLength;
    }
    return 0;
}

BOOL SetCurrentDirectory(LPCTSTR path)
{
    if(path == NULL)
        return FALSE;

    if(_tcsncmp(path, assetsPrefix, assetsPrefixLength / sizeof(TCHAR)) == 0)
        szCurrentDirectorySet = (LPTSTR) (path + assetsPrefixLength);
    else
        szCurrentDirectorySet = NULL;

    return (BOOL) chdir(path);
}

extern BOOL settingsPort2en;
extern BOOL settingsPort2wr;

HANDLE CreateFile(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPVOID lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, LPVOID hTemplateFile)
{
    BOOL forceNormalFile = FALSE;
    if(_tcscmp(lpFileName, szPort2Filename) == 0) {
        // Special case for Port2 filename
        forceNormalFile = TRUE;
        if(!settingsPort2wr && (dwDesiredAccess & GENERIC_WRITE))
            return (HANDLE) INVALID_HANDLE_VALUE;
    }

    if(!forceNormalFile && (szCurrentDirectorySet || _tcsncmp(lpFileName, assetsPrefix, assetsPrefixLength / sizeof(TCHAR)) == 0)) {
        TCHAR szFileName[MAX_PATH];
        AAsset * asset = NULL;
        szFileName[0] = _T('\0');
        if(szCurrentDirectorySet) {
            _tcscpy(szFileName, szCurrentDirectorySet);
            _tcscat(szFileName, lpFileName);
            asset = AAssetManager_open(assetManager, szFileName, AASSET_MODE_STREAMING);
        } else {
            asset = AAssetManager_open(assetManager, lpFileName + assetsPrefixLength, AASSET_MODE_STREAMING);
        }
        if(asset) {
            HANDLE handle = malloc(sizeof(struct _HANDLE));
            memset(handle, 0, sizeof(struct _HANDLE));
            handle->handleType = HANDLE_TYPE_FILE_ASSET;
            handle->fileAsset = asset;
            return handle;
        }
    } else {

        BOOL useOpenFileFromContentResolver = FALSE;
        int flags = O_RDWR;
        int fd = -1;
        struct flock lock;
        mode_t perm = S_IRUSR | S_IWUSR;

        if (GENERIC_READ == dwDesiredAccess)
            flags = O_RDONLY;
        else {
            if (GENERIC_WRITE == dwDesiredAccess)
                flags = O_WRONLY;
            else if (0 != ((GENERIC_READ | GENERIC_WRITE) & dwDesiredAccess))
                flags = O_RDWR;

            if (CREATE_ALWAYS == dwCreationDisposition)
                flags |= O_CREAT;
        }

        TCHAR * urlContentSchemeFound = _tcsstr(lpFileName, _T("content://"));
        if(urlContentSchemeFound) {
            fd = openFileFromContentResolver(lpFileName, dwDesiredAccess);
            useOpenFileFromContentResolver = TRUE;
            if(fd == -1) {
                LOGD("openFileFromContentResolver() %d", errno);
            }
        } else {
            TCHAR * urlFileSchemeFound = _tcsstr(lpFileName, _T("file://"));
            if(urlFileSchemeFound)
                lpFileName = urlFileSchemeFound + 7;
            fd = open(lpFileName, flags, perm);
            if(fd == -1) {
                LOGD("open() %d", errno);
            }
        }
        if (fd != -1) {
            HANDLE handle = malloc(sizeof(struct _HANDLE));
            memset(handle, 0, sizeof(struct _HANDLE));
            handle->handleType = HANDLE_TYPE_FILE;
            handle->fileDescriptor = fd;
            handle->fileOpenFileFromContentResolver = useOpenFileFromContentResolver;
            return handle;
        }
    }
    return INVALID_HANDLE_VALUE;
}

BOOL ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped) {
    DWORD readByteCount = 0;
    if(hFile->handleType == HANDLE_TYPE_FILE) {
        readByteCount = (DWORD) read(hFile->fileDescriptor, lpBuffer, nNumberOfBytesToRead);
    } else if(hFile->handleType == HANDLE_TYPE_FILE_ASSET) {
        readByteCount = (DWORD) AAsset_read(hFile->fileAsset, lpBuffer, nNumberOfBytesToRead);
    }
    if(lpNumberOfBytesRead)
        *lpNumberOfBytesRead = readByteCount;
    return readByteCount >= 0;
}

BOOL WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {
    if(hFile->handleType == HANDLE_TYPE_FILE_ASSET)
        return FALSE;
    ssize_t writenByteCount = write(hFile->fileDescriptor, lpBuffer, nNumberOfBytesToWrite);
    if(lpNumberOfBytesWritten)
        *lpNumberOfBytesWritten = (DWORD) writenByteCount;
    return writenByteCount >= 0;
}

DWORD SetFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod) {
    int moveMode = FILE_BEGIN;
    if(dwMoveMethod == FILE_BEGIN)
        moveMode = SEEK_SET;
    else if(dwMoveMethod == FILE_CURRENT)
        moveMode = SEEK_CUR;
    else if(dwMoveMethod == FILE_END)
        moveMode = SEEK_END;
    int seekResult = -1;
    if(hFile->handleType == HANDLE_TYPE_FILE) {
        seekResult = (int) lseek(hFile->fileDescriptor, lDistanceToMove, moveMode);
    } else if(hFile->handleType == HANDLE_TYPE_FILE_ASSET) {
        seekResult = (int) AAsset_seek64(hFile->fileAsset, lDistanceToMove, moveMode);
    }
    return seekResult < 0 ? INVALID_SET_FILE_POINTER : seekResult;
}

BOOL SetEndOfFile(HANDLE hFile) {
    if(hFile->handleType == HANDLE_TYPE_FILE_ASSET)
        return FALSE;
    off_t currentPosition = lseek(hFile->fileDescriptor, 0, SEEK_CUR);
    int truncateResult = ftruncate(hFile->fileDescriptor, currentPosition);
    return truncateResult == 0;
}

DWORD GetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh) {
    if(lpFileSizeHigh)
        *lpFileSizeHigh = 0;
    if(hFile->handleType == HANDLE_TYPE_FILE) {
        off_t currentPosition = lseek(hFile->fileDescriptor, 0, SEEK_CUR);
        off_t fileLength = lseek(hFile->fileDescriptor, 0, SEEK_END); // + 1;
        lseek(hFile->fileDescriptor, currentPosition, SEEK_SET);
        return (DWORD) fileLength;
    } else if(hFile->handleType == HANDLE_TYPE_FILE_ASSET) {
        return (DWORD) AAsset_getLength64(hFile->fileAsset);
    }
}

//** https://www.ibm.com/developerworks/systems/library/es-MigratingWin32toLinux.html
//https://www.ibm.com/developerworks/systems/library/es-win32linux.html
//https://www.ibm.com/developerworks/systems/library/es-win32linux-sem.html
HANDLE CreateFileMapping(HANDLE hFile, LPSECURITY_ATTRIBUTES lpFileMappingAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCSTR lpName) {
    HANDLE handle = malloc(sizeof(struct _HANDLE));
    memset(handle, 0, sizeof(struct _HANDLE));
    if(hFile->handleType == HANDLE_TYPE_FILE) {
        handle->handleType = HANDLE_TYPE_FILE_MAPPING;
        handle->fileDescriptor = hFile->fileDescriptor;
    } else if(hFile->handleType == HANDLE_TYPE_FILE_ASSET) {
        handle->handleType = HANDLE_TYPE_FILE_MAPPING_ASSET;
        handle->fileAsset = hFile->fileAsset;
    }
    handle->fileMappingSize = (dwMaximumSizeHigh << 32) | dwMaximumSizeLow;
    handle->fileMappingAddress = NULL;
    return handle;
}

//https://msdn.microsoft.com/en-us/library/Aa366761(v=VS.85).aspx
LPVOID MapViewOfFile(HANDLE hFileMappingObject, DWORD dwDesiredAccess, DWORD dwFileOffsetHigh, DWORD dwFileOffsetLow, SIZE_T dwNumberOfBytesToMap) {
    off_t offset = (dwFileOffsetHigh << 32) & dwFileOffsetLow;
    LPVOID result = NULL;
    if(hFileMappingObject->handleType == HANDLE_TYPE_FILE_MAPPING) {
        int prot = PROT_NONE;
        if (dwDesiredAccess & FILE_MAP_READ)
            prot |= PROT_READ;
        if (dwDesiredAccess & FILE_MAP_WRITE)
            prot |= PROT_WRITE;
        hFileMappingObject->fileMappingAddress = mmap(NULL, hFileMappingObject->fileMappingSize,
                                                      prot, MAP_PRIVATE,
                                                      hFileMappingObject->fileDescriptor, offset);
    } else if(hFileMappingObject->handleType == HANDLE_TYPE_FILE_MAPPING_ASSET) {
        if (dwDesiredAccess & FILE_MAP_WRITE)
            return NULL;
        hFileMappingObject->fileMappingAddress = (LPVOID) (AAsset_getBuffer(hFileMappingObject->fileAsset) + offset);
    }
    if(hFileMappingObject->fileMappingAddress) {
        for (int i = 0; i < MAX_FILE_MAPPING_HANDLE; ++i) {
            if(!fileMappingHandles[i]) {
                fileMappingHandles[i] = hFileMappingObject;
                break;
            }
        }
    }
    result = hFileMappingObject->fileMappingAddress;
    return result;
}

// https://msdn.microsoft.com/en-us/library/aa366882(v=vs.85).aspx
BOOL UnmapViewOfFile(LPCVOID lpBaseAddress) {
    int result = -1;
    for (int i = 0; i < MAX_FILE_MAPPING_HANDLE; ++i) {
        HANDLE fileMappingHandle = fileMappingHandles[i];
        if(fileMappingHandle && lpBaseAddress == fileMappingHandle->fileMappingAddress) {
            if(fileMappingHandle->handleType == HANDLE_TYPE_FILE_MAPPING) {
                // munmap does not seem to work, so:
                off_t currentPosition = lseek(fileMappingHandle->fileDescriptor, 0, SEEK_CUR);
                lseek(fileMappingHandle->fileDescriptor, 0, SEEK_SET);
                write(fileMappingHandle->fileDescriptor, fileMappingHandle->fileMappingAddress, fileMappingHandle->fileMappingSize);
                lseek(fileMappingHandle->fileDescriptor, currentPosition, SEEK_SET);
                result = munmap(lpBaseAddress, fileMappingHandle->fileMappingSize);
            } else if(fileMappingHandle->handleType == HANDLE_TYPE_FILE_MAPPING_ASSET) {
                // No need to unmap
                result = 0;
            }
            fileMappingHandles[i] = NULL;
            break;
        }
    }

    return result == 0;
}

//https://github.com/neosmart/pevents/blob/master/src/pevents.cpp
HANDLE CreateEvent(LPVOID lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCTSTR name) {
    HANDLE handle = malloc(sizeof(struct _HANDLE));
    memset(handle, 0, sizeof(struct _HANDLE));
    handle->handleType = HANDLE_TYPE_EVENT;

    int result = pthread_cond_init(&(handle->eventCVariable), 0);
    _ASSERT(result == 0);

    result = pthread_mutex_init(&(handle->eventMutex), 0);
    _ASSERT(result == 0);

    handle->eventState = FALSE;
    handle->eventAutoReset = bManualReset ? FALSE : TRUE;

    if (bInitialState)
    {
        result = SetEvent(handle);
        _ASSERT(result == 0);
    }

    return handle;
}

BOOL SetEvent(HANDLE hEvent) {
    int result = pthread_mutex_lock(&hEvent->eventMutex);
    _ASSERT(result == 0);

    hEvent->eventState = TRUE;

    //Depending on the event type, we either trigger everyone or only one
    if (hEvent->eventAutoReset)
    {
        //hEvent->eventState can be false if compiled with WFMO support
        if (hEvent->eventState)
        {
            result = pthread_mutex_unlock(&hEvent->eventMutex);
            _ASSERT(result == 0);
            result = pthread_cond_signal(&hEvent->eventCVariable);
            _ASSERT(result == 0);
            return 0;
        }
    }
    else
    {
        result = pthread_mutex_unlock(&hEvent->eventMutex);
        _ASSERT(result == 0);
        result = pthread_cond_broadcast(&hEvent->eventCVariable);
        _ASSERT(result == 0);
    }

    return 0;
}

BOOL ResetEvent(HANDLE hEvent)
{
    if(hEvent) {
        int result = pthread_mutex_lock(&hEvent->eventMutex);
        _ASSERT(result == 0);

        hEvent->eventState = FALSE;

        result = pthread_mutex_unlock(&hEvent->eventMutex);
        _ASSERT(result == 0);

        return TRUE;
    }
    return FALSE;
}

int UnlockedWaitForEvent(HANDLE hHandle, uint64_t milliseconds)
{
    int result = 0;
    if (!hHandle->eventState)
    {
        //Zero-timeout event state check optimization
        if (milliseconds == 0)
        {
            return WAIT_TIMEOUT;
        }

        struct timespec ts;
        if (milliseconds != (uint64_t) -1)
        {
            struct timeval tv;
            gettimeofday(&tv, NULL);

            uint64_t nanoseconds = ((uint64_t) tv.tv_sec) * 1000 * 1000 * 1000 + milliseconds * 1000 * 1000 + ((uint64_t) tv.tv_usec) * 1000;

            ts.tv_sec = nanoseconds / 1000 / 1000 / 1000;
            ts.tv_nsec = (nanoseconds - ((uint64_t) ts.tv_sec) * 1000 * 1000 * 1000);
        }

        do
        {
            //Regardless of whether it's an auto-reset or manual-reset event:
            //wait to obtain the event, then lock anyone else out
            if (milliseconds != (uint64_t) -1)
            {
                result = pthread_cond_timedwait(&hHandle->eventCVariable, &hHandle->eventMutex, &ts);
            }
            else
            {
                result = pthread_cond_wait(&hHandle->eventCVariable, &hHandle->eventMutex);
            }
        } while (result == 0 && !hHandle->eventState);

        if (result == 0 && hHandle->eventAutoReset)
        {
            //We've only accquired the event if the wait succeeded
            hHandle->eventState = FALSE;
        }
    }
    else if (hHandle->eventAutoReset)
    {
        //It's an auto-reset event that's currently available;
        //we need to stop anyone else from using it
        result = 0;
        hHandle->eventState = FALSE;
    }
    //Else we're trying to obtain a manual reset event with a signaled state;
    //don't do anything

    return result;
}

static DWORD ThreadStart(LPVOID lpThreadParameter) {
    HANDLE handle = (HANDLE)lpThreadParameter;
    if(handle) {
        handle->threadStartAddress(handle->threadParameter);
    }

    CloseHandle(handle->threadEventMessage);
}


// Should be protected by mutex
#define MAX_CREATED_THREAD 20
static HANDLE threads[MAX_CREATED_THREAD];
static int threadsNextIndex = 0;

HANDLE CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId) {
    pthread_attr_t  attr;
    pthread_attr_init(&attr);
    if(dwStackSize)
        pthread_attr_setstacksize(&attr, dwStackSize);

    HANDLE handle = malloc(sizeof(struct _HANDLE));
    memset(handle, 0, sizeof(struct _HANDLE));
    handle->handleType = HANDLE_TYPE_THREAD;
    handle->threadStartAddress = lpStartAddress;
    handle->threadParameter = lpParameter;
    handle->threadEventMessage = CreateEvent(NULL, FALSE, FALSE, NULL);
    threads[threadsNextIndex] = handle;

    //Suspended
    //https://stackoverflow.com/questions/3140867/suspend-pthreads-without-using-condition
    //https://stackoverflow.com/questions/7953917/create-thread-in-suspended-mode-using-pthreads
    //http://man7.org/linux/man-pages/man3/pthread_create.3.html
    int pthreadResult = pthread_create(&handle->threadId, &attr, (void *(*)(void *)) ThreadStart, handle);
    if(pthreadResult == 0) {
        threadsNextIndex++;
        if(lpThreadId)
            *lpThreadId = (DWORD) handle->threadId;
        return handle;
    } else {
        threads[threadsNextIndex] = NULL;
        CloseHandle(handle->threadEventMessage);
        free(handle);
    }
    return NULL;
}

DWORD ResumeThread(HANDLE hThread) {
    //TODO
    return 0;
}

//https://docs.microsoft.com/en-us/windows/desktop/api/synchapi/nf-synchapi-waitforsingleobject
DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds)
{
    if(hHandle->handleType == HANDLE_TYPE_THREAD) {
        _ASSERT(dwMilliseconds == INFINITE)
        if(dwMilliseconds == INFINITE) {
            pthread_join(hHandle->threadId, NULL);
        }
        //TODO for dwMilliseconds != INFINITE
        //https://stackoverflow.com/questions/2719580/waitforsingleobject-and-waitformultipleobjects-equivalent-in-linux
    } else if(hHandle->handleType == HANDLE_TYPE_EVENT) {

        int tempResult;
        if (dwMilliseconds == 0)
        {
            tempResult = pthread_mutex_trylock(&hHandle->eventMutex);
            if (tempResult == EBUSY)
            {
                return WAIT_TIMEOUT;
            }
        }
        else
        {
            tempResult = pthread_mutex_lock(&hHandle->eventMutex);
        }

        _ASSERT(tempResult == 0);

        int result = UnlockedWaitForEvent(hHandle, dwMilliseconds);

        tempResult = pthread_mutex_unlock(&hHandle->eventMutex);
        _ASSERT(tempResult == 0);

        return (DWORD) result;
    }

    DWORD result = WAIT_OBJECT_0;
    return result;
}

BOOL WINAPI CloseHandle(HANDLE hObject) {
    //https://msdn.microsoft.com/en-us/9b84891d-62ca-4ddc-97b7-c4c79482abd9
    // Can be a thread/event/file handle!
    switch(hObject->handleType) {
        case HANDLE_TYPE_FILE: {
            int closeResult;
            if(hObject->fileOpenFileFromContentResolver) {
                closeResult = closeFileFromContentResolver(hObject->fileDescriptor);
            } else {
                closeResult = close(hObject->fileDescriptor);
                if(closeResult == -1) {
                    LOGD("close() %d", errno); //9 -> EBADF
                }
            }
            if(closeResult >= 0) {
                hObject->handleType = HANDLE_TYPE_INVALID;
                hObject->fileDescriptor = 0;
                free(hObject);
                return TRUE;
            }

            break;
        }
        case HANDLE_TYPE_FILE_ASSET: {
            AAsset_close(hObject->fileAsset);
            hObject->handleType = HANDLE_TYPE_INVALID;
            hObject->fileAsset = NULL;
            free(hObject);
            return TRUE;
        }
        case HANDLE_TYPE_FILE_MAPPING: {
            hObject->handleType = HANDLE_TYPE_INVALID;
            hObject->fileDescriptor = 0;
            hObject->fileMappingSize = 0;
            hObject->fileMappingAddress = NULL;
            free(hObject);
            return TRUE;
        }
        case HANDLE_TYPE_FILE_MAPPING_ASSET: {
            hObject->handleType = HANDLE_TYPE_INVALID;
            hObject->fileAsset = NULL;
            hObject->fileMappingSize = 0;
            hObject->fileMappingAddress = NULL;
            free(hObject);
            return TRUE;
        }
        case HANDLE_TYPE_EVENT: {
            hObject->handleType = HANDLE_TYPE_INVALID;
            int result = 0;
            result = pthread_cond_destroy(&hObject->eventCVariable);
            _ASSERT(result == 0);
            result = pthread_mutex_destroy(&hObject->eventMutex);
            _ASSERT(result == 0);
            hObject->eventAutoReset = FALSE;
            hObject->eventState = FALSE;
            free(hObject);
            return TRUE;
        }
        case HANDLE_TYPE_THREAD:
            hObject->handleType = HANDLE_TYPE_INVALID;
            hObject->threadId = 0;
            free(hObject);
            return TRUE;
        default:
            break;
    }
    return FALSE;
}

void Sleep(int ms)
{
    time_t seconds = ms / 1000;
    long milliseconds = ms - 1000 * seconds;
    struct timespec timeOut, remains;
    timeOut.tv_sec = seconds;
    timeOut.tv_nsec = milliseconds * 1000000; /* 50 milliseconds */
    nanosleep(&timeOut, &remains);
}

BOOL QueryPerformanceFrequency(PLARGE_INTEGER l) {
    //https://msdn.microsoft.com/en-us/library/windows/desktop/ms644904(v=vs.85).aspx
//    static struct mach_timebase_info timebase = { 0, 0 };
//    if (0 == timebase.denom)
//        mach_timebase_info(&timebase);
////    l->LowPart  = 1e9 * timebase.denom / timebase.numer;
    l->QuadPart = 1000000;
    return TRUE;
}

BOOL QueryPerformanceCounter(PLARGE_INTEGER l)
{
    struct timespec /*{
        time_t   tv_sec;
        long     tv_nsec;
    } */ time;
    int result = clock_gettime(CLOCK_MONOTONIC, &time);
    l->QuadPart = (uint64_t) ((1e9 * time.tv_sec + time.tv_nsec) / 1000);
    return TRUE;
}
void InitializeCriticalSection(CRITICAL_SECTION * lpCriticalSection) {
    pthread_mutexattr_t Attr;
    pthread_mutexattr_init(&Attr);
    pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(lpCriticalSection, &Attr);
}
void DeleteCriticalSection(CRITICAL_SECTION * lpCriticalSection) {
    pthread_mutex_destroy(lpCriticalSection);
}

void EnterCriticalSection(CRITICAL_SECTION *lock)
{
    pthread_mutex_lock(lock);
}

void LeaveCriticalSection(CRITICAL_SECTION *lock)
{
    pthread_mutex_unlock(lock);
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
    return (UINT) nDefault;
}
BOOL WritePrivateProfileString(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpString, LPCTSTR lpFileName) {
    //TODO
    return 0;
}

/*
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


HGLOBAL WINAPI GlobalAlloc(UINT uFlags, SIZE_T dwBytes) {
    return malloc(dwBytes);
}
LPVOID WINAPI GlobalLock (HGLOBAL hMem) {
    return hMem;
}

BOOL WINAPI GlobalUnlock(HGLOBAL hMem) {
    return TRUE;
}

HGLOBAL WINAPI GlobalFree(HGLOBAL hMem) {
    free(hMem);
    return NULL;
}

BOOL GetOpenFileName(LPOPENFILENAME openFilename) {
    return FALSE;
}
BOOL GetSaveFileName(LPOPENFILENAME openFilename) {
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
BOOL PostMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    //TODO
    if(hWnd == 0 && Msg == WM_COMMAND) {
        int menuCommand = (wParam & 0xffff) - 40000;
        LOGD("Menu Item %d", menuCommand);
        sendMenuItemCommand(menuCommand);
    }
    return NULL;
}


int MessageBox(HANDLE h, LPCTSTR szMessage, LPCTSTR title, int flags)
{
//    int result = IDOK;
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
    if(flags & MB_YESNO) {
        return IDOK;
    }

    return showAlert(szMessage, flags);
}

DWORD timeGetTime(void)
{
    time_t t = time(NULL);
    return (DWORD)(t * 1000);
}

BOOL GetSystemPowerStatus(LPSYSTEM_POWER_STATUS status)
{
    status->ACLineStatus = AC_LINE_ONLINE;
    return TRUE;
}


// Wave API




// this callback handler is called every time a buffer finishes playing
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    HWAVEOUT hwo = context;
    if (hwo->pWaveHeaderNext != NULL) {
        LPWAVEHDR pWaveHeaderNext = hwo->pWaveHeaderNext->lpNext;
        //PostThreadMessage(0, MM_WOM_DONE, hwo, hwo->pWaveHeaderNext);
        free(hwo->pWaveHeaderNext->lpData);
        free(hwo->pWaveHeaderNext);
        hwo->pWaveHeaderNext = pWaveHeaderNext;
        if(pWaveHeaderNext != NULL) {
            SLresult result = (*hwo->bqPlayerBufferQueue)->Enqueue(hwo->bqPlayerBufferQueue, pWaveHeaderNext->lpData, pWaveHeaderNext->dwBufferLength);
            if (SL_RESULT_SUCCESS != result) {
                // Error
                //pthread_mutex_unlock(&hwo->audioEngineLock);
//                return;
            }
//            return;
        }
    }
//    pthread_mutex_unlock(&hwo->audioEngineLock);
}


MMRESULT waveOutOpen(LPHWAVEOUT phwo, UINT uDeviceID, LPCWAVEFORMATEX pwfx, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen) {

    HWAVEOUT handle = (HWAVEOUT)malloc(sizeof(struct _HWAVEOUT));
    memset(handle, 0, sizeof(struct _HWAVEOUT));
    handle->pwfx = (WAVEFORMATEX *) pwfx;
    handle->uDeviceID = uDeviceID;


    SLObjectItf engineObject = NULL;
    SLObjectItf outputMixObject = NULL;
    SLObjectItf bqPlayerObject = NULL;
    //pthread_mutex_t  audioEngineLock = PTHREAD_MUTEX_INITIALIZER;
    //handle->audioEngineLock = PTHREAD_MUTEX_INITIALIZER;


    SLresult result;

    // create engine
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    //_ASSERT(SL_RESULT_SUCCESS == result);

    // realize the engine
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);

    // get the engine interface, which is needed in order to create other objects
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &handle->engineEngine);

    // create output mix, with environmental reverb specified as a non-required interface
    const SLInterfaceID ids[1] = { SL_IID_ENVIRONMENTALREVERB };
    const SLboolean req[1] = { SL_BOOLEAN_FALSE };
    result = (*handle->engineEngine)->CreateOutputMix(handle->engineEngine, &outputMixObject, 1, ids, req);

    // realize the output mix
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);

    // configure audio source
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {
            SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,   // locatorType
            20                                          // numBuffers
    };
    SLDataFormat_PCM format_pcm = {
            SL_DATAFORMAT_PCM,             // formatType
            1,                             // numChannels
            SL_SAMPLINGRATE_8,             // samplesPerSec
            SL_PCMSAMPLEFORMAT_FIXED_16,   // bitsPerSample
            SL_PCMSAMPLEFORMAT_FIXED_16,   // containerSize
            SL_SPEAKER_FRONT_CENTER,       // channelMask
            SL_BYTEORDER_LITTLEENDIAN      // endianness
    };
    /*
     * Enable Fast Audio when possible:  once we set the same rate to be the native, fast audio path
     * will be triggered
     */
    format_pcm.samplesPerSec = pwfx->nSamplesPerSec * 1000;       //sample rate in mili second
    format_pcm.bitsPerSample = pwfx->wBitsPerSample;
    format_pcm.containerSize = pwfx->wBitsPerSample;
    format_pcm.numChannels = pwfx->nChannels;

    SLDataSource audioSrc = {
            &loc_bufq,   // pLocator
            &format_pcm  // pFormat
    };

    // configure audio sink
    SLDataLocator_OutputMix loc_outmix = {
            SL_DATALOCATOR_OUTPUTMIX, // locatorType
            outputMixObject           // outputMix
    };
    SLDataSink audioSnk = {
            &loc_outmix,  // pLocator
            NULL          // pFormat
    };

    /*
     * create audio player:
     *     fast audio does not support when SL_IID_EFFECTSEND is required, skip it
     *     for fast audio case
     */
    const SLInterfaceID ids2[3] = {
            SL_IID_BUFFERQUEUE,
            SL_IID_VOLUME,
            //SL_IID_EFFECTSEND,
            //SL_IID_MUTESOLO,
    };
    const SLboolean req2[3] = {
            SL_BOOLEAN_TRUE,
            SL_BOOLEAN_TRUE,
            //SL_BOOLEAN_TRUE,
            //SL_BOOLEAN_TRUE,
    };

    result = (*handle->engineEngine)->CreateAudioPlayer(handle->engineEngine, &bqPlayerObject, &audioSrc, &audioSnk, 2, ids2, req2);

    // realize the player
    result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);

    // get the play interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &handle->bqPlayerPlay);

    // get the buffer queue interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE, &handle->bqPlayerBufferQueue);

    // register callback on the buffer queue
    result = (*handle->bqPlayerBufferQueue)->RegisterCallback(handle->bqPlayerBufferQueue, bqPlayerCallback, handle);

    // get the volume interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &handle->bqPlayerVolume);

    // set the player's state to playing
    result = (*handle->bqPlayerPlay)->SetPlayState(handle->bqPlayerPlay, SL_PLAYSTATE_PLAYING);




    *phwo = handle;
    return MMSYSERR_NOERROR;
}

MMRESULT waveOutReset(HWAVEOUT hwo) {
    //TODO
    return 0;
}

MMRESULT waveOutClose(HWAVEOUT handle) {

    // destroy buffer queue audio player object, and invalidate all associated interfaces
    if (handle->bqPlayerObject != NULL) {
        (*handle->bqPlayerObject)->Destroy(handle->bqPlayerObject);
        handle->bqPlayerObject = NULL;
        handle->bqPlayerPlay = NULL;
        handle->bqPlayerBufferQueue = NULL;
        handle->bqPlayerVolume = NULL;
    }

    // destroy output mix object, and invalidate all associated interfaces
    if (handle->outputMixObject != NULL) {
        (*handle->outputMixObject)->Destroy(handle->outputMixObject);
        handle->outputMixObject = NULL;
    }

    // destroy engine object, and invalidate all associated interfaces
    if (handle->engineObject != NULL) {
        (*handle->engineObject)->Destroy(handle->engineObject);
        handle->engineObject = NULL;
        handle->engineEngine = NULL;
    }

    pthread_mutex_destroy(&handle->audioEngineLock);


    memset(handle, 0, sizeof(struct _HWAVEOUT));
    free(handle);
    return 0;
}

MMRESULT waveOutPrepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh) {
    //TODO
    return 0;
}

MMRESULT waveOutUnprepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh) {
    //TODO
    return 0;
}

MMRESULT waveOutWrite(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh) {

//    if (pthread_mutex_trylock(&hwo->audioEngineLock)) {
//        // If we could not acquire audio engine lock, reject this request and client should re-try
//        return MMSYSERR_ERROR;
//    }
    pwh->lpNext = NULL;
    if(hwo->pWaveHeaderNext == NULL) {
        hwo->pWaveHeaderNext = pwh;
        SLresult result = (*hwo->bqPlayerBufferQueue)->Enqueue(hwo->bqPlayerBufferQueue, pwh->lpData, pwh->dwBufferLength);
        if (SL_RESULT_SUCCESS != result) {
            // SL_RESULT_BUFFER_INSUFFICIENT?
            //pthread_mutex_unlock(&hwo->audioEngineLock);
            return MMSYSERR_ERROR;
        }
    } else {
        LPWAVEHDR pWaveHeaderNext = hwo->pWaveHeaderNext;
        while (pWaveHeaderNext->lpNext)
            pWaveHeaderNext = pWaveHeaderNext->lpNext;
        pWaveHeaderNext->lpNext = pwh;
    }

    return MMSYSERR_NOERROR;
}

MMRESULT waveOutGetDevCaps(UINT_PTR uDeviceID, LPWAVEOUTCAPS pwoc, UINT cbwoc) {
    if(pwoc) {
        pwoc->dwFormats = WAVE_FORMAT_4M08;
    }
    return MMSYSERR_NOERROR;
}

MMRESULT waveOutGetID(HWAVEOUT hwo, LPUINT puDeviceID) {
    //TODO
    return 0;
}




BOOL GetMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) {
    //TODO
//    pthread_t thId = pthread_self();
//    for(int i = 0; i < MAX_CREATED_THREAD; i++) {
//        HANDLE threadHandle = threads[i];
//        if(threadHandle && threadHandle->threadId == thId) {
//            WaitForSingleObject(threadHandle->threadEventMessage, INFINITE);
//            if(lpMsg)
//                memcpy(lpMsg, &threadHandle->threadMessage, sizeof(MSG));
//            if(lpMsg->message == WM_QUIT)
//                return FALSE;
//            return TRUE;
//        }
//    }

    return FALSE;
}

BOOL PostThreadMessage(DWORD idThread, UINT Msg, WPARAM wParam, LPARAM lParam) {
    //TODO not working because idThread is uint32_t (4 bytes) and threadId if a long (8 bytes)
//    for(int i = 0; i < MAX_CREATED_THREAD; i++) {
//        HANDLE threadHandle = threads[i];
//        if(threadHandle && threadHandle->threadId == idThread) {
//            threadHandle->threadMessage.hwnd = NULL;
//            threadHandle->threadMessage.message = Msg;
//            threadHandle->threadMessage.wParam = wParam;
//            threadHandle->threadMessage.lParam = lParam;
//            SetEvent(threadHandle->threadEventMessage);
//            return TRUE;
//        }
//    }
    return TRUE;
    //return FALSE;
}

BOOL DestroyWindow(HWND hWnd) {
    //TODO
    return 0;
}

BOOL GetWindowPlacement(HWND hWnd, WINDOWPLACEMENT *lpwndpl) {
    if(lpwndpl) {
        lpwndpl->rcNormalPosition.left = 0;
        lpwndpl->rcNormalPosition.top = 0;
    }
    return TRUE;
}
BOOL SetWindowPlacement(HWND hWnd, CONST WINDOWPLACEMENT *lpwndpl) { return 0; }
BOOL InvalidateRect(HWND hWnd, CONST RECT *lpRect, BOOL bErase) { return 0; }
BOOL AdjustWindowRect(LPRECT lpRect, DWORD dwStyle, BOOL bMenu) { return 0; }
LONG GetWindowLong(HWND hWnd, int nIndex) { return 0; }
HMENU GetMenu(HWND hWnd) { return NULL; }
int GetMenuItemCount(HMENU hMenu) { return 0; }
UINT GetMenuItemID(HMENU hMenu, int nPos) { return 0; }
HMENU GetSubMenu(HMENU hMenu, int nPos) { return NULL; }
int GetMenuString(HMENU hMenu, UINT uIDItem, LPTSTR lpString,int cchMax, UINT flags) { return 0; }
BOOL DeleteMenu(HMENU hMenu, UINT uPosition, UINT uFlags) { return FALSE; }
BOOL InsertMenu(HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCTSTR lpNewItem) { return FALSE; }

BOOL SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags) { return 0; }
BOOL IsRectEmpty(CONST RECT *lprc) { return 0; }
BOOL WINAPI SetWindowOrgEx(HDC hdc, int x, int y, LPPOINT lppt) { return 0; }

// GDI
HGDIOBJ SelectObject(HDC hdc, HGDIOBJ h) {
    if(h) {
        switch (h->handleType) {
            case HGDIOBJ_TYPE_PEN:
                break;
            case HGDIOBJ_TYPE_BRUSH:
                break;
            case HGDIOBJ_TYPE_FONT:
                break;
            case HGDIOBJ_TYPE_BITMAP: {
                //HBITMAP oldSelectedBitmap = hdc->selectedBitmap;
                hdc->selectedBitmap = h;
                return h; //oldSelectedBitmap;
            }
            case HGDIOBJ_TYPE_REGION:
                break;
            case HGDIOBJ_TYPE_PALETTE: {
                //HPALETTE oldSelectedPalette = hdc->selectedPalette;
                hdc->selectedPalette = h;
                return h; //oldSelectedPalette;
            }
            default:
                break;
        }
    }
    return NULL;
}
int GetObject(HGDIOBJ h, int c, LPVOID pv) {
    if(h) {
        switch (h->handleType) {
            case HGDIOBJ_TYPE_PEN:
                break;
            case HGDIOBJ_TYPE_BRUSH:
                break;
            case HGDIOBJ_TYPE_FONT:
                break;
            case HGDIOBJ_TYPE_BITMAP:
                if(h && c == sizeof(BITMAP) && pv) {
                    BITMAP * pBITMAP = (BITMAP *)pv;
                    HBITMAP hBITMAP = (HBITMAP)h;
                    pBITMAP->bmType = 0;
                    pBITMAP->bmWidth = hBITMAP->bitmapInfoHeader->biWidth;
                    pBITMAP->bmHeight = hBITMAP->bitmapInfoHeader->biWidth;
                    pBITMAP->bmWidthBytes = (4 * ((hBITMAP->bitmapInfoHeader->biWidth * hBITMAP->bitmapInfoHeader->biBitCount + 31) / 32));
                    pBITMAP->bmPlanes = hBITMAP->bitmapInfoHeader->biPlanes;
                    pBITMAP->bmBitsPixel = hBITMAP->bitmapInfoHeader->biBitCount;
                    pBITMAP->bmBits = (LPVOID)hBITMAP->bitmapBits;
                    return sizeof(BITMAP);
                }
                break;
            case HGDIOBJ_TYPE_REGION:
                break;
            case HGDIOBJ_TYPE_PALETTE:
                break;
            default:
                break;
        }
    }    return 0;
}
HGDIOBJ GetCurrentObject(HDC hdc, UINT type) {
    //TODO
    return NULL;
}
BOOL DeleteObject(HGDIOBJ ho) {
    switch(ho->handleType) {
        case HGDIOBJ_TYPE_PALETTE: {
            ho->handleType = HGDIOBJ_TYPE_INVALID;
            if(ho->paletteLog)
                free(ho->paletteLog);
            ho->paletteLog = NULL;
            free(ho);
            return TRUE;
        }
        case HGDIOBJ_TYPE_BITMAP: {
            ho->handleType = HGDIOBJ_TYPE_INVALID;
            ho->bitmapInfoHeader = NULL;
            ho->bitmapBits = NULL;
            ho->bitmapInfo = NULL;
            free(ho);
            return TRUE;
        }
        default:
            break;
    }
    return FALSE;
}
HGDIOBJ GetStockObject(int i) {
    //TODO
    return NULL;
}
HPALETTE CreatePalette(CONST LOGPALETTE * plpal) {
    HGDIOBJ handle = (HGDIOBJ)malloc(sizeof(_HGDIOBJ));
    memset(handle, 0, sizeof(_HGDIOBJ));
    handle->handleType = HGDIOBJ_TYPE_PALETTE;
    if(plpal && plpal->palNumEntries >= 0 && plpal->palNumEntries <= 256) {
        size_t structSize = sizeof(LOGPALETTE) + sizeof(PALETTEENTRY) * (size_t)(plpal->palNumEntries - 1);
        handle->paletteLog = malloc(structSize);
        memcpy(handle->paletteLog, plpal, structSize);
    }
    return handle;
}
HPALETTE SelectPalette(HDC hdc, HPALETTE hPal, BOOL bForceBkgd) {
    HPALETTE hOldPal = hdc->selectedPalette;
    hdc->selectedPalette = hPal;
    return hOldPal;
}
UINT RealizePalette(HDC hdc) {
    if(hdc && hdc->selectedPalette) {
        PLOGPALETTE paletteLog = hdc->selectedPalette->paletteLog;
        if (paletteLog) {
            HPALETTE oldRealizedPalette = hdc->realizedPalette;
            hdc->realizedPalette = CreatePalette(paletteLog);
            if(oldRealizedPalette)
                DeleteObject(oldRealizedPalette);
        }
    }
    return 0;
}

// DC

HDC CreateCompatibleDC(HDC hdc) {
    HDC handle = (HDC)malloc(sizeof(struct _HDC));
    memset(handle, 0, sizeof(struct _HDC));
    handle->handleType = HDC_TYPE_DC;
    handle->hdcCompatible = hdc;
    return handle;
}
BOOL DeleteDC(HDC hdc) {
    memset(hdc, 0, sizeof(struct _HDC));
    free(hdc);
    return TRUE;
}
BOOL MoveToEx(HDC hdc, int x, int y, LPPOINT lppt) {
    //TODO
    return 0;
}
BOOL LineTo(HDC hdc, int x, int y) {
    //TODO
    return 0;
}
BOOL PatBlt(HDC hdc, int x, int y, int w, int h, DWORD rop) {
    //TODO
    return 0;
}
BOOL BitBlt(HDC hdc, int x, int y, int cx, int cy, HDC hdcSrc, int x1, int y1, DWORD rop) {
    if(hdcSrc && hdcSrc->selectedBitmap) {
        HBITMAP hBitmap = hdcSrc->selectedBitmap;
        int sourceWidth = hBitmap->bitmapInfoHeader->biWidth;
        int sourceHeight = abs(hBitmap->bitmapInfoHeader->biHeight);
        return StretchBlt(hdc, x, y, cx, cy, hdcSrc, x1, y1, min(cx, sourceWidth), min(cy, sourceHeight), rop);
    }
    return FALSE;
}
int SetStretchBltMode(HDC hdc, int mode) {
    //TODO
    return 0;
}
BOOL StretchBlt(HDC hdcDest, int xDest, int yDest, int wDest, int hDest, HDC hdcSrc, int xSrc, int ySrc, int wSrc, int hSrc, DWORD rop) {
    if((hdcDest->selectedBitmap || hdcDest->hdcCompatible == NULL)
              && hdcSrc->selectedBitmap && hDest && hSrc) {

        HBITMAP hBitmapSource = hdcSrc->selectedBitmap;
        void * pixelsSource = (void *) hBitmapSource->bitmapBits;

        HBITMAP hBitmapDestination = NULL;
        void * pixelsDestination = NULL;

        int sourceWidth = hBitmapSource->bitmapInfoHeader->biWidth;
        int sourceHeight = abs(hBitmapSource->bitmapInfoHeader->biHeight);
        int destinationWidth = 0;
        int destinationHeight = 0;

        int sourceBitCount = hBitmapSource->bitmapInfoHeader->biBitCount;
        int sourceBytes = (sourceBitCount >> 3);
        float sourceBytesWithDecimal = (float)sourceBitCount / 8.0f;
        //TODO float sourceStride = (float)(sourceBytesWithDecimal * ((sourceWidth * hBitmapSource->bitmapInfoHeader->biBitCount + 31) / 32));
        float sourceStride = (float)(4 * ((sourceWidth * hBitmapSource->bitmapInfoHeader->biBitCount + 31) / 32));
        int destinationBytes = 0;
        float destinationStride = 0;

        JNIEnv * jniEnv = NULL;

        if(hdcDest->hdcCompatible == NULL) {
            // We update the main window

            jint ret;
            BOOL needDetach = FALSE;
            ret = (*java_machine)->GetEnv(java_machine, (void **) &jniEnv, JNI_VERSION_1_6);
            if (ret == JNI_EDETACHED) {
                // GetEnv: not attached
                ret = (*java_machine)->AttachCurrentThread(java_machine, &jniEnv, NULL);
                if (ret == JNI_OK) {
                    needDetach = TRUE;
                }
            }

            destinationWidth = androidBitmapInfo.width;
            destinationHeight = androidBitmapInfo.height;

            destinationBytes = 4;
            destinationStride = androidBitmapInfo.stride;

            if ((ret = AndroidBitmap_lockPixels(jniEnv, bitmapMainScreen, &pixelsDestination)) < 0) {
                LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
            }
        } else {
            hBitmapDestination = hdcDest->selectedBitmap;
            pixelsDestination = (void *) hBitmapDestination->bitmapBits;

            destinationWidth = hBitmapDestination->bitmapInfoHeader->biWidth;
            destinationHeight = abs(hBitmapDestination->bitmapInfoHeader->biHeight);

            destinationBytes = (hBitmapDestination->bitmapInfoHeader->biBitCount >> 3);
            destinationStride = (float)(destinationBytes * ((destinationWidth * hBitmapDestination->bitmapInfoHeader->biBitCount + 31) / 32));
        }


        //https://softwareengineering.stackexchange.com/questions/148123/what-is-the-algorithm-to-copy-a-region-of-one-bitmap-into-a-region-in-another
        float src_dx = (float)wSrc / (float)wDest;
        float src_dy = (float)hSrc / (float)hDest;
        float src_maxx = xSrc + wSrc;
        float src_maxy = ySrc + hSrc;
        float dst_maxx = xDest + wDest;
        float dst_maxy = yDest + hDest;
        float src_cury = ySrc;

        //LOGD("StretchBlt(%08x, x:%d, y:%d, w:%d, h:%d, %08x, x:%d, y:%d, w:%d, h:%d) -> sourceBytes: %d", hdcDest->hdcCompatible, xDest, yDest, wDest, hDest, hdcSrc, xSrc, ySrc, wSrc, hSrc, sourceBytesWithDecimal);

//        PALETTEENTRY * palPalEntry = hdcSrc->selectedPalette && hdcSrc->selectedPalette->paletteLog && hdcSrc->selectedPalette->paletteLog->palPalEntry ?
//                                     hdcSrc->selectedPalette->paletteLog->palPalEntry : NULL;
        HPALETTE palette = hdcSrc->realizedPalette;
        if(!palette)
            palette = hdcSrc->selectedPalette;
        PALETTEENTRY * palPalEntry = palette && palette->paletteLog && palette->paletteLog->palPalEntry ?
                                     palette->paletteLog->palPalEntry : NULL;

        for (float y = yDest; y < dst_maxy; y++) {
            float src_curx = xSrc;
            BYTE parity = xSrc;
            for (float x = xDest; x < dst_maxx; x++, parity++) {
                // Point sampling - you can also impl as bilinear or other


                float currentXBytes = sourceBytesWithDecimal * (int)src_curx;
                BYTE * sourcePixel = pixelsSource + (int)(sourceStride * (int)src_cury) + (int)currentXBytes;
                BYTE * destinationPixel = pixelsDestination + (int)(destinationStride * y + 4.0 * x);

                // -> ARGB_8888
                switch (sourceBitCount) {
                    case 4: {
                        BYTE colorIndex = (parity & 0x1 ? sourcePixel[0] & (BYTE)0x0F : sourcePixel[0] >> 4);
                        //BYTE colorIndex = (parity & 0x1 ? sourcePixel[0] >> 4 : sourcePixel[0] & (BYTE)0x0F);
                        if (palPalEntry) {
                            destinationPixel[0] = palPalEntry[colorIndex].peRed; //TODO Exchange Blue and Red?
                            destinationPixel[1] = palPalEntry[colorIndex].peGreen;
                            destinationPixel[2] = palPalEntry[colorIndex].peBlue;
                            destinationPixel[3] = 255;
                        } else {
                            destinationPixel[0] = colorIndex;
                            destinationPixel[1] = colorIndex;
                            destinationPixel[2] = colorIndex;
                            destinationPixel[3] = 255;
                        }
                        break;
                    }
                    case 8: {
                        BYTE colorIndex = sourcePixel[0];
                        if (palPalEntry) {
                            destinationPixel[0] = palPalEntry[colorIndex].peRed; //TODO Exchange Blue and Red?
                            destinationPixel[1] = palPalEntry[colorIndex].peGreen;
                            destinationPixel[2] = palPalEntry[colorIndex].peBlue;
                            destinationPixel[3] = 255;
                        } else {
                            destinationPixel[0] = sourcePixel[0];
                            destinationPixel[1] = sourcePixel[0];
                            destinationPixel[2] = sourcePixel[0];
                            destinationPixel[3] = 255;
                        }
                        break;
                    }
                    case 24:
                        destinationPixel[0] = sourcePixel[2];
                        destinationPixel[1] = sourcePixel[1];
                        destinationPixel[2] = sourcePixel[0];
                        destinationPixel[3] = 255;
                        break;
                    case 32:
                        memcpy(destinationPixel, sourcePixel, (size_t) sourceBytes);
                        break;
                    default:
                        break;
                }

                src_curx += src_dx;
            }

            src_cury += src_dy;
        }

        if(jniEnv)
            AndroidBitmap_unlockPixels(jniEnv, bitmapMainScreen);

        return TRUE;
    }
    return FALSE;

//    if(hdcDest->hdcCompatible == NULL && hdcSrc->selectedBitmap && hDest && hSrc) {
//        // We update the main window
//
//        JNIEnv * jniEnv;
//        jint ret;
//        BOOL needDetach = FALSE;
//        ret = (*java_machine)->GetEnv(java_machine, (void **) &jniEnv, JNI_VERSION_1_6);
//        if (ret == JNI_EDETACHED) {
//            // GetEnv: not attached
//            ret = (*java_machine)->AttachCurrentThread(java_machine, &jniEnv, NULL);
//            if (ret == JNI_OK) {
//                needDetach = TRUE;
//            }
//        }
//
//        void * pixelsDestination;
//        if ((ret = AndroidBitmap_lockPixels(jniEnv, bitmapMainScreen, &pixelsDestination)) < 0) {
//            LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
//        }
//
//        HBITMAP hBitmapSource = hdcSrc->selectedBitmap;
//
//        void * pixelsSource = (void *) hBitmapSource->bitmapBits;
//
//        int sourceWidth = hBitmapSource->bitmapInfoHeader->biWidth;
//        int sourceHeight = abs(hBitmapSource->bitmapInfoHeader->biHeight);
//        int destinationWidth = androidBitmapInfo.width;
//        int destinationHeight = androidBitmapInfo.height;
//
//        //https://softwareengineering.stackexchange.com/questions/148123/what-is-the-algorithm-to-copy-a-region-of-one-bitmap-into-a-region-in-another
//        float src_dx = (float)wSrc / (float)wDest;
//        float src_dy = (float)hSrc / (float)hDest;
//        float src_maxx = xSrc + wSrc;
//        float src_maxy = ySrc + hSrc;
//        float dst_maxx = xDest + wDest;
//        float dst_maxy = yDest + hDest;
//        float src_cury = ySrc;
//
//        int sourceBytes = (hBitmapSource->bitmapInfoHeader->biBitCount >> 3);
////        float sourceStride = sourceWidth * sourceBytes;
////        sourceStride = (float)(4 * ((sourceWidth * hBitmapSource->bitmapInfoHeader->biBitCount + 31) / 32));
//        float sourceStride = (float)(4 * ((sourceWidth * hBitmapSource->bitmapInfoHeader->biBitCount + 31) / 32));
//        float destinationStride = androidBitmapInfo.stride; // Destination always 4 bytes RGBA
//        //LOGD("StretchBlt(%08x, x:%d, y:%d, w:%d, h:%d, %08x, x:%d, y:%d, w:%d, h:%d) -> sourceBytes: %d", hdcDest->hdcCompatible, xDest, yDest, wDest, hDest, hdcSrc, xSrc, ySrc, wSrc, hSrc, sourceBytes);
//
//        HPALETTE palette = hdcSrc->realizedPalette;
//        if(!palette)
//            palette = hdcSrc->selectedPalette;
//        PALETTEENTRY * palPalEntry = palette && palette->paletteLog && palette->paletteLog->palPalEntry ?
//                palette->paletteLog->palPalEntry : NULL;
//
//        for (float y = yDest; y < dst_maxy; y++)
//        {
//            float src_curx = xSrc;
//            for (float x = xDest; x < dst_maxx; x++)
//            {
//                // Point sampling - you can also impl as bilinear or other
//                //dst.bmp[x,y] = src.bmp[src_curx, src_cury];
//
//                BYTE * destinationPixel = pixelsDestination + (int)(4.0 * x + destinationStride * y);
//                BYTE * sourcePixel = pixelsSource + (int)(sourceBytes * (int)src_curx) + (int)(sourceStride * (int)src_cury);
//
//                // -> ARGB_8888
//                switch (sourceBytes) {
//                    case 1:
//                        if(palPalEntry) {
//                            BYTE colorIndex = sourcePixel[0];
//                            destinationPixel[0] = palPalEntry[colorIndex].peRed;
//                            destinationPixel[1] = palPalEntry[colorIndex].peGreen;
//                            destinationPixel[2] = palPalEntry[colorIndex].peBlue;
//                            destinationPixel[3] = 255;
//                        } else {
//                            destinationPixel[0] = sourcePixel[0];
//                            destinationPixel[1] = sourcePixel[0];
//                            destinationPixel[2] = sourcePixel[0];
//                            destinationPixel[3] = 255;
//                        }
//                        break;
//                    case 3:
//                        destinationPixel[0] = sourcePixel[2];
//                        destinationPixel[1] = sourcePixel[1];
//                        destinationPixel[2] = sourcePixel[0];
//                        destinationPixel[3] = 255;
//                        break;
//                    case 4:
//                        memcpy(destinationPixel, sourcePixel, (size_t) sourceBytes);
//                        break;
//                    default:
//                        break;
//                }
//
//                src_curx += src_dx;
//            }
//
//            src_cury += src_dy;
//        }
//
//        AndroidBitmap_unlockPixels(jniEnv, bitmapMainScreen);
//
////        if(needDetach)
////            ret = (*java_machine)->DetachCurrentThread(java_machine);
//
//        //mainViewUpdateCallback();
//        return TRUE;
//    } else if(hdcDest->selectedBitmap && hdcSrc->selectedBitmap && hDest && hSrc) {
//        // We update the main window
//
//        HBITMAP hBitmapSource = hdcSrc->selectedBitmap;
//        void * pixelsSource = (void *) hBitmapSource->bitmapBits;
//
//        HBITMAP hBitmapDestination = hdcDest->selectedBitmap;
//        void * pixelsDestination = (void *) hBitmapDestination->bitmapBits;
//
//        int sourceWidth = hBitmapSource->bitmapInfoHeader->biWidth;
//        int sourceHeight = abs(hBitmapSource->bitmapInfoHeader->biHeight);
//        int destinationWidth = hBitmapDestination->bitmapInfoHeader->biWidth;
//        int destinationHeight = abs(hBitmapDestination->bitmapInfoHeader->biHeight);
//
//        //https://softwareengineering.stackexchange.com/questions/148123/what-is-the-algorithm-to-copy-a-region-of-one-bitmap-into-a-region-in-another
//        float src_dx = (float)wSrc / (float)wDest;
//        float src_dy = (float)hSrc / (float)hDest;
//        float src_maxx = xSrc + wSrc;
//        float src_maxy = ySrc + hSrc;
//        float dst_maxx = xDest + wDest;
//        float dst_maxy = yDest + hDest;
//        float src_cury = ySrc;
//
//        int sourceBytes = (hBitmapSource->bitmapInfoHeader->biBitCount >> 3);
//        //TODO float sourceStride = (float)(sourceBytes * ((sourceWidth * hBitmapSource->bitmapInfoHeader->biBitCount + 31) / 32));
//        float sourceStride = (float)(4 * ((sourceWidth * hBitmapSource->bitmapInfoHeader->biBitCount + 31) / 32));
//        int destinationBytes = (hBitmapDestination->bitmapInfoHeader->biBitCount >> 3);
//        float destinationStride = (float)(destinationBytes * ((destinationWidth * hBitmapDestination->bitmapInfoHeader->biBitCount + 31) / 32));
//        //LOGD("StretchBlt(%08x, x:%d, y:%d, w:%d, h:%d, %08x, x:%d, y:%d, w:%d, h:%d) -> sourceBytes: %d", hdcDest->hdcCompatible, xDest, yDest, wDest, hDest, hdcSrc, xSrc, ySrc, wSrc, hSrc, sourceBytes);
//
//        PALETTEENTRY * palPalEntry = hdcSrc->selectedPalette && hdcSrc->selectedPalette->paletteLog && hdcSrc->selectedPalette->paletteLog->palPalEntry ?
//                                     hdcSrc->selectedPalette->paletteLog->palPalEntry : NULL;
//
//        for (float y = yDest; y < dst_maxy; y++)
//        {
//            float src_curx = xSrc;
//            for (float x = xDest; x < dst_maxx; x++)
//            {
//                // Point sampling - you can also impl as bilinear or other
//                //dst.bmp[x,y] = src.bmp[src_curx, src_cury];
//
//                BYTE * destinationPixel = pixelsDestination + (int)(4.0 * x + destinationStride * y);
//                BYTE * sourcePixel = pixelsSource + (int)(sourceBytes * (int)src_curx) + (int)(sourceStride * (int)src_cury);
//
//                // -> ARGB_8888
//                switch (sourceBytes) {
//                    case 1:
//                        if(palPalEntry) {
//                            BYTE colorIndex = sourcePixel[0];
//                            destinationPixel[0] = palPalEntry[colorIndex].peBlue;
//                            destinationPixel[1] = palPalEntry[colorIndex].peGreen;
//                            destinationPixel[2] = palPalEntry[colorIndex].peRed;
//                            destinationPixel[3] = 255;
//                        } else {
//                            destinationPixel[0] = sourcePixel[0];
//                            destinationPixel[1] = sourcePixel[0];
//                            destinationPixel[2] = sourcePixel[0];
//                            destinationPixel[3] = 255;
//                        }
//                        break;
//                    case 3:
//                        destinationPixel[0] = sourcePixel[2];
//                        destinationPixel[1] = sourcePixel[1];
//                        destinationPixel[2] = sourcePixel[0];
//                        destinationPixel[3] = 255;
//                        break;
//                    case 4:
//                        memcpy(destinationPixel, sourcePixel, (size_t) sourceBytes);
//                        break;
//                    default:
//                        break;
//                }
//
//                src_curx += src_dx;
//            }
//
//            src_cury += src_dy;
//        }
//        return TRUE;
//    }
//    return FALSE;
}
UINT SetDIBColorTable(HDC  hdc, UINT iStart, UINT cEntries, CONST RGBQUAD *prgbq) {
    if(prgbq
       && hdc && hdc->realizedPalette && hdc->realizedPalette->paletteLog && hdc->realizedPalette->paletteLog->palPalEntry
       && hdc->realizedPalette->paletteLog->palNumEntries > 0 && iStart < hdc->realizedPalette->paletteLog->palNumEntries) {
        PALETTEENTRY * palPalEntry = hdc->realizedPalette->paletteLog->palPalEntry;
        for (int i = iStart, j = 0; i < cEntries; i++, j++) {
            palPalEntry[i].peRed = prgbq[j].rgbBlue;
            palPalEntry[i].peGreen = prgbq[j].rgbGreen;
            palPalEntry[i].peBlue = prgbq[j].rgbRed;
            palPalEntry[i].peFlags = 0;
        }
    }
    return 0;
}
HBITMAP CreateDIBitmap( HDC hdc, CONST BITMAPINFOHEADER *pbmih, DWORD flInit, CONST VOID *pjBits, CONST BITMAPINFO *pbmi, UINT iUsage) {
    HGDIOBJ newHBITMAP = (HGDIOBJ)malloc(sizeof(_HGDIOBJ));
    memset(newHBITMAP, 0, sizeof(_HGDIOBJ));
    newHBITMAP->handleType = HGDIOBJ_TYPE_BITMAP;
    BITMAPINFO * newBitmapInfo = malloc(sizeof(BITMAPINFO));
    memcpy(newBitmapInfo, pbmi, sizeof(BITMAPINFO));
    newHBITMAP->bitmapInfo = newBitmapInfo;
    newHBITMAP->bitmapInfoHeader = (BITMAPINFOHEADER *)newBitmapInfo;
    //size_t stride = (size_t)(newBitmapInfo->bmiHeader.biWidth * (newBitmapInfo->bmiHeader.biBitCount >> 3));
    size_t stride = (size_t)(4 * ((newBitmapInfo->bmiHeader.biWidth * newBitmapInfo->bmiHeader.biBitCount + 31) / 32));
    size_t size = newBitmapInfo->bmiHeader.biSizeImage ?
                    newBitmapInfo->bmiHeader.biSizeImage :
                    newBitmapInfo->bmiHeader.biHeight * stride;
    VOID * bitmapBits = malloc(size);
    //memcpy(bitmapBits, pjBits, size);
    // Switch Y
    VOID * bitmapBitsSource = (VOID *)pjBits;
    VOID * bitmapBitsDestination = bitmapBits + (newBitmapInfo->bmiHeader.biHeight - 1) * stride;
    for(int y = 0; y < newBitmapInfo->bmiHeader.biHeight; y++) {
        memcpy(bitmapBitsDestination, bitmapBitsSource, stride);
        bitmapBitsSource += stride;
        bitmapBitsDestination -= stride;
    }
    newHBITMAP->bitmapBits = bitmapBits;
    return newHBITMAP;
}
HBITMAP CreateDIBSection(HDC hdc, CONST BITMAPINFO *pbmi, UINT usage, VOID **ppvBits, HANDLE hSection, DWORD offset) {
    HGDIOBJ newHBITMAP = (HGDIOBJ)malloc(sizeof(_HGDIOBJ));
    memset(newHBITMAP, 0, sizeof(_HGDIOBJ));
    newHBITMAP->handleType = HGDIOBJ_TYPE_BITMAP;
    newHBITMAP->bitmapInfo = pbmi;
    newHBITMAP->bitmapInfoHeader = (const BITMAPINFOHEADER *) pbmi;
    // For DIB_RGB_COLORS only
    size_t size = (size_t) (pbmi->bmiHeader.biWidth * abs(pbmi->bmiHeader.biHeight) * 4);
    newHBITMAP->bitmapBits = malloc(size);
    memset((void *) newHBITMAP->bitmapBits, 0, size);
    *ppvBits = (void *) newHBITMAP->bitmapBits;
    return newHBITMAP;
}
HBITMAP CreateCompatibleBitmap( HDC hdc, int cx, int cy) {
    HGDIOBJ newHBITMAP = (HGDIOBJ)malloc(sizeof(_HGDIOBJ));
    memset(newHBITMAP, 0, sizeof(_HGDIOBJ));
    newHBITMAP->handleType = HGDIOBJ_TYPE_BITMAP;

    BITMAPINFO * newBitmapInfo = malloc(sizeof(BITMAPINFO));
    memset(newBitmapInfo, 0, sizeof(BITMAPINFO));
    newHBITMAP->bitmapInfo = newBitmapInfo;
    newHBITMAP->bitmapInfoHeader = (BITMAPINFOHEADER *)newBitmapInfo;

    newBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    newBitmapInfo->bmiHeader.biWidth = cx;
    newBitmapInfo->bmiHeader.biHeight = cy;
    newBitmapInfo->bmiHeader.biBitCount = 32;

    size_t stride = (size_t)(4 * ((newBitmapInfo->bmiHeader.biWidth * newBitmapInfo->bmiHeader.biBitCount + 31) / 32));
    size_t size = newBitmapInfo->bmiHeader.biHeight * stride;
    newBitmapInfo->bmiHeader.biSizeImage = (DWORD) size;
    newHBITMAP->bitmapBits = malloc(size);
    memset((void *) newHBITMAP->bitmapBits, 0, size);
    return newHBITMAP;
}
int GetDIBits(HDC hdc, HBITMAP hbm, UINT start, UINT cLines, LPVOID lpvBits, LPBITMAPINFO lpbmi, UINT usage) {
    //TODO
    return 0;
}
BOOL SetRect(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom) {
    //TODO
    return 0;
}
int SetWindowRgn(HWND hWnd, HRGN hRgn, BOOL bRedraw) {
    //TODO
    return 0;
}
HRGN ExtCreateRegion(CONST XFORM * lpx, DWORD nCount, CONST RGNDATA * lpData) {
    //TODO
    return NULL;
}
BOOL GdiFlush(void) {
    mainViewUpdateCallback();
    return 0;
}
HDC BeginPaint(HWND hWnd, LPPAINTSTRUCT lpPaint) {
    //hWindowDC;
//    if(!mainPaintDC)
//        mainPaintDC = CreateCompatibleDC(NULL);
    if(lpPaint) {
        memset(lpPaint, 0, sizeof(PAINTSTRUCT));
        lpPaint->fErase = TRUE;
        lpPaint->hdc = hWindowDC; //mainPaintDC;
        lpPaint->rcPaint.right = (short) nBackgroundW; //androidBitmapInfo.width; // - 1;
        lpPaint->rcPaint.bottom = (short) nBackgroundH; //androidBitmapInfo.height; // - 1;
    }
    return hWindowDC; //mainPaintDC;
}
BOOL EndPaint(HWND hWnd, CONST PAINTSTRUCT *lpPaint) {
    mainViewUpdateCallback();
    return 0;
}

// Window

BOOL WINAPI MessageBeep(UINT uType) {
    //TODO System beep
    return 1;
}

BOOL WINAPI OpenClipboard(HWND hWndNewOwner) {
    return TRUE;
}
BOOL WINAPI CloseClipboard(VOID) {
    //TODO
    return 0;
}

BOOL WINAPI EmptyClipboard(VOID) {
    return TRUE;
}

HANDLE WINAPI SetClipboardData(UINT uFormat,HANDLE hMem) {
    if(CF_TEXT) {
        clipboardCopyText((const TCHAR *)hMem);
    }
    GlobalFree(hMem);
    return NULL;
}

BOOL WINAPI IsClipboardFormatAvailable(UINT format) {
    TCHAR * szText = clipboardPasteText();
    BOOL result = szText != NULL;
    GlobalFree(szText);
    return result;
}

HANDLE WINAPI GetClipboardData(UINT uFormat) {
    TCHAR * szText = clipboardPasteText();
    return szText;
}

void deleteTimeEvent(UINT uTimerID) {
    timer_delete(timerEvents[uTimerID - 1].timer);
    timerEvents[uTimerID - 1].valid = FALSE;
}
void timerCallback(int timerId) {
    if(timerId >= 0 && timerId < MAX_TIMER && timerEvents[timerId].valid) {
        timerEvents[timerId].fptc((UINT) (timerId + 1), 0, (DWORD) timerEvents[timerId].dwUser, 0, 0);

        if(timerEvents[timerId].fuEvent == TIME_ONESHOT) {
            //LOGD("timerCallback remove timer uTimerID [%d]", timerId + 1);
            deleteTimeEvent((UINT) (timerId + 1));
        }
    }
}
MMRESULT timeSetEvent(UINT uDelay, UINT uResolution, LPTIMECALLBACK fptc, DWORD_PTR dwUser, UINT fuEvent) {
    //LOGD("timeSetEvent(uDelay: %d, fuEvent: %d)", uDelay, fuEvent);

    // Find a timer id
    int timerId = -1;
    for (int i = 0; i < MAX_TIMER; ++i) {
        if(!timerEvents[i].valid) {
            timerId = i;
            break;
        }
    }
    if(timerId == -1) {
        //LOGD("timeSetEvent() ERROR: No more timer available");
        return NULL;
    }
    timerEvents[timerId].timerId = timerId;
    timerEvents[timerId].fptc = fptc;
    timerEvents[timerId].dwUser = dwUser;
    timerEvents[timerId].fuEvent = fuEvent;


    struct sigevent sev;
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = (void (*)(sigval_t)) timerCallback; //this function will be called when timer expires
    sev.sigev_value.sival_int = timerEvents[timerId].timerId; //this argument will be passed to cbf
    sev.sigev_notify_attributes = NULL;
    timer_t * timer = &(timerEvents[timerId].timer);
    if (timer_create(CLOCK_REALTIME, &sev, timer) == -1) {
        //LOGD("timeSetEvent() ERROR in timer_create");
        return NULL;
    }

    long long freq_nanosecs = uDelay * 1000000;
    struct itimerspec its;
    its.it_value.tv_sec = freq_nanosecs / 1000000000;
    its.it_value.tv_nsec = freq_nanosecs % 1000000000;
    if(fuEvent == TIME_PERIODIC) {
        its.it_interval.tv_sec = its.it_value.tv_sec;
        its.it_interval.tv_nsec = its.it_value.tv_nsec;
    } else /*if(fuEvent == TIME_ONESHOT)*/ {
        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 0;
    }
    if (timer_settime(timerEvents[timerId].timer, 0, &its, NULL) == -1) {
        timer_delete(timerEvents[timerId].timer);
        //LOGD("timeSetEvent() ERROR in timer_settime");
        return NULL;
    }
    timerEvents[timerId].valid = TRUE;
    //LOGD("timeSetEvent() -> timerId+1: [%d]", timerId + 1);
    return (MMRESULT) (timerId + 1); // No error
}
MMRESULT timeKillEvent(UINT uTimerID) {
    //LOGD("timeKillEvent(uTimerID: [%d])", uTimerID);
    deleteTimeEvent(uTimerID);
    return 0; //No error
}

MMRESULT timeGetDevCaps(LPTIMECAPS ptc, UINT cbtc) {
    if(ptc) {
        ptc->wPeriodMin = 1; // ms
        ptc->wPeriodMax = 1000000; // ms -> 1000s
    }
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
    if(lpSystemTime) {
        struct timespec ts = {0, 0};
        struct tm tm = {};
        clock_gettime(CLOCK_REALTIME, &ts);
        time_t tim = ts.tv_sec;
        localtime_r(&tim, &tm);
        lpSystemTime->wYear = 1900 + tm.tm_year;
        lpSystemTime->wMonth = 1 + tm.tm_mon;
        lpSystemTime->wDayOfWeek = tm.tm_wday;
        lpSystemTime->wDay = tm.tm_mday;
        lpSystemTime->wHour = tm.tm_hour;
        lpSystemTime->wMinute = tm.tm_min;
        lpSystemTime->wSecond = tm.tm_sec;
        lpSystemTime->wMilliseconds = ts.tv_nsec / 1e6;
    }
    return;
}
WORD GetTickCount(VOID) {
    //TODO
    return 0;
}


BOOL EnableWindow(HWND hWnd, BOOL bEnable) {
    //TODO
    return 0;
}
HWND GetDlgItem(HWND hDlg, int nIDDlgItem) {
    //TODO
    return NULL;
}
UINT GetDlgItemTextA(HWND hDlg, int nIDDlgItem, LPSTR lpString,int cchMax) {
    //TODO
    return 0;
}

extern TCHAR szKmlLog[10240];
extern TCHAR szKmlTitle[10240];

BOOL SetDlgItemText(HWND hDlg, int nIDDlgItem, LPCTSTR lpString) {
    if(nIDDlgItem == IDC_KMLLOG) {
        //LOGD("KML log:\r\n%s", lpString);
        _tcsncpy(szKmlLog, lpString, sizeof(szKmlLog)/sizeof(TCHAR));
    } if(nIDDlgItem == IDC_TITLE) {
        _tcsncpy(szKmlTitle, lpString, sizeof(szKmlTitle)/sizeof(TCHAR));
    }
    //TODO
    return 0;
}
LRESULT SendDlgItemMessage(HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam) {
    //TODO
    return 0;
}
BOOL CheckDlgButton(HWND hDlg, int nIDButton, UINT uCheck) {
    //TODO
    return 0;
}
UINT IsDlgButtonChecked(HWND hDlg, int nIDButton) {
    //TODO
    return 0;
}
BOOL EndDialog(HWND hDlg, INT_PTR nResult) {
    //TODO
    return 0;
}
HANDLE  FindFirstFileA(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData) {
    //TODO
    return INVALID_HANDLE_VALUE;
}
BOOL FindNextFileA(HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData) {
    //TODO
    return 0;
}
BOOL FindClose(HANDLE hFindFile) {
    //TODO
    return 0;
}
BOOL SHGetPathFromIDListA(PCIDLIST_ABSOLUTE pidl, LPSTR pszPath) {
    //TODO
    return 0;
}
HRESULT SHGetMalloc(IMalloc **ppMalloc) {
    //TODO
    return 0;
}
PIDLIST_ABSOLUTE SHBrowseForFolderA(LPBROWSEINFOA lpbi) {
    //TODO
    return NULL;
}
INT_PTR DialogBoxParam(HINSTANCE hInstance, LPCTSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam) {
    //TODO
    if(lpTemplateName == MAKEINTRESOURCE(IDD_CHOOSEKML)) {
        if(chooseCurrentKmlMode == ChooseKmlMode_UNKNOWN) {
        } else if(chooseCurrentKmlMode == ChooseKmlMode_FILE_NEW) {
            lstrcpy(szCurrentKml, szChosenCurrentKml);
        } else if(chooseCurrentKmlMode == ChooseKmlMode_FILE_OPEN) {
            if(getFirstKMLFilenameForType(Chipset.type, szCurrentKml, sizeof(szCurrentKml) / sizeof(szCurrentKml[0])))
                showAlert(_T("Cannot find the KML template file, so, try another one."), 0);
            else
                showAlert(_T("Cannot find the KML template file, sorry."), 0);
        }
    } else if(lpTemplateName == MAKEINTRESOURCE(IDD_KMLLOG)) {
        lpDialogFunc(NULL, WM_INITDIALOG, 0, 0);
    }
    return NULL;
}
HCURSOR SetCursor(HCURSOR hCursor) {
    //TODO
    return NULL;
}
int MulDiv(int nNumber, int nNumerator, int nDenominator) {
    //TODO
    return 0;
}

BOOL GetKeyboardLayoutName(LPSTR pwszKLID) {
    //TODO
    return 0;
}

void DragAcceptFiles(HWND hWnd, BOOL fAccept) {
    //TODO
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
void _wmakepath(wchar_t _Buffer, wchar_t const* _Drive, wchar_t const* _Dir, wchar_t const* _Filename, wchar_t const* _Ext)
{
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

void _makepath(char _Buffer, char const* _Drive, char const* _Dir, char const* _Filename, char const* _Ext)
{
}

#endif // !UNICODE




BOOL GetClientRect(HWND hWnd, LPRECT lpRect)
{
    return 0;
}