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

#include "core/pch.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <pthread.h>
#include <semaphore.h>
#include <android/bitmap.h>
#include "core/resource.h"
#include "win32-layer.h"
#include "emu.h"
#include "core/lodepng.h"


extern JavaVM *java_machine;
extern jobject bitmapMainScreen;
extern AndroidBitmapInfo androidBitmapInfo;

HANDLE hWnd;
LPTSTR szTitle;
LPTSTR szCurrentAssetDirectory = NULL;
LPTSTR szCurrentContentDirectory = NULL;
AAssetManager * assetManager;
const TCHAR * assetsPrefix = _T("assets/");
size_t assetsPrefixLength;
const TCHAR * contentScheme = _T("content://");
size_t contentSchemeLength;
const TCHAR * documentScheme = _T("document:");


DWORD GetLastError(VOID) {
    //TODO
    return NO_ERROR;
}


static void initTimer();

#define MAX_FILE_MAPPING_HANDLE 10
static HANDLE fileMappingHandles[MAX_FILE_MAPPING_HANDLE];

void win32Init() {
    initTimer();
    for (int i = 0; i < MAX_FILE_MAPPING_HANDLE; ++i) {
        fileMappingHandles[i] = NULL;
    }

    assetsPrefixLength = _tcslen(assetsPrefix);
    contentSchemeLength = _tcslen(contentScheme);
}

int abs (int i) {
    return i < 0 ? -i : i;
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
    szCurrentContentDirectory = NULL;
    szCurrentAssetDirectory = NULL;

    if(path == NULL)
        return FALSE;

    if(_tcsncmp(path, contentScheme, contentSchemeLength) == 0) {
        szCurrentContentDirectory = (LPTSTR) path;
        return TRUE;
    } else if(_tcsncmp(path, assetsPrefix, assetsPrefixLength) == 0) {
        szCurrentAssetDirectory = (LPTSTR) (path + assetsPrefixLength * sizeof(TCHAR));
        return TRUE;
    } else {
        return (BOOL) chdir(path);
    }
}

extern BOOL settingsPort2en;
extern BOOL settingsPort2wr;

HANDLE CreateFile(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPVOID lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, LPVOID hTemplateFile)
{
    FILE_LOGD("CreateFile(lpFileName: \"%s\", dwDesiredAccess: 0x%08x)", lpFileName, dwShareMode);
    BOOL forceNormalFile = FALSE;
    securityExceptionOccured = FALSE;
#if EMUXX == 48
    if(_tcscmp(lpFileName, szPort2Filename) == 0) {
        // Special case for Port2 filename
        forceNormalFile = TRUE;
        if(!settingsPort2wr && (dwDesiredAccess & GENERIC_WRITE))
            return (HANDLE) INVALID_HANDLE_VALUE;
    }
#endif

    TCHAR * foundDocumentScheme = _tcsstr(lpFileName, documentScheme);

    if(chooseCurrentKmlMode == ChooseKmlMode_FILE_OPEN || chooseCurrentKmlMode == ChooseKmlMode_CHANGE_KML) {
        // When we open a new E48 state document
        TCHAR * fileExtension = _tcsrchr(lpFileName, _T('.'));
        if(fileExtension && ((fileExtension[1] == 'K' && fileExtension[2] == 'M' && fileExtension[3] == 'L') ||
                (fileExtension[1] == 'k' && fileExtension[2] == 'm' && fileExtension[3] == 'l')
        )) {
            // And opening a KML file
            if(lpFileName[0] == '/') {
                // With a recorded standard file
                _tcscpy(szEmuDirectory, lpFileName);
                TCHAR * filename = _tcsrchr(szEmuDirectory, _T('/'));
                if(filename) {
                    *filename = _T('\0');
                }
#if EMUXX == 48
                _tcscpy(szRomDirectory, szEmuDirectory);
#endif
                SetCurrentDirectory(szEmuDirectory);
            } else if(foundDocumentScheme) {
                // With a recorded "document:" scheme, extract the folder URL with content:// scheme
                _tcscpy(szEmuDirectory, lpFileName + _tcslen(documentScheme) * sizeof(TCHAR));
                TCHAR * filename = _tcschr(szEmuDirectory, _T('|'));
                if(filename) {
                    *filename = _T('\0');
                }
#if EMUXX == 48
                _tcscpy(szRomDirectory, szEmuDirectory);
#endif
                SetCurrentDirectory(szEmuDirectory);
            } else {
                _tcscpy(szEmuDirectory, "assets/calculators/");
#if EMUXX == 48
                _tcscpy(szRomDirectory, "assets/calculators/");
#endif
                SetCurrentDirectory(szEmuDirectory);
            }
        }
    }

    if(!forceNormalFile && (szCurrentAssetDirectory || _tcsncmp(lpFileName, assetsPrefix, assetsPrefixLength) == 0) && foundDocumentScheme == NULL) {
        // Asset file
        TCHAR szFileName[MAX_PATH];
        AAsset * asset = NULL;
        szFileName[0] = _T('\0');
        if(szCurrentAssetDirectory) {
            _tcscpy(szFileName, szCurrentAssetDirectory);
            _tcscat(szFileName, lpFileName);
            asset = AAssetManager_open(assetManager, szFileName, AASSET_MODE_STREAMING);
        } else {
            asset = AAssetManager_open(assetManager, lpFileName + assetsPrefixLength * sizeof(TCHAR), AASSET_MODE_STREAMING);
        }
        if(asset) {
            HANDLE handle = malloc(sizeof(struct _HANDLE));
            memset(handle, 0, sizeof(struct _HANDLE));
            handle->handleType = HANDLE_TYPE_FILE_ASSET;
            handle->fileAsset = asset;
            return handle;
        }
    } else {
        // Normal file
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

        if(foundDocumentScheme) {
            TCHAR * filename = _tcsrchr(lpFileName, _T('|'));
            if(filename)
                lpFileName = filename + 1;
        }

        TCHAR * urlContentSchemeFound = _tcsstr(lpFileName, contentScheme);
        if(urlContentSchemeFound) {
            // Case of an absolute file with the scheme content://
            fd = openFileFromContentResolver(lpFileName, dwDesiredAccess);
            useOpenFileFromContentResolver = TRUE;
            if(fd < 0) {
                FILE_LOGD("CreateFile() openFileFromContentResolver() %d", errno);
                if(fd == -2)
                    securityExceptionOccured = TRUE;
            }
        } else if(szCurrentContentDirectory) {
            // Case of a relative file to a folder with the scheme content://
            fd = openFileInFolderFromContentResolver(lpFileName, szCurrentContentDirectory, dwDesiredAccess);
            useOpenFileFromContentResolver = TRUE;
            if(fd == -1) {
                FILE_LOGD("CreateFile() openFileFromContentResolver() %d", errno);
            }
        } else {
            TCHAR * urlFileSchemeFound = _tcsstr(lpFileName, _T("file://"));
            if(urlFileSchemeFound)
                lpFileName = urlFileSchemeFound + 7;
            fd = open(lpFileName, flags, perm);
            if(fd == -1) {
                FILE_LOGD("CreateFile() open() %d", errno);
            }
        }
        if (fd >= 0) {
            HANDLE handle = malloc(sizeof(struct _HANDLE));
            memset(handle, 0, sizeof(struct _HANDLE));
            handle->handleType = HANDLE_TYPE_FILE;
            handle->fileDescriptor = fd;
            handle->fileOpenFileFromContentResolver = useOpenFileFromContentResolver;
            return handle;
        }
    }
    FILE_LOGD("CreateFile() INVALID_HANDLE_VALUE");
    return (HANDLE) INVALID_HANDLE_VALUE;
}

BOOL ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped) {
    FILE_LOGD("ReadFile(hFile: %p, lpBuffer: 0x%08x, nNumberOfBytesToRead: %d)", hFile, lpBuffer, nNumberOfBytesToRead);
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
    FILE_LOGD("WriteFile(hFile: %p, lpBuffer: 0x%08x, nNumberOfBytesToWrite: %d)", hFile, lpBuffer, nNumberOfBytesToWrite);
    if(hFile->handleType == HANDLE_TYPE_FILE_ASSET)
        return FALSE;
    ssize_t writenByteCount = write(hFile->fileDescriptor, lpBuffer, nNumberOfBytesToWrite);
    if(lpNumberOfBytesWritten)
        *lpNumberOfBytesWritten = (DWORD) writenByteCount;
    return writenByteCount >= 0;
}

DWORD SetFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod) {
    FILE_LOGD("SetFilePointer(hFile: %p, lDistanceToMove: %d, dwMoveMethod: %d)", hFile, lDistanceToMove, dwMoveMethod);
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
    FILE_LOGD("SetEndOfFile(hFile: %p)", hFile);
    if(hFile->handleType == HANDLE_TYPE_FILE_ASSET)
        return FALSE;
    off_t currentPosition = lseek(hFile->fileDescriptor, 0, SEEK_CUR);
    int truncateResult = ftruncate(hFile->fileDescriptor, currentPosition);
    return truncateResult == 0;
}

DWORD GetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh) {
    FILE_LOGD("GetFileSize(hFile: %p)", hFile);
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
    FILE_LOGD("CreateFileMapping(hFile: %p, flProtect: 0x08x, dwMaximumSizeLow: %d)", hFile, flProtect, dwMaximumSizeLow);
    HANDLE handle = malloc(sizeof(struct _HANDLE));
    memset(handle, 0, sizeof(struct _HANDLE));
    if(hFile->handleType == HANDLE_TYPE_FILE) {
        if(hFile->fileOpenFileFromContentResolver)
            handle->handleType = HANDLE_TYPE_FILE_MAPPING_CONTENT;
        else
            handle->handleType = HANDLE_TYPE_FILE_MAPPING;
        handle->fileDescriptor = hFile->fileDescriptor;
    } else if(hFile->handleType == HANDLE_TYPE_FILE_ASSET) {
        handle->handleType = HANDLE_TYPE_FILE_MAPPING_ASSET;
        handle->fileAsset = hFile->fileAsset;
    }
    if(dwMaximumSizeHigh == 0 && dwMaximumSizeLow == 0) {
        dwMaximumSizeLow = GetFileSize(hFile, &dwMaximumSizeHigh);
    }
    handle->fileMappingSize = /*(dwMaximumSizeHigh << 32) |*/ dwMaximumSizeLow;
    handle->fileMappingAddress = NULL;
    handle->fileMappingProtect = flProtect;
    return handle;
}

//https://msdn.microsoft.com/en-us/library/Aa366761(v=VS.85).aspx
LPVOID MapViewOfFile(HANDLE hFileMappingObject, DWORD dwDesiredAccess, DWORD dwFileOffsetHigh, DWORD dwFileOffsetLow, SIZE_T dwNumberOfBytesToMap) {
    FILE_LOGD("MapViewOfFile(hFileMappingObject: %p, dwDesiredAccess: 0x%08x, dwFileOffsetLow: %d)", hFileMappingObject, dwDesiredAccess, dwFileOffsetLow);
    hFileMappingObject->fileMappingOffset = /*(dwFileOffsetHigh << 32) |*/ dwFileOffsetLow;
    LPVOID result = NULL;
    if(hFileMappingObject->handleType == HANDLE_TYPE_FILE_MAPPING) {
        int prot = PROT_NONE;
        if (dwDesiredAccess & FILE_MAP_READ)
            prot |= PROT_READ;
        if (dwDesiredAccess & FILE_MAP_WRITE)
            prot |= PROT_WRITE;
        hFileMappingObject->fileMappingAddress = mmap(NULL, hFileMappingObject->fileMappingSize,
                                                      prot, MAP_PRIVATE,
                                                      hFileMappingObject->fileDescriptor, hFileMappingObject->fileMappingOffset);
    } else if(hFileMappingObject->handleType == HANDLE_TYPE_FILE_MAPPING_CONTENT) {
        size_t numberOfBytesToRead = hFileMappingObject->fileMappingSize - hFileMappingObject->fileMappingOffset;
        hFileMappingObject->fileMappingAddress = malloc(numberOfBytesToRead);
        off_t currentPosition = lseek(hFileMappingObject->fileDescriptor, 0, SEEK_CUR);
        lseek(hFileMappingObject->fileDescriptor, hFileMappingObject->fileMappingOffset, SEEK_SET);
        DWORD readByteCount = (DWORD) read(hFileMappingObject->fileDescriptor, hFileMappingObject->fileMappingAddress, numberOfBytesToRead);
        lseek(hFileMappingObject->fileDescriptor, currentPosition, SEEK_SET);
    } else if(hFileMappingObject->handleType == HANDLE_TYPE_FILE_MAPPING_ASSET) {
        if (dwDesiredAccess & FILE_MAP_WRITE)
            return NULL;
        hFileMappingObject->fileMappingAddress = (LPVOID) (AAsset_getBuffer(hFileMappingObject->fileAsset) + hFileMappingObject->fileMappingOffset);
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
    FILE_LOGD("UnmapViewOfFile(lpBaseAddress: %p)", lpBaseAddress);
    int result = -1;
    for (int i = 0; i < MAX_FILE_MAPPING_HANDLE; ++i) {
        HANDLE fileMappingHandle = fileMappingHandles[i];
        if(fileMappingHandle && lpBaseAddress == fileMappingHandle->fileMappingAddress) {
            // Only save the file manually (for Port2 actually)
            fileMappingHandles[i] = NULL;
            break;
        }
    }

    return result == 0;
}

// This is not a Win32 function
BOOL SaveMapViewToFile(LPCVOID lpBaseAddress) {
    FILE_LOGD("SaveMapViewToFile(lpBaseAddress: %p)", lpBaseAddress);
    int result = -1;
    for (int i = 0; i < MAX_FILE_MAPPING_HANDLE; ++i) {
        HANDLE fileMappingHandle = fileMappingHandles[i];
        if(fileMappingHandle && lpBaseAddress == fileMappingHandle->fileMappingAddress) {
            if(fileMappingHandle->handleType == HANDLE_TYPE_FILE_MAPPING
            || fileMappingHandle->handleType == HANDLE_TYPE_FILE_MAPPING_CONTENT) {
                //size_t numberOfBytesToWrite = fileMappingHandle->fileMappingSize - fileMappingHandle->fileMappingOffset;
                size_t numberOfBytesToWrite = fileMappingHandle->fileMappingSize;
                if(numberOfBytesToWrite > 0) {
                    off_t currentPosition = lseek(fileMappingHandle->fileDescriptor, 0, SEEK_CUR);
                    //lseek(fileMappingHandle->fileDescriptor, fileMappingHandle->fileMappingOffset, SEEK_SET);
                    lseek(fileMappingHandle->fileDescriptor, 0, SEEK_SET);
                    write(fileMappingHandle->fileDescriptor, fileMappingHandle->fileMappingAddress, numberOfBytesToWrite);
                    lseek(fileMappingHandle->fileDescriptor, currentPosition, SEEK_SET);
                } else {
                    // BUG
                }
            } else if(fileMappingHandle->handleType == HANDLE_TYPE_FILE_MAPPING_ASSET) {
                // No need to unmap
                result = 0;
            }
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
                THREAD_LOGD("UnlockedWaitForEvent() pthread_cond_timedwait() for event %x return %d", hHandle, result);
                if(result == ETIMEDOUT)
                    result = WAIT_TIMEOUT;
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

extern int jniDetachCurrentThread();

// Should be protected by mutex
#define MAX_CREATED_THREAD 30
static HANDLE threads[MAX_CREATED_THREAD];

static DWORD ThreadStart(LPVOID lpThreadParameter) {
    HANDLE handle = (HANDLE)lpThreadParameter;
    if(handle) {
        handle->threadStartAddress(handle->threadParameter);
    }

    threads[handle->threadIndex] = NULL;
    jniDetachCurrentThread();
    CloseHandle(handle->threadEventMessage);
}

HANDLE CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId) {
    THREAD_LOGD("CreateThread()");
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
    for(int i = 0; i < MAX_CREATED_THREAD; i++) {
        if(threads[i] == NULL) {
            handle->threadIndex = i;
            threads[handle->threadIndex] = handle;
            break;
        }
    }

    //Suspended
    //https://stackoverflow.com/questions/3140867/suspend-pthreads-without-using-condition
    //https://stackoverflow.com/questions/7953917/create-thread-in-suspended-mode-using-pthreads
    //http://man7.org/linux/man-pages/man3/pthread_create.3.html
    int pthreadResult = pthread_create(&handle->threadId, &attr, (void *(*)(void *)) ThreadStart, handle);
    if(pthreadResult == 0) {
        if(lpThreadId)
            *lpThreadId = (DWORD) handle->threadIndex; //handle->threadId;
        THREAD_LOGD("CreateThread() threadId: 0x%lx, threadIndex: %d", handle->threadId, handle->threadIndex);
        return handle;
    } else {
        threads[handle->threadIndex] = NULL;
        if(handle->threadEventMessage)
            CloseHandle(handle->threadEventMessage);
        free(handle);
    }
    return NULL;
}

DWORD ResumeThread(HANDLE hThread) {
    THREAD_LOGD("ResumeThread()");
    //TODO
    return 0;
}

BOOL SetThreadPriority(HANDLE hThread, int nPriority) {
    THREAD_LOGD("SetThreadPriority()");
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
    FILE_LOGD("CloseHandle(hObject: %p)", hObject);
    //https://msdn.microsoft.com/en-us/9b84891d-62ca-4ddc-97b7-c4c79482abd9
    // Can be a thread/event/file handle!
    switch(hObject->handleType) {
        case HANDLE_TYPE_FILE: {
            FILE_LOGD("CloseHandle() HANDLE_TYPE_FILE");
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
            FILE_LOGD("CloseHandle() HANDLE_TYPE_FILE_ASSET");
            AAsset_close(hObject->fileAsset);
            hObject->handleType = HANDLE_TYPE_INVALID;
            hObject->fileAsset = NULL;
            free(hObject);
            return TRUE;
        }
        case HANDLE_TYPE_FILE_MAPPING:
        case HANDLE_TYPE_FILE_MAPPING_CONTENT:
        case HANDLE_TYPE_FILE_MAPPING_ASSET: {
            FILE_LOGD("CloseHandle() HANDLE_TYPE_FILE_MAPPING");
            hObject->handleType = HANDLE_TYPE_INVALID;
            hObject->fileDescriptor = 0;
            hObject->fileAsset = NULL;
            hObject->fileMappingSize = 0;
            hObject->fileMappingAddress = NULL;
            free(hObject);
            return TRUE;
        }
        case HANDLE_TYPE_EVENT: {
            FILE_LOGD("CloseHandle() HANDLE_TYPE_EVENT");
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
            THREAD_LOGD("CloseHandle() THREAD 0x%lx", hObject->threadId);
            if(hObject->threadEventMessage && hObject->threadEventMessage->handleType == HANDLE_TYPE_EVENT) {
                CloseHandle(hObject->threadEventMessage);
                hObject->threadEventMessage = NULL;
            }
            hObject->handleType = HANDLE_TYPE_INVALID;
            hObject->threadId = 0;
            hObject->threadStartAddress = NULL;
            hObject->threadParameter = NULL;
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
    l->QuadPart = 10000000;
    return TRUE;
}

BOOL QueryPerformanceCounter(PLARGE_INTEGER l)
{
    struct timespec /*{
        time_t   tv_sec;
        long     tv_nsec;
    } */ time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    l->QuadPart = (uint64_t) ((1e9 * time.tv_sec + time.tv_nsec) / 100);
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
    if(openFilename) {
        if(currentDialogBoxMode == DialogBoxMode_OPEN_MACRO) {
            openFilename->nMaxFile = MAX_PATH;
            openFilename->nFileExtension = 1;
            //openFilename->lpstrFile = getSaveObjectFilenameResult;
            _tcsncpy(openFilename->lpstrFile, getSaveObjectFilenameResult, openFilename->nMaxFile);
            return TRUE;
        }
    }
    return FALSE;
}

TCHAR getSaveObjectFilenameResult[MAX_PATH];
BOOL GetSaveFileName(LPOPENFILENAME openFilename) {
    if(openFilename) {
        if(currentDialogBoxMode == DialogBoxMode_SET_USRPRG32
        || currentDialogBoxMode == DialogBoxMode_SET_USRPRG42
        || currentDialogBoxMode == DialogBoxMode_SAVE_MACRO) {
            openFilename->nMaxFile = MAX_PATH;
            openFilename->nFileExtension = 1;
            //openFilename->lpstrFile = getSaveObjectFilenameResult;
            _tcsncpy(openFilename->lpstrFile, getSaveObjectFilenameResult, openFilename->nMaxFile);
            return TRUE;
        }
    }
    return FALSE;
}

// Almost the same function as the private core function DibNumColors()
static __inline WORD DibNumColors(BITMAPINFOHEADER CONST *lpbi)
{
    if (lpbi->biClrUsed != 0) return (WORD) lpbi->biClrUsed;

    /* a 24 bitcount DIB has no color table */
    return (lpbi->biBitCount <= 8) ? (1 << lpbi->biBitCount) : 0;
}

static HBITMAP DecodeBMPIcon(LPBYTE imageBuffer, size_t imageSize) {
    // size of bitmap header information
    DWORD dwFileSize = sizeof(BITMAPINFOHEADER);
    if (imageSize < dwFileSize)
        return NULL;

    LPBITMAPINFO pBmi = (LPBITMAPINFO)imageBuffer;

    // size with color table
    if (pBmi->bmiHeader.biCompression == BI_BITFIELDS)
        dwFileSize += 3 * sizeof(DWORD);
    else
        dwFileSize += DibNumColors(&pBmi->bmiHeader) * sizeof(RGBQUAD);

    pBmi->bmiHeader.biHeight /= 2;

    DWORD stride = (((pBmi->bmiHeader.biWidth * (DWORD)pBmi->bmiHeader.biBitCount) + 31) / 32 * 4);
    DWORD height = (DWORD) abs(pBmi->bmiHeader.biHeight);

    // size with bitmap data
    if (pBmi->bmiHeader.biCompression != BI_RGB)
        dwFileSize += pBmi->bmiHeader.biSizeImage;
    else {
        pBmi->bmiHeader.biSizeImage =  stride * height;
        dwFileSize += pBmi->bmiHeader.biSizeImage;
    }
    if (imageSize < dwFileSize)
        return NULL;

    HBITMAP hBitmap = CreateDIBitmap(hWindowDC, &pBmi->bmiHeader, CBM_INIT, imageBuffer, pBmi, DIB_RGB_COLORS);
    if(hBitmap) {
        // Inverse the height
        BYTE *source = imageBuffer + dwFileSize - stride;
        BYTE *destination = hBitmap->bitmapBits;
        for (int i = 0; i < height; ++i) {
            memcpy(destination, source, stride);
            source -= stride;
            destination += stride;
        }
    }
    // Only support 32bits RGBA BMP for now.
    return hBitmap;
}

static HBITMAP DecodePNGIcon(LPBYTE imageBuffer, size_t imageSize) {
    HBITMAP hBitmap = NULL;
    UINT uWidth,uHeight;
    LPBYTE pbyImage = NULL;
    UINT uError = lodepng_decode_memory(&pbyImage, &uWidth, &uHeight, imageBuffer, imageSize, LCT_RGBA, 8);
    if (uError == 0) {
        BITMAPINFO bmi;
        ZeroMemory(&bmi, sizeof(bmi));			// init bitmap info
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = (LONG) uWidth;
        bmi.bmiHeader.biHeight = (LONG) uHeight;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;			// create a true color DIB
        bmi.bmiHeader.biCompression = BI_RGB;

        // bitmap dimensions
        LONG lBytesPerLine = (((bmi.bmiHeader.biWidth * bmi.bmiHeader.biBitCount) + 31) / 32 * 4);
        bmi.bmiHeader.biSizeImage = (DWORD) (lBytesPerLine * bmi.bmiHeader.biHeight);

        // allocate buffer for pixels
        LPBYTE  pbyPixels;						// BMP buffer
        hBitmap = CreateDIBSection(hWindowDC, &bmi, DIB_RGB_COLORS, (VOID **)&pbyPixels, NULL, 0);
        if (hBitmap)
            memcpy(pbyPixels, pbyImage, bmi.bmiHeader.biSizeImage);
    }

    if (pbyImage != NULL)
        free(pbyImage);
    return hBitmap;
}

// ICO (file format) https://en.wikipedia.org/wiki/ICO_(file_format)
// https://docs.microsoft.com/en-us/previous-versions/ms997538(v=msdn.10)
typedef struct {
    WORD idReserved;
    WORD idType;
    WORD idCount;
} ICONDIR, *LPICONDIR;
typedef struct ICONDIRENTRY {
    BYTE bWidth; // Specifies image width in pixels. Can be any number between 0 and 255. Value 0 means image width is 256 pixels.
    BYTE bHeight; // Specifies image height in pixels. Can be any number between 0 and 255. Value 0 means image height is 256 pixels.
    BYTE bColorCount; // Specifies number of colors in the color palette. Should be 0 if the image does not use a color palette.
    BYTE bReserved; // Reserved. Should be 0.
    WORD wPlanes; // In ICO format: Specifies color planes. Should be 0 or 1.
    WORD wBitCount; // In ICO format: Specifies bits per pixel.
    DWORD dwBytesInRes; // Specifies the size of the image's data in bytes
    DWORD dwImageOffset; // Specifies the offset of BMP or PNG data from the beginning of the ICO/CUR file
} ICONDIRENTRY, *LPICONDIRENTRY;

typedef struct {
    BITMAPINFOHEADER   icHeader;      // DIB header
    RGBQUAD         icColors[1];   // Color table
    BYTE            icXOR[1];      // DIB bits for XOR mask
    BYTE            icAND[1];      // DIB bits for AND mask
} ICONIMAGE, *LPICONIMAGE;

HANDLE LoadImage(HINSTANCE hInst, LPCSTR name, UINT type, int cx, int cy, UINT fuLoad) {

    HANDLE hIconFile = CreateFile(name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hIconFile == INVALID_HANDLE_VALUE)
        return NULL;

    ICONDIR fileHeader;
    DWORD nNumberOfBytesToRead = 0;
    if(ReadFile(hIconFile, &fileHeader, sizeof(ICONDIR), &nNumberOfBytesToRead, NULL) == FALSE || sizeof(ICONDIR) != nNumberOfBytesToRead) {
        CloseHandle(hIconFile);
        return NULL;
    }

    size_t iconsBufferSize = fileHeader.idCount * sizeof(ICONDIRENTRY);
    ICONDIRENTRY * iconHeaderArray = malloc(iconsBufferSize);
    nNumberOfBytesToRead = 0;
    if(ReadFile(hIconFile, iconHeaderArray, iconsBufferSize, &nNumberOfBytesToRead, NULL) == FALSE || iconsBufferSize != nNumberOfBytesToRead) {
        CloseHandle(hIconFile);
        return NULL;
    }

    int maxWidth = -1;
    int maxHeight = -1;
    int maxBitPerPixel = -1;
    int maxIconIndex = -1;
    for (int i = 0; i < fileHeader.idCount; ++i) {
        ICONDIRENTRY * iconHeader = &(iconHeaderArray[i]);
        int width = iconHeader->bWidth == 0 ? 256 : iconHeader->bWidth;
        int height = iconHeader->bHeight == 0 ? 256 : iconHeader->bHeight;
        if(width >= maxWidth && height >= maxHeight && iconHeader->wBitCount > maxBitPerPixel) {
            maxWidth = width;
            maxHeight = height;
            maxBitPerPixel = iconHeader->wBitCount;
            maxIconIndex = i;
        }
    }
    maxIconIndex = 1; // To test BMP
    if(maxIconIndex == -1) {
        CloseHandle(hIconFile);
        return NULL;
    }

    DWORD dwBytesInRes = iconHeaderArray[maxIconIndex].dwBytesInRes;
    LPBYTE iconBuffer = malloc(dwBytesInRes);

    SetFilePointer(hIconFile, iconHeaderArray[maxIconIndex].dwImageOffset, 0, FILE_BEGIN);
    if(ReadFile(hIconFile, iconBuffer, dwBytesInRes, &nNumberOfBytesToRead, NULL) == FALSE || dwBytesInRes != nNumberOfBytesToRead) {
        CloseHandle(hIconFile);
        free(iconHeaderArray);
        free(iconBuffer);
        return NULL;
    }
    CloseHandle(hIconFile);

    HBITMAP icon = NULL;
    if (dwBytesInRes >= 8 && memcmp(iconBuffer, "\x89PNG\r\n\x1a\n", 8) == 0)
        // It is a PNG image
        icon = DecodePNGIcon(iconBuffer, dwBytesInRes);
    else
        // It is a BMP image
        icon = DecodeBMPIcon(iconBuffer, dwBytesInRes);


    free(iconHeaderArray);
    free(iconBuffer);

    if(!icon)
        return NULL;

    HANDLE handle = malloc(sizeof(struct _HANDLE));
    memset(handle, 0, sizeof(struct _HANDLE));
    handle->handleType = HANDLE_TYPE_ICON;
    handle->icon = icon;
    return handle;
}


LPARAM itemData[MAX_ITEMDATA];
TCHAR *itemString[MAX_ITEMDATA];
int itemDataCount = 0;
int selItemDataIndex[MAX_ITEMDATA];
int selItemDataCount = 0;
TCHAR labels[MAX_LABEL_SIZE];

LRESULT SendMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    //TODO
    if(currentDialogBoxMode == DialogBoxMode_GET_USRPRG32
    || currentDialogBoxMode == DialogBoxMode_GET_USRPRG42
    || currentDialogBoxMode == DialogBoxMode_SET_USRPRG32
    || currentDialogBoxMode == DialogBoxMode_SET_USRPRG42) {
        if (Msg == LB_ADDSTRING && itemDataCount < MAX_ITEMDATA) {
            itemString[itemDataCount] = (TCHAR *)lParam;
            return itemDataCount++;
        } else if (Msg == LB_SETITEMDATA && wParam < MAX_ITEMDATA) {
            itemData[wParam] = lParam;
        } else if (Msg == LB_GETITEMDATA && wParam < MAX_ITEMDATA) {
            return itemData[wParam];
        } else if (Msg == LB_GETCOUNT) {
            return itemDataCount;
        } else if (Msg == LB_GETSELCOUNT) {
            return selItemDataCount;
        } else if (Msg == LB_GETSEL && wParam < itemDataCount && wParam < MAX_ITEMDATA) {
            return selItemDataIndex[wParam];
        }
    }
    if(Msg == WM_SETICON && wParam == ICON_BIG) {
        if(lParam != NULL) {
            HANDLE hIcon = (HANDLE)lParam;
            if(hIcon->handleType == HANDLE_TYPE_ICON && hIcon->icon != NULL) {
                HBITMAP icon = hIcon->icon;
                if(icon && icon->bitmapInfoHeader && icon->bitmapBits)
                    setKMLIcon(icon->bitmapInfoHeader->biWidth, icon->bitmapInfoHeader->biHeight, icon->bitmapBits, icon->bitmapInfoHeader->biSizeImage);
            }
        } else {
            setKMLIcon(0, 0, NULL, 0);
        }
    }
    return NULL;
}
BOOL PostMessage(HWND handleWindow, UINT Msg, WPARAM wParam, LPARAM lParam) {
    //TODO
    if(handleWindow == hWnd && Msg == WM_COMMAND) {
        int menuCommand = (int) ((wParam & 0xffff) - 40000);
        LOGD("Menu Item %d", menuCommand);
        sendMenuItemCommand(menuCommand);
    }
    return NULL;
}


int MessageBox(HANDLE h, LPCTSTR szMessage, LPCTSTR title, int flags)
{
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

#if defined NEW_WIN32_SOUND_ENGINE
// this callback handler is called every time a buffer finishes playing
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    WAVE_OUT_LOGD("waveOut bqPlayerCallback()");

    HWAVEOUT hwo = context;
    LPWAVEHDR pWaveHeaderNextToRead = NULL;

    pthread_mutex_lock(&hwo->audioEngineLock);
    if (hwo->pWaveHeaderNextToRead) {
        pWaveHeaderNextToRead = hwo->pWaveHeaderNextToRead;
        hwo->pWaveHeaderNextToRead = hwo->pWaveHeaderNextToRead->lpNext;
        if(!hwo->pWaveHeaderNextToRead) {
            hwo->pWaveHeaderNextToWrite = NULL;
        }
    }
    pthread_mutex_unlock(&hwo->audioEngineLock);

    if(pWaveHeaderNextToRead) {
        PostThreadMessage(1, MM_WOM_DONE, hwo, pWaveHeaderNextToRead);
    } else {
        WAVE_OUT_LOGD("waveOut bqPlayerCallback() Nothing to play?");
    }

    WAVE_OUT_LOGD("bqPlayerCallback() before sem_post() +1");
    sem_post(&hwo->waveOutLock); // Increment
    WAVE_OUT_LOGD("bqPlayerCallback() after sem_post() +1");
}
#else

void onPlayerDone(HWAVEOUT hwo) {
    WAVE_OUT_LOGD("waveOut onPlayerDone()");
    //PostThreadMessage(0, MM_WOM_DONE, hwo, hwo->pWaveHeaderNext);
    // Artificially replace the post message MM_WOM_DONE
    bSoundSlow = FALSE;			// no sound slow down
    bEnableSlow = TRUE;			// reenable CPU slow down possibility
}

// this callback handler is called every time a buffer finishes playing
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    WAVE_OUT_LOGD("waveOut bqPlayerCallback()");
    HWAVEOUT hwo = context;
    if (hwo->pWaveHeaderNext != NULL) {
        LPWAVEHDR pWaveHeaderNext = hwo->pWaveHeaderNext->lpNext;
        free(hwo->pWaveHeaderNext->lpData);
        free(hwo->pWaveHeaderNext);
        hwo->pWaveHeaderNext = pWaveHeaderNext;
        if(pWaveHeaderNext != NULL) {
            WAVE_OUT_LOGD("waveOut bqPlayerCallback() bqPlayerBufferQueue->Enqueue");
            SLresult result = (*hwo->bqPlayerBufferQueue)->Enqueue(hwo->bqPlayerBufferQueue, pWaveHeaderNext->lpData, pWaveHeaderNext->dwBufferLength);
            if (SL_RESULT_SUCCESS != result) {
                WAVE_OUT_LOGD("waveOut bqPlayerCallback() Enqueue Error %d", result);
                onPlayerDone(hwo);
                // Error
                //pthread_mutex_unlock(&hwo->audioEngineLock);
//                return;
            }
//            return;
        } else {
            WAVE_OUT_LOGD("waveOut bqPlayerCallback() Nothing next, so, this is the end");
            onPlayerDone(hwo);
        }
    } else {
        WAVE_OUT_LOGD("waveOut bqPlayerCallback() Nothing to play? So, this is the end");
        onPlayerDone(hwo);
    }
}

// Timer to replace the missing bqPlayerCallback
void onPlayerDoneTimerCallback(void *context) {
    WAVE_OUT_LOGD("waveOut onPlayerDoneTimerCallback()");
    HWAVEOUT hwo = context;
    onPlayerDone(hwo);

    //TODO May be needed if an attached occurs in the future
    //jniDetachCurrentThread();
}

#endif

MMRESULT waveOutOpen(LPHWAVEOUT phwo, UINT uDeviceID, LPCWAVEFORMATEX pwfx, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen) {
    WAVE_OUT_LOGD("waveOutOpen()");

    HWAVEOUT handle = (HWAVEOUT)malloc(sizeof(struct _HWAVEOUT));
    memset(handle, 0, sizeof(struct _HWAVEOUT));
    handle->pwfx = (WAVEFORMATEX *) pwfx;
    handle->uDeviceID = uDeviceID;

    SLresult result;

    // create engine
    result = slCreateEngine(&handle->engineObject, 0, NULL, 0, NULL, NULL);
    //_ASSERT(SL_RESULT_SUCCESS == result);
    if(result != SL_RESULT_SUCCESS) {
        WAVE_OUT_LOGD("waveOutOpen() slCreateEngine error: %d", result);
        waveOutClose(handle);
        return MMSYSERR_ERROR;
    }

    // realize the engine
    result = (*handle->engineObject)->Realize(handle->engineObject, SL_BOOLEAN_FALSE);
    if(result != SL_RESULT_SUCCESS) {
        WAVE_OUT_LOGD("waveOutOpen() engineObject->Realize error: %d", result);
        waveOutClose(handle);
        return MMSYSERR_ERROR;
    }

    // get the engine interface, which is needed in order to create other objects
    result = (*handle->engineObject)->GetInterface(handle->engineObject, SL_IID_ENGINE, &handle->engineEngine);
    if(result != SL_RESULT_SUCCESS) {
        WAVE_OUT_LOGD("waveOutOpen() engineObject->GetInterface error: %d", result);
        waveOutClose(handle);
        return MMSYSERR_ERROR;
    }

    // create output mix, with environmental reverb specified as a non-required interface
    const SLInterfaceID ids[1] = { SL_IID_ENVIRONMENTALREVERB };
    const SLboolean req[1] = { SL_BOOLEAN_FALSE };
    result = (*handle->engineEngine)->CreateOutputMix(handle->engineEngine, &handle->outputMixObject, 1, ids, req);
    if(result != SL_RESULT_SUCCESS) {
        WAVE_OUT_LOGD("waveOutOpen() engineObject->CreateOutputMix error: %d", result);
        waveOutClose(handle);
        return MMSYSERR_ERROR;
    }
    // realize the output mix
    result = (*handle->outputMixObject)->Realize(handle->outputMixObject, SL_BOOLEAN_FALSE);
    if(result != SL_RESULT_SUCCESS) {
        WAVE_OUT_LOGD("waveOutOpen() outputMixObject->Realize error: %d", result);
        waveOutClose(handle);
        return MMSYSERR_ERROR;
    }
    // configure audio source
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {
            SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,   // locatorType
#if defined NEW_WIN32_SOUND_ENGINE
            //1                                          // numBuffers
            //2                                          // numBuffers
            20                                          // numBuffers
#else
            20                                          // numBuffers
#endif
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
            handle->outputMixObject           // outputMix
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

    result = (*handle->engineEngine)->CreateAudioPlayer(handle->engineEngine, &handle->bqPlayerObject, &audioSrc, &audioSnk, 2, ids2, req2);
    if(result != SL_RESULT_SUCCESS) {
        WAVE_OUT_LOGD("waveOutOpen() engineEngine->CreateAudioPlayer error: %d", result);
        waveOutClose(handle);
        return MMSYSERR_ERROR;
    }

    // realize the player
    result = (*handle->bqPlayerObject)->Realize(handle->bqPlayerObject, SL_BOOLEAN_FALSE);
    if(result != SL_RESULT_SUCCESS) {
        WAVE_OUT_LOGD("waveOutOpen() bqPlayerObject->Realize error: %d", result);
        waveOutClose(handle);
        return MMSYSERR_ERROR;
    }

    // get the play interface
    result = (*handle->bqPlayerObject)->GetInterface(handle->bqPlayerObject, SL_IID_PLAY, &handle->bqPlayerPlay);
    if(result != SL_RESULT_SUCCESS) {
        WAVE_OUT_LOGD("waveOutOpen() bqPlayerObject->GetInterface SL_IID_PLAY error: %d", result);
        waveOutClose(handle);
        return MMSYSERR_ERROR;
    }

    // get the buffer queue interface
    result = (*handle->bqPlayerObject)->GetInterface(handle->bqPlayerObject, SL_IID_BUFFERQUEUE, &handle->bqPlayerBufferQueue);
    if(result != SL_RESULT_SUCCESS) {
        WAVE_OUT_LOGD("waveOutOpen() bqPlayerObject->GetInterface SL_IID_BUFFERQUEUE error: %d", result);
        waveOutClose(handle);
        return MMSYSERR_ERROR;
    }

    // register callback on the buffer queue
    result = (*handle->bqPlayerBufferQueue)->RegisterCallback(handle->bqPlayerBufferQueue, bqPlayerCallback, handle);
    if(result != SL_RESULT_SUCCESS) {
        WAVE_OUT_LOGD("waveOutOpen() bqPlayerBufferQueue->RegisterCallback error: %d", result);
        waveOutClose(handle);
        return MMSYSERR_ERROR;
    }

    // get the volume interface
    result = (*handle->bqPlayerObject)->GetInterface(handle->bqPlayerObject, SL_IID_VOLUME, &handle->bqPlayerVolume);
    if(result != SL_RESULT_SUCCESS) {
        WAVE_OUT_LOGD("waveOutOpen() bqPlayerObject->GetInterface SL_IID_VOLUME error: %d", result);
        waveOutClose(handle);
        return MMSYSERR_ERROR;
    }

    // set the player's state to playing
    result = (*handle->bqPlayerPlay)->SetPlayState(handle->bqPlayerPlay, SL_PLAYSTATE_PLAYING);
    if(result != SL_RESULT_SUCCESS) {
        WAVE_OUT_LOGD("waveOutOpen() bqPlayerObject->SetPlayState error: %d", result);
        waveOutClose(handle);
        return MMSYSERR_ERROR;
    }

#if defined NEW_WIN32_SOUND_ENGINE
    sem_init(&handle->waveOutLock, 0, 1);
#else
    struct sigevent sev;
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = (void (*)(sigval_t)) onPlayerDoneTimerCallback; //this function will be called when timer expires
    sev.sigev_value.sival_ptr = handle; //this argument will be passed to cbf
    sev.sigev_notify_attributes = NULL;
    timer_t * timer = &(handle->playerDoneTimer);
    if (timer_create(CLOCK_MONOTONIC, &sev, timer) == -1) {
        WAVE_OUT_LOGD("waveOutOpen() ERROR in timer_create");
        return NULL;
    }

    long delayInMilliseconds = 1000; // msecs
    handle->playerDoneTimerSetTimes.it_value.tv_sec = delayInMilliseconds / 1000;
    handle->playerDoneTimerSetTimes.it_value.tv_nsec = (delayInMilliseconds % 1000) * 1000000;
    handle->playerDoneTimerSetTimes.it_interval.tv_sec = 0;
    handle->playerDoneTimerSetTimes.it_interval.tv_nsec = 0;
#endif

    *phwo = handle;
    return MMSYSERR_NOERROR;
}

MMRESULT waveOutReset(HWAVEOUT hwo) {
    WAVE_OUT_LOGD("waveOutReset()");
    //TODO
    return 0;
}

MMRESULT waveOutClose(HWAVEOUT handle) {
    WAVE_OUT_LOGD("waveOutClose()");

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

#if defined NEW_WIN32_SOUND_ENGINE
    sem_destroy(&handle->waveOutLock);
#else
    onPlayerDone(handle);
    if(handle->playerDoneTimer)
        timer_delete(handle->playerDoneTimer);
#endif

    memset(handle, 0, sizeof(struct _HWAVEOUT));
    free(handle);
    return 0;
}

MMRESULT waveOutPrepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh) {
    WAVE_OUT_LOGD("waveOutPrepareHeader()");
    //TODO
    return 0;
}

MMRESULT waveOutUnprepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh) {
    WAVE_OUT_LOGD("waveOutUnprepareHeader()");
    //TODO
    return 0;
}

MMRESULT waveOutWrite(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh) {
    WAVE_OUT_LOGD("waveOutWrite()");

#if defined NEW_WIN32_SOUND_ENGINE

    WAVE_OUT_LOGD("waveOutWrite() before sem_wait() -1");
    sem_wait(&hwo->waveOutLock); // Decrement, lock if 0
    WAVE_OUT_LOGD("waveOutWrite() after sem_wait() -1");


    LPWAVEHDR pWaveHeaderNextToWriteBackup = hwo->pWaveHeaderNextToWrite;

    pthread_mutex_lock(&hwo->audioEngineLock);
    if(hwo->pWaveHeaderNextToWrite) {
        hwo->pWaveHeaderNextToWrite->lpNext = pwh;
    } else {
        hwo->pWaveHeaderNextToRead = pwh;
    }
    hwo->pWaveHeaderNextToWrite = pwh;
    pwh->lpNext = NULL;
    pthread_mutex_unlock(&hwo->audioEngineLock);

    WAVE_OUT_LOGD("waveOutWrite() bqPlayerBufferQueue->Enqueue() play right now");
    SLresult result = (*hwo->bqPlayerBufferQueue)->Enqueue(hwo->bqPlayerBufferQueue, pwh->lpData, pwh->dwBufferLength);
    if (SL_RESULT_SUCCESS != result) {
        WAVE_OUT_LOGD("waveOutWrite() bqPlayerBufferQueue->Enqueue() error: %d (SL_RESULT_BUFFER_INSUFFICIENT=7)", result);

        pthread_mutex_lock(&hwo->audioEngineLock);
        if(pWaveHeaderNextToWriteBackup) {
            hwo->pWaveHeaderNextToWrite = pWaveHeaderNextToWriteBackup;
            hwo->pWaveHeaderNextToWrite->lpNext = NULL;
            if(hwo->pWaveHeaderNextToRead == pwh) {
                hwo->pWaveHeaderNextToRead = pWaveHeaderNextToWriteBackup;
            }
        } else {
            hwo->pWaveHeaderNextToWrite = NULL;
            hwo->pWaveHeaderNextToRead = NULL;
        }
        pthread_mutex_unlock(&hwo->audioEngineLock);

        PostThreadMessage(1, MM_WOM_DONE, hwo, pwh);

        return MMSYSERR_ERROR;
    }

#else

    pwh->lpNext = NULL;
    if(hwo->pWaveHeaderNext == NULL) {
        hwo->pWaveHeaderNext = pwh;
        WAVE_OUT_LOGD("waveOutWrite() bqPlayerBufferQueue->Enqueue() play right now");
        SLresult result = (*hwo->bqPlayerBufferQueue)->Enqueue(hwo->bqPlayerBufferQueue, pwh->lpData, pwh->dwBufferLength);
        if (SL_RESULT_SUCCESS != result) {
            WAVE_OUT_LOGD("waveOutWrite() bqPlayerBufferQueue->Enqueue() error: %d (SL_RESULT_BUFFER_INSUFFICIENT=7)", result);
            //onPlayerDone(hwo);
            //pthread_mutex_unlock(&hwo->audioEngineLock);
            return MMSYSERR_ERROR;
        }
    } else {
        LPWAVEHDR pWaveHeaderNext = hwo->pWaveHeaderNext;
        while (pWaveHeaderNext->lpNext)
            pWaveHeaderNext = pWaveHeaderNext->lpNext;
        WAVE_OUT_LOGD("waveOutWrite() play when finishing the current one");
        pWaveHeaderNext->lpNext = pwh;
    }

    if (timer_settime(hwo->playerDoneTimer, 0, &(hwo->playerDoneTimerSetTimes), NULL) == -1) {
        WAVE_OUT_LOGD("waveOutWrite() ERROR in timer_settime (playerDoneTimerSetTimes)");
    }

#endif

    return MMSYSERR_NOERROR;
}


MMRESULT waveOutGetDevCaps(UINT_PTR uDeviceID, LPWAVEOUTCAPS pwoc, UINT cbwoc) {
    WAVE_OUT_LOGD("waveOutGetDevCaps()");
    if(pwoc) {
        pwoc->dwFormats = WAVE_FORMAT_4M08;
    }
    return MMSYSERR_NOERROR;
}

MMRESULT waveOutGetID(HWAVEOUT hwo, LPUINT puDeviceID) {
    WAVE_OUT_LOGD("waveOutGetID()");
    //TODO
    return 0;
}




BOOL GetMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) {
#if defined NEW_WIN32_SOUND_ENGINE
    pthread_t thId = pthread_self();
    for(int i = 0; i < MAX_CREATED_THREAD; i++) {
        HANDLE threadHandle = threads[i];
        if(threadHandle && threadHandle->threadId == thId) {
            WaitForSingleObject(threadHandle->threadEventMessage, INFINITE);
            if(lpMsg)
                memcpy(lpMsg, &threadHandle->threadMessage, sizeof(MSG));
            if(lpMsg->message == WM_QUIT)
                return FALSE;
            return TRUE;
        }
    }

    return FALSE;
#else
    return FALSE;
#endif
}

BOOL PostThreadMessage(DWORD idThread, UINT Msg, WPARAM wParam, LPARAM lParam) {
    WAVE_OUT_LOGD("waveOut PostThreadMessage()");
#if defined NEW_WIN32_SOUND_ENGINE
    for(int i = 0; i < MAX_CREATED_THREAD; i++) {
        HANDLE threadHandle = threads[i];
        if(threadHandle && threadHandle->threadIndex == idThread) {
            threadHandle->threadMessage.hwnd = NULL;
            threadHandle->threadMessage.message = Msg;
            threadHandle->threadMessage.wParam = wParam;
            threadHandle->threadMessage.lParam = lParam;
            WAVE_OUT_LOGD("waveOut SetEvent()");
            SetEvent(threadHandle->threadEventMessage);
            return TRUE;
        }
    }
    return TRUE;
    //return FALSE;
#else
    return TRUE;
#endif
}

HWND CreateWindow() {
    HANDLE handle = malloc(sizeof(struct _HANDLE));
    memset(handle, 0, sizeof(struct _HANDLE));
    handle->handleType = HANDLE_TYPE_WINDOW;
    handle->windowDC = CreateCompatibleDC(NULL);
    return handle;
}

BOOL DestroyWindow(HWND hWnd) {
    memset(hWnd, 0, sizeof(struct _HANDLE));
    free(hWnd);
    return TRUE;
}

BOOL GetWindowPlacement(HWND hWnd, WINDOWPLACEMENT *lpwndpl) {
    if(lpwndpl) {
        lpwndpl->rcNormalPosition.left = 0;
        lpwndpl->rcNormalPosition.top = 0;
    }
    return TRUE;
}
BOOL SetWindowPlacement(HWND hWnd, CONST WINDOWPLACEMENT *lpwndpl) { return 0; }
extern void draw();
BOOL InvalidateRect(HWND hWnd, CONST RECT *lpRect, BOOL bErase) {
    // Update when switch the screen off
    draw(); //TODO Need a true WM_PAINT event!
    return 0;
}
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
BOOL WINAPI SetWindowOrgEx(HDC hdc, int x, int y, LPPOINT lppt) {
    if(lppt) {
        lppt->x = hdc->windowOriginX;
        lppt->y = hdc->windowOriginY;
    }
    hdc->windowOriginX = x;
    hdc->windowOriginY = y;
    return TRUE;
}

// GDI
HGDIOBJ SelectObject(HDC hdc, HGDIOBJ h) {
    if(h) {
        switch (h->handleType) {
            case HGDIOBJ_TYPE_PEN:
                break;
            case HGDIOBJ_TYPE_BRUSH: {
                HBRUSH oldSelectedBrushColor = hdc->selectedBrushColor;
                hdc->selectedBrushColor = h;
                return oldSelectedBrushColor; //h;
            }
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
    if(hdc)
        return hdc->selectedBitmap;
    return NULL;
}
BOOL DeleteObject(HGDIOBJ ho) {
    PAINT_LOGD("PAINT DeleteObject(ho: %p)", ho);
    if(ho) {
        switch(ho->handleType) {
            case HGDIOBJ_TYPE_PALETTE: {
                PAINT_LOGD("PAINT DeleteObject() HGDIOBJ_TYPE_PALETTE");
                ho->handleType = HGDIOBJ_TYPE_INVALID;
                if(ho->paletteLog)
                    free(ho->paletteLog);
                ho->paletteLog = NULL;
                free(ho);
                return TRUE;
            }
            case HGDIOBJ_TYPE_BITMAP: {
                PAINT_LOGD("PAINT DeleteObject() HGDIOBJ_TYPE_BITMAP");
                ho->handleType = HGDIOBJ_TYPE_INVALID;
                if(ho->bitmapInfo)
                    free((void *) ho->bitmapInfo);
                ho->bitmapInfo = NULL;
                ho->bitmapInfoHeader = NULL;
                if(ho->bitmapBits)
                    free((void *) ho->bitmapBits);
                ho->bitmapBits = NULL;
                free(ho);
                return TRUE;
            }
            default:
                break;
        }
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

COLORREF SetBkColor(HDC hdc, COLORREF color) {
    COLORREF backgroundColorBackup = hdc->backgroundColor;
    hdc->backgroundColor = color;
    hdc->isBackgroundColorSet = TRUE;
    return backgroundColorBackup;
}

// DC

HDC CreateCompatibleDC(HDC hdc) {
    HDC handle = (HDC)malloc(sizeof(struct _HDC));
    memset(handle, 0, sizeof(struct _HDC));
    handle->handleType = HDC_TYPE_DC;
    handle->hdcCompatible = hdc;
    return handle;
}
HDC GetDC(HWND hWnd) {
    if(!hWnd)
        return NULL;
    return hWnd->windowDC;
}
int ReleaseDC(HWND hWnd, HDC hDC) {
    if(!hWnd)
        return NULL;
    hWnd->windowDC = NULL; //?
    return TRUE;
}

BOOL DeleteDC(HDC hdc) {
    memset(hdc, 0, sizeof(struct _HDC));
    free(hdc);
    return TRUE;
}

HBRUSH  WINAPI CreateSolidBrush(COLORREF color) {
    HGDIOBJ handle = (HGDIOBJ)malloc(sizeof(_HGDIOBJ));
    memset(handle, 0, sizeof(_HGDIOBJ));
    handle->handleType = HGDIOBJ_TYPE_BRUSH;
    handle->brushColor = color;
    return handle;
}

BOOL MoveToEx(HDC hdc, int x, int y, LPPOINT lppt) {
    //TODO
    return 0;
}
BOOL LineTo(HDC hdc, int x, int y) {
    //TODO
    return 0;
}
BOOL PatBlt(HDC hdcDest, int x, int y, int w, int h, DWORD rop) {
    PAINT_LOGD("PAINT PatBlt(hdcDest: %p, x: %d, y: %d, w: %d, h: %d, rop: 0x%08x)", hdcDest, x, y, w, h, rop);

    if((hdcDest->selectedBitmap || hdcDest->hdcCompatible == NULL) && w && h) {
        HBITMAP hBitmapDestination = NULL;
        void * pixelsDestination = NULL;
        int destinationWidth = 0;
        int destinationHeight = 0;
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

            destinationStride = androidBitmapInfo.stride;

            if ((ret = AndroidBitmap_lockPixels(jniEnv, bitmapMainScreen, &pixelsDestination)) < 0) {
                LOGD("AndroidBitmap_lockPixels() failed ! error=%d", ret);
                return FALSE;
            }
        } else {
            hBitmapDestination = hdcDest->selectedBitmap;
            pixelsDestination = (void *) hBitmapDestination->bitmapBits;

            destinationWidth = hBitmapDestination->bitmapInfoHeader->biWidth;
            destinationHeight = abs(hBitmapDestination->bitmapInfoHeader->biHeight);

            destinationStride = (float)(4 * ((destinationWidth * hBitmapDestination->bitmapInfoHeader->biBitCount + 31) / 32));
        }

        HPALETTE palette = hdcDest->realizedPalette;
        if(!palette)
            palette = hdcDest->selectedPalette;
        PALETTEENTRY * palPalEntry = palette && palette->paletteLog && palette->paletteLog->palPalEntry ?
                                     palette->paletteLog->palPalEntry : NULL;

        COLORREF brushColor = 0xFF000000; // 0xAABBGGRR
        if(hdcDest->selectedBrushColor) {
            brushColor = hdcDest->selectedBrushColor->brushColor;
        }

        for (int currentY = y; currentY < y + h; currentY++) {
            for (int currentX = x; currentX < x + w; currentX++) {
                BYTE * destinationPixel = pixelsDestination + (int)(destinationStride * currentY + 4.0 * currentX);

                // -> ARGB_8888
                switch (rop) {
                    case DSTINVERT:
                        destinationPixel[0] = (BYTE) (255 - destinationPixel[0]);
                        destinationPixel[1] = (BYTE) (255 - destinationPixel[1]);
                        destinationPixel[2] = (BYTE) (255 - destinationPixel[2]);
                        destinationPixel[3] = 255;
                        break;
                    case BLACKNESS:
                        destinationPixel[0] = palPalEntry[0].peRed;
                        destinationPixel[1] = palPalEntry[0].peGreen;
                        destinationPixel[2] = palPalEntry[0].peBlue;
                        destinationPixel[3] = 255;
                        break;
                    case WHITENESS:
                        destinationPixel[0] = 255; //palPalEntry[1].peRed;
                        destinationPixel[1] = 255; //palPalEntry[1].peGreen;
                        destinationPixel[2] = 255; //palPalEntry[1].peBlue;
                        destinationPixel[3] = 255;
                        break;
                    case PATCOPY:
                        // 0xAABBGGRR
                        *((UINT *)destinationPixel) = brushColor;
                        break;
                    default:
                        break;
                }
            }
        }

        if(jniEnv)
            AndroidBitmap_unlockPixels(jniEnv, bitmapMainScreen);
    }
    return 0;
}
#define ROP_PSDPxax 0x00B8074A				// ternary ROP
#define ROP_PDSPxax 0x00D80745
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
    PAINT_LOGD("PAINT StretchBlt(hdcDest: %p, xDest: %d, yDest: %d, wDest: %d, hDest: %d, hdcSrc: %p, xSrc: %d, ySrc: %d, wSrc: %d, hSrc: %d, rop: 0x%08x)",
            hdcDest, xDest, yDest, wDest, hDest, hdcSrc, xSrc, ySrc, wSrc, hSrc, rop);

    if(hdcDest && hdcSrc
    && (hdcDest->selectedBitmap || hdcDest->hdcCompatible == NULL)
    && hdcSrc->selectedBitmap && hDest && hSrc) {

        HBITMAP hBitmapSource = hdcSrc->selectedBitmap;
        void * pixelsSource = (void *) hBitmapSource->bitmapBits;

        HBITMAP hBitmapDestination = NULL;
        void * pixelsDestination = NULL;
        int destinationBitCount = 8;

        BOOL reverseHeight = hBitmapSource->bitmapInfoHeader->biHeight < 0;

        int sourceWidth = hBitmapSource->bitmapInfoHeader->biWidth;
        int sourceHeight = abs(hBitmapSource->bitmapInfoHeader->biHeight); // Can be < 0
        int destinationWidth = 0;
        int destinationHeight = 0;

        UINT sourceBitCount = hBitmapSource->bitmapInfoHeader->biBitCount;
        int sourceStride = 4 * ((sourceWidth * hBitmapSource->bitmapInfoHeader->biBitCount + 31) / 32);
        int destinationStride = 0;

        JNIEnv * jniEnv = NULL;
        jint ret;

        if(hdcDest->hdcCompatible == NULL) {
            // We update the main window

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
            destinationBitCount = 32;
            destinationStride = androidBitmapInfo.stride;

            if ((ret = AndroidBitmap_lockPixels(jniEnv, bitmapMainScreen, &pixelsDestination)) < 0) {
                LOGD("AndroidBitmap_lockPixels() failed ! error=%d", ret);
                return FALSE;
            }
        } else {
            hBitmapDestination = hdcDest->selectedBitmap;
            pixelsDestination = (void *) hBitmapDestination->bitmapBits;

            destinationWidth = hBitmapDestination->bitmapInfoHeader->biWidth;
            destinationHeight = abs(hBitmapDestination->bitmapInfoHeader->biHeight);
            destinationBitCount = hBitmapDestination->bitmapInfoHeader->biBitCount;
            destinationStride = 4 * ((destinationWidth * hBitmapDestination->bitmapInfoHeader->biBitCount + 31) / 32);
        }

        xDest -= hdcDest->windowOriginX;
        yDest -= hdcDest->windowOriginY;

        //LOGD("StretchBlt(%p, x:%d, y:%d, w:%d, h:%d, %08x, x:%d, y:%d, w:%d, h:%d) -> sourceBitCount: %d", hdcDest->hdcCompatible, xDest, yDest, wDest, hDest, hdcSrc, xSrc, ySrc, wSrc, hSrc, sourceBitCount);

        HPALETTE palette = hdcSrc->realizedPalette;
        if(!palette)
            palette = hdcSrc->selectedPalette;
        PALETTEENTRY * palPalEntry = palette && palette->paletteLog && palette->paletteLog->palPalEntry ?
                                     palette->paletteLog->palPalEntry : NULL;
        if(!palPalEntry && sourceBitCount <= 8 && hBitmapSource->bitmapInfoHeader->biClrUsed > 0) {
            palPalEntry = (PALETTEENTRY *)hBitmapSource->bitmapInfo->bmiColors;
        }
        COLORREF brushColor = 0xFF000000; // 0xAABBGGRR
        if(hdcDest->selectedBrushColor) {
            brushColor = hdcDest->selectedBrushColor->brushColor;
        }
        COLORREF backgroundColor = 0xFF000000; // 0xAABBGGRR
        if(sourceBitCount > 1 && destinationBitCount == 1 && hdcSrc->isBackgroundColorSet)
        {
            backgroundColor = hdcSrc->backgroundColor;
        }
        else if(sourceBitCount == 1 && destinationBitCount > 1 && hdcDest->isBackgroundColorSet)
        {
            backgroundColor = hdcDest->backgroundColor;
        }

        StretchBltInternal(xDest, yDest, wDest, hDest, pixelsDestination, destinationBitCount,
                           destinationStride, destinationWidth, destinationHeight,
                           xSrc, ySrc, wSrc, hSrc, pixelsSource, sourceBitCount,
                           sourceStride, sourceWidth, sourceHeight,
                           rop, reverseHeight, palPalEntry, brushColor, backgroundColor);

        if(jniEnv && hdcDest->hdcCompatible == NULL && (ret = AndroidBitmap_unlockPixels(jniEnv, bitmapMainScreen)) < 0) {
            LOGD("AndroidBitmap_unlockPixels() failed ! error=%d", ret);
            return FALSE;
        }

        return TRUE;
    }
    return FALSE;
}

void StretchBltInternal(int xDest, int yDest, int wDest, int hDest,
                        const void *pixelsDestination, int destinationBitCount, int destinationStride, int destinationWidth, int destinationHeight,
                        int xSrc, int ySrc, int wSrc, int hSrc,
                        const void *pixelsSource, UINT sourceBitCount, int sourceStride, int sourceWidth, int sourceHeight,
                        DWORD rop, BOOL reverseHeight, const PALETTEENTRY *palPalEntry, COLORREF brushColor, COLORREF backgroundColor) {
    int dst_maxx = xDest + wDest;
    int dst_maxy = yDest + hDest;

    if(xDest < 0) {
            wDest += xDest;
            xDest = 0;
        }
    if(yDest < 0) {
            hDest += yDest;
            yDest = 0;
        }
    if(dst_maxx > destinationWidth)
            dst_maxx = destinationWidth;
    if(dst_maxy > destinationHeight)
            dst_maxy = destinationHeight;

    for (int y = yDest; y < dst_maxy; y++) {
            int src_cury = ySrc + (y - yDest) * hSrc / hDest;
            if(!reverseHeight)
                src_cury = sourceHeight - 1 - src_cury;
            if (src_cury < 0 || src_cury >= sourceHeight)
                continue;
            BYTE parity = (BYTE) xSrc;
            for (int x = xDest; x < dst_maxx; x++, parity++) {
                int src_curx = xSrc + (x - xDest) * wSrc / wDest;
                if (src_curx < 0 || src_curx >= sourceWidth)
                    continue;

                BYTE * sourcePixelBase = pixelsSource + sourceStride * src_cury;
                BYTE * destinationPixelBase = pixelsDestination + destinationStride * y;

                COLORREF sourceColor = 0xFF000000;
                BYTE * sourceColorPointer = (BYTE *) &sourceColor;

                switch (sourceBitCount) {
                    case 1: {
                        //TODO https://devblogs.microsoft.com/oldnewthing/?p=29013
                        // When blitting from a monochrome DC to a color DC,
                        // the color black in the source turns into the destination’s text color,
                        // and the color white in the source turns into the destination’s background
                        // color.
                        BYTE * sourcePixel = sourcePixelBase + ((UINT)src_curx >> (UINT)3);
                        UINT bitNumber = (UINT) (src_curx % 8);
                        if(*sourcePixel & ((UINT)1 << bitNumber)) {
                            // Monochrome 1=White
                            sourceColorPointer[0] = 255;
                            sourceColorPointer[1] = 255;
                            sourceColorPointer[2] = 255;
                        } else {
                            // Monochrome 0=Black
                            sourceColorPointer[0] = 0;
                            sourceColorPointer[1] = 0;
                            sourceColorPointer[2] = 0;
                        }
                        sourceColorPointer[3] = 255;
                        break;
                    }
                    case 4: {
                        int currentXBytes = ((sourceBitCount >> (UINT)2) * src_curx) >> (UINT)1;
                        BYTE * sourcePixel = sourcePixelBase + currentXBytes;
                        BYTE colorIndex = (parity & (BYTE)0x1 ? sourcePixel[0] & (BYTE)0x0F : sourcePixel[0] >> (UINT)4);
                        if (palPalEntry) {
                            sourceColorPointer[0] = palPalEntry[colorIndex].peBlue;
                            sourceColorPointer[1] = palPalEntry[colorIndex].peGreen;
                            sourceColorPointer[2] = palPalEntry[colorIndex].peRed;
                            sourceColorPointer[3] = 255;
                        } else {
                            sourceColorPointer[0] = colorIndex;
                            sourceColorPointer[1] = colorIndex;
                            sourceColorPointer[2] = colorIndex;
                            sourceColorPointer[3] = 255;
                        }
                        break;
                    }
                    case 8: {
                        BYTE * sourcePixel = sourcePixelBase + src_curx;
                        BYTE colorIndex = sourcePixel[0];
                        if (palPalEntry) {
                            sourceColorPointer[0] = palPalEntry[colorIndex].peBlue;
                            sourceColorPointer[1] = palPalEntry[colorIndex].peGreen;
                            sourceColorPointer[2] = palPalEntry[colorIndex].peRed;
                            sourceColorPointer[3] = 255;
                        } else {
                            sourceColorPointer[0] = colorIndex;
                            sourceColorPointer[1] = colorIndex;
                            sourceColorPointer[2] = colorIndex;
                            sourceColorPointer[3] = 255;
                        }
                        break;
                    }
                    case 24: {
                        BYTE * sourcePixel = sourcePixelBase + 3 * src_curx;
                        sourceColorPointer[0] = sourcePixel[2];
                        sourceColorPointer[1] = sourcePixel[1];
                        sourceColorPointer[2] = sourcePixel[0];
                        sourceColorPointer[3] = 255;
                        break;
                    }
                    case 32: {
                        BYTE *sourcePixel = sourcePixelBase + 4 * src_curx;
                        memcpy(sourceColorPointer, sourcePixel, 4);
                        break;
                    }
                    default:
                        break;
                }

                switch (destinationBitCount) {
                    case 1: {
                        //TODO https://devblogs.microsoft.com/oldnewthing/?p=29013
                        // If you blit from a color DC to a monochrome DC,
                        // then all pixels in the source that are equal to the background color
                        // will turn white, and all other pixels will turn black.
                        // In other words, GDI considers a monochrome bitmap to be
                        // black pixels on a white background.
                        BYTE * destinationPixel = destinationPixelBase + (x >> 3);
                        UINT bitNumber = x % 8;
                        if(backgroundColor == sourceColor) {
                            *destinationPixel |= (1 << bitNumber); // 1 White
                        } else {
                            *destinationPixel &= ~(1 << bitNumber); // 0 Black
                        }
                        break;
                    }
                    case 4: {
                        //TODO
                        break;
                    }
                    case 8: {
                        //TODO
                        break;
                    }
                    case 24: {
                        BYTE * destinationPixel = destinationPixelBase + 3 * x;
                        destinationPixel[0] = sourceColorPointer[0];
                        destinationPixel[1] = sourceColorPointer[1];
                        destinationPixel[2] = sourceColorPointer[2];
                        break;
                    }
                    case 32: {
                        BYTE * destinationPixel = destinationPixelBase + 4 * x;
                        // https://docs.microsoft.com/en-us/windows/desktop/gdi/ternary-raster-operations
                        // http://www.qnx.com/developers/docs/6.4.1/gf/dev_guide/api/gf_context_set_rop.html
                        if (rop == ROP_PDSPxax) { // P ^ (D & (S ^ P))
                            UINT destination = *((UINT *) destinationPixel);
                            *((UINT *)destinationPixel) = (brushColor ^ (destination & (sourceColor ^ brushColor))) | 0xFF000000;
                        } else if (rop == ROP_PSDPxax) { // P ^ (S & (D ^ P))
                            UINT destination = *((UINT *) destinationPixel);
                            *((UINT *)destinationPixel) = (brushColor ^ (sourceColor & (destination ^ brushColor))) | 0xFF000000;
                        } else if (rop == SRCAND) { // dest = source AND dest
                            UINT destination = *((UINT *) destinationPixel);
                            *((UINT *)destinationPixel) = (sourceColor & destination) | 0xFF000000;
                        } else
                            *((UINT *)destinationPixel) = sourceColor;
                        break;
                    }
                    default:
                        break;
                }
            }
        }
}

UINT SetDIBColorTable(HDC  hdc, UINT iStart, UINT cEntries, CONST RGBQUAD *prgbq) {
    if(prgbq
       && hdc && hdc->realizedPalette && hdc->realizedPalette->paletteLog && hdc->realizedPalette->paletteLog->palPalEntry
       && hdc->realizedPalette->paletteLog->palNumEntries > 0 && iStart < hdc->realizedPalette->paletteLog->palNumEntries) {
        PALETTEENTRY * palPalEntry = hdc->realizedPalette->paletteLog->palPalEntry;
        for (int i = iStart, j = 0; i < cEntries; i++, j++) {
            palPalEntry[i].peRed = prgbq[j].rgbRed;
            palPalEntry[i].peGreen = prgbq[j].rgbGreen;
            palPalEntry[i].peBlue = prgbq[j].rgbBlue;
            palPalEntry[i].peFlags = 0;
        }
    }
    return 0;
}
HBITMAP CreateBitmap( int nWidth, int nHeight, UINT nPlanes, UINT nBitCount, CONST VOID *lpBits) {
    PAINT_LOGD("PAINT CreateBitmap()");

    HGDIOBJ newHBITMAP = (HGDIOBJ)malloc(sizeof(_HGDIOBJ));
    memset(newHBITMAP, 0, sizeof(_HGDIOBJ));
    newHBITMAP->handleType = HGDIOBJ_TYPE_BITMAP;

    BITMAPINFO * newBitmapInfo = malloc(sizeof(BITMAPINFO));
    memset(newBitmapInfo, 0, sizeof(BITMAPINFO));
    newBitmapInfo->bmiHeader.biBitCount = (WORD) nBitCount;
    newBitmapInfo->bmiHeader.biClrUsed = 0;
    newBitmapInfo->bmiHeader.biWidth = nWidth;
    newBitmapInfo->bmiHeader.biHeight = -nHeight;
    newBitmapInfo->bmiHeader.biPlanes = (WORD) nPlanes;
    newHBITMAP->bitmapInfo = newBitmapInfo;
    newHBITMAP->bitmapInfoHeader = (BITMAPINFOHEADER *)newBitmapInfo;

    size_t stride = (size_t)(4 * ((newBitmapInfo->bmiHeader.biWidth * newBitmapInfo->bmiHeader.biBitCount + 31) / 32));
    size_t size = abs(newBitmapInfo->bmiHeader.biHeight) * stride;
    newBitmapInfo->bmiHeader.biSizeImage = (DWORD) size;
    VOID * bitmapBits = malloc(size);
    memset(bitmapBits, 0, size);
    newHBITMAP->bitmapBits = bitmapBits;
    return newHBITMAP;
}
HBITMAP CreateDIBitmap( HDC hdc, CONST BITMAPINFOHEADER *pbmih, DWORD flInit, CONST VOID *pjBits, CONST BITMAPINFO *pbmi, UINT iUsage) {
    PAINT_LOGD("PAINT CreateDIBitmap()");

    HGDIOBJ newHBITMAP = (HGDIOBJ)malloc(sizeof(_HGDIOBJ));
    memset(newHBITMAP, 0, sizeof(_HGDIOBJ));
    newHBITMAP->handleType = HGDIOBJ_TYPE_BITMAP;

    BITMAPINFO * newBitmapInfo = malloc(sizeof(BITMAPINFO));
    memcpy(newBitmapInfo, pbmi, sizeof(BITMAPINFO));
    newHBITMAP->bitmapInfo = newBitmapInfo;
    newHBITMAP->bitmapInfoHeader = (BITMAPINFOHEADER *)newBitmapInfo;

    size_t stride = (size_t)(4 * ((newBitmapInfo->bmiHeader.biWidth * newBitmapInfo->bmiHeader.biBitCount + 31) / 32));
    size_t size = newBitmapInfo->bmiHeader.biSizeImage ?
                    newBitmapInfo->bmiHeader.biSizeImage :
                  abs(newBitmapInfo->bmiHeader.biHeight) * stride;
    VOID * bitmapBits = malloc(size);
    memcpy(bitmapBits, pjBits, size);
    newHBITMAP->bitmapBits = bitmapBits;
    return newHBITMAP;
}
HBITMAP CreateDIBSection(HDC hdc, CONST BITMAPINFO *pbmi, UINT usage, VOID **ppvBits, HANDLE hSection, DWORD offset) {
    PAINT_LOGD("PAINT CreateDIBitmap()");

    HGDIOBJ newHBITMAP = (HGDIOBJ)malloc(sizeof(_HGDIOBJ));
    memset(newHBITMAP, 0, sizeof(_HGDIOBJ));
    newHBITMAP->handleType = HGDIOBJ_TYPE_BITMAP;

    size_t bitmapInfoSize = sizeof(BITMAPINFO);
    if(pbmi->bmiHeader.biClrUsed > 0)
        bitmapInfoSize += sizeof(RGBQUAD) * min(pbmi->bmiHeader.biClrUsed, 2^16);
    BITMAPINFO * newBitmapInfo = malloc(bitmapInfoSize);
    memcpy(newBitmapInfo, pbmi, bitmapInfoSize);
    newHBITMAP->bitmapInfo = newBitmapInfo;
    newHBITMAP->bitmapInfoHeader = (BITMAPINFOHEADER *)newBitmapInfo;

    // For DIB_RGB_COLORS only
    size_t stride = (size_t)(4 * ((newBitmapInfo->bmiHeader.biWidth * newBitmapInfo->bmiHeader.biBitCount + 31) / 32));
    size_t size = newBitmapInfo->bmiHeader.biSizeImage ?
                  newBitmapInfo->bmiHeader.biSizeImage :
                  abs(newBitmapInfo->bmiHeader.biHeight) * stride;
    newHBITMAP->bitmapBits = malloc(size);

    memset((void *) newHBITMAP->bitmapBits, 0, size);
    *ppvBits = (void *) newHBITMAP->bitmapBits;
    return newHBITMAP;
}
HBITMAP CreateCompatibleBitmap( HDC hdc, int cx, int cy) {
    PAINT_LOGD("PAINT CreateDIBitmap()");

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
    size_t size = abs(newBitmapInfo->bmiHeader.biHeight) * stride;
    newBitmapInfo->bmiHeader.biSizeImage = (DWORD) size;
    newHBITMAP->bitmapBits = malloc(size);
    memset((void *) newHBITMAP->bitmapBits, 0, size);
    return newHBITMAP;
}
int GetDIBits(HDC hdc, HBITMAP hbm, UINT start, UINT cLines, LPVOID lpvBits, LPBITMAPINFO lpbmi, UINT usage) {
    //TODO Not sure at all for this function
    if(hbm && lpbmi) {
        CONST BITMAPINFO *pbmi = hbm->bitmapInfo;
        if(!lpvBits) {
            size_t bitmapInfoSize = sizeof(BITMAPINFOHEADER);
            memcpy(lpbmi, pbmi, bitmapInfoSize);
        } else {
            // We consider the source and destination dib with the same format
            size_t stride = (size_t)(4 * ((pbmi->bmiHeader.biWidth * pbmi->bmiHeader.biBitCount + 31) / 32));
            VOID * sourceDibBits = (VOID *)hbm->bitmapBits;
            VOID * destinationDibBits = lpvBits;
            for(int y = 0; y < cLines; y++) {
                size_t lineSize = (start + y) * stride;
                memcpy(destinationDibBits + lineSize, sourceDibBits + lineSize, stride);
            }
        }
    }
    return 0;
}
COLORREF GetPixel(HDC hdc, int x ,int y) {
    HBITMAP hBitmapSource = hdc->selectedBitmap;
    void * pixelsSource = (void *) hBitmapSource->bitmapBits;

    BOOL reverseHeight = hBitmapSource->bitmapInfoHeader->biHeight < 0;

    int sourceWidth = hBitmapSource->bitmapInfoHeader->biWidth;
    int sourceHeight = abs(hBitmapSource->bitmapInfoHeader->biHeight); // Can be < 0

    int sourceBitCount = hBitmapSource->bitmapInfoHeader->biBitCount;
    int sourceStride = 4 * ((sourceWidth * hBitmapSource->bitmapInfoHeader->biBitCount + 31) / 32);

    x -= hdc->windowOriginX;
    y -= hdc->windowOriginY;

    if(!reverseHeight)
        y = sourceHeight - 1 - y;

    HPALETTE palette = hdc->realizedPalette;
    if(!palette)
        palette = hdc->selectedPalette;
    PALETTEENTRY * palPalEntry = palette && palette->paletteLog && palette->paletteLog->palPalEntry ?
                                 palette->paletteLog->palPalEntry : NULL;
    if(!palPalEntry && sourceBitCount <= 8 && hBitmapSource->bitmapInfoHeader->biClrUsed > 0) {
        palPalEntry = (PALETTEENTRY *)hBitmapSource->bitmapInfo->bmiColors;
    }
    COLORREF resultColor = CLR_INVALID; // 0xAABBGGRR

    if(x >= 0 && y >= 0 && x < sourceWidth && y < sourceHeight) {
        BYTE * sourcePixel = pixelsSource + sourceStride * y;

        // -> ARGB_8888
        switch (sourceBitCount) {
            case 1:
                //TODO
                break;
            case 4: {
                sourcePixel += x >> 2;
                BYTE colorIndex = (x & 0x1 ? sourcePixel[0] & (BYTE)0x0F : sourcePixel[0] >> 4);
                if (palPalEntry) {
                    resultColor = 0xFF000000 | RGB(palPalEntry[colorIndex].peRed, palPalEntry[colorIndex].peGreen, palPalEntry[colorIndex].peBlue);
                } else {
                    resultColor = 0xFF000000 | RGB(colorIndex, colorIndex, colorIndex);
                }
                break;
            }
            case 8: {
                sourcePixel += x;
                BYTE colorIndex = sourcePixel[0];
                if (palPalEntry) {
                    resultColor = 0xFF000000 | RGB(palPalEntry[colorIndex].peRed, palPalEntry[colorIndex].peGreen, palPalEntry[colorIndex].peBlue);
                } else {
                    resultColor = 0xFF000000 | RGB(sourcePixel[0], sourcePixel[0], sourcePixel[0]);
                }
                break;
            }
            case 24:
                sourcePixel += 3 * x;
                resultColor = 0xFF000000 | RGB(sourcePixel[2], sourcePixel[1], sourcePixel[0]);
                break;
            case 32:
                sourcePixel += 4 * x;
                resultColor = 0xFF000000 | RGB(sourcePixel[2], sourcePixel[1], sourcePixel[0]);
                break;
            default:
                break;
        }
    }

    return resultColor;
}
BOOL SetRect(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom) {
    //TODO
    return 0;
}
BOOL SetRectEmpty(LPRECT lprc) {
    if(lprc) {
        lprc->top = 0;
        lprc->bottom = 0;
        lprc->left = 0;
        lprc->right = 0;
        return TRUE;
    }
    return FALSE;
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
    if(lpPaint) {
        memset(lpPaint, 0, sizeof(PAINTSTRUCT));
        lpPaint->fErase = TRUE;
        lpPaint->hdc = CreateCompatibleDC(NULL); //hWnd->windowDC);
        lpPaint->rcPaint.right = (short) nBackgroundW;
        lpPaint->rcPaint.bottom = (short) nBackgroundH;
        return lpPaint->hdc;
    }
    return NULL;
}
BOOL EndPaint(HWND hWnd, CONST PAINTSTRUCT *lpPaint) {
    //mainViewUpdateCallback(); //TODO May be not needed
    DeleteDC(lpPaint->hdc);
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

#if defined WIN32_TIMER_THREAD
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
pthread_mutex_t timerEventsLock;
static void initTimer() {
    for (int i = 0; i < MAX_TIMER; ++i) {
        timerEvents[i].valid = FALSE;
    }
}

void deleteTimeEvent(UINT uTimerID) {
    pthread_mutex_lock(&timerEventsLock);
    timer_delete(timerEvents[uTimerID - 1].timer);
    timerEvents[uTimerID - 1].valid = FALSE;
    pthread_mutex_unlock(&timerEventsLock);
}

MMRESULT timeKillEvent(UINT uTimerID) {
    TIMER_LOGD("timeKillEvent(uTimerID: [%d])", uTimerID);
    deleteTimeEvent(uTimerID);
    return 0; //No error
}

void timerCallback(int timerId) {
    if(timerId >= 0 && timerId < MAX_TIMER && timerEvents[timerId].valid) {
        timerEvents[timerId].fptc((UINT) (timerId + 1), 0, (DWORD) timerEvents[timerId].dwUser, 0, 0);

        if(timerEvents[timerId].fuEvent == TIME_ONESHOT) {
            TIMER_LOGD("timerCallback remove timer uTimerID [%d]", timerId + 1);
            deleteTimeEvent((UINT) (timerId + 1));
        }

        jniDetachCurrentThread();
    }
}
MMRESULT timeSetEvent(UINT uDelay, UINT uResolution, LPTIMECALLBACK fptc, DWORD_PTR dwUser, UINT fuEvent) {
    TIMER_LOGD("timeSetEvent(uDelay: %d, fuEvent: %d)", uDelay, fuEvent);

    pthread_mutex_lock(&timerEventsLock);

    // Find a timer id
    int timerId = -1;
    for (int i = 0; i < MAX_TIMER; ++i) {
        if(!timerEvents[i].valid) {
            timerId = i;
            break;
        }
    }
    if(timerId == -1) {
        LOGD("timeSetEvent() ERROR: No more timer available");
        pthread_mutex_unlock(&timerEventsLock);
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

    // CLOCK_REALTIME 0 OK but X intervals only
    // CLOCK_MONOTONIC 1 OK but X intervals only
    // CLOCK_PROCESS_CPUTIME_ID 2 NOTOK, not working with PERIODIC!
    // CLOCK_THREAD_CPUTIME_ID 3 NOTOK
    // CLOCK_MONOTONIC_RAW 4 NOTOK
    // CLOCK_REALTIME_COARSE 5 NOTOK
    // CLOCK_MONOTONIC_COARSE 6 NOTOK
    // CLOCK_BOOTTIME 7 OK but X intervals only
    // CLOCK_REALTIME_ALARM 8 NOTOK EPERM
    // CLOCK_BOOTTIME_ALARM 9 NOTOK EPERM
    // CLOCK_SGI_CYCLE 10 NOTOK EINVAL
    // CLOCK_TAI 11

    if (timer_create(CLOCK_REALTIME, &sev, timer) == -1) {
        LOGD("timeSetEvent() ERROR in timer_create, errno: %d (EAGAIN 11 / EINVAL 22 / ENOMEM 12)", errno);
        //        EAGAIN Temporary error during kernel allocation of timer structures.
        //        EINVAL Clock ID, sigev_notify, sigev_signo, or sigev_notify_thread_id is invalid.
        //        ENOMEM Could not allocate memory.
        pthread_mutex_unlock(&timerEventsLock);
        return NULL;
    }

    long freq_nanosecs = uDelay;
    struct itimerspec its;
    its.it_value.tv_sec = freq_nanosecs / 1000;
    its.it_value.tv_nsec = (freq_nanosecs % 1000) * 1000000;
    if(fuEvent == TIME_PERIODIC) {
        its.it_interval.tv_sec = its.it_value.tv_sec;
        its.it_interval.tv_nsec = its.it_value.tv_nsec;
    } else /*if(fuEvent == TIME_ONESHOT)*/ {
        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 0;
    }
    if (timer_settime(timerEvents[timerId].timer, 0, &its, NULL) == -1) {
        LOGD("timeSetEvent() ERROR in timer_settime, errno: %d (EFAULT 14 / EINVAL 22)", errno);
        //        EFAULT new_value, old_value, or curr_value is not a valid pointer.
        //        EINVAL timerid is invalid. Or new_value.it_value is negative; or new_value.it_value.tv_nsec is negative or greater than 999,999,999.
        timer_delete(timerEvents[timerId].timer);
        pthread_mutex_unlock(&timerEventsLock);
        return NULL;
    }
    timerEvents[timerId].valid = TRUE;
    TIMER_LOGD("timeSetEvent() -> timerId+1: [%d]", timerId + 1);
    pthread_mutex_unlock(&timerEventsLock);
    return (MMRESULT) (timerId + 1); // No error
}
#else
struct timerEvent {
    int timerId;
    UINT uDelay;
    UINT uResolution;
    LPTIMECALLBACK fptc;
    DWORD_PTR dwUser;
    UINT fuEvent;

    ULONGLONG triggerTime;

    struct timerEvent * next;
    struct timerEvent * prev;
};
static struct timerEvent timersList;
static int lastTimerId;
static pthread_mutex_t timerEventsLock;
static pthread_t timerThreadId;
static BOOL timerThreadToEnd;

static void initTimer() {
    timersList.next = &timersList;
    timersList.prev = &timersList;
    timerThreadId = 0;
    timerThreadToEnd = TRUE;
    lastTimerId = 0;
}

MMRESULT timeKillEvent(UINT uTimerID) {
    TIMER_LOGD("timeKillEvent(uTimerID: [%d])", uTimerID);

    pthread_mutex_lock(&timerEventsLock);

    struct timerEvent * nextTimer, * timerToFree = NULL;
    // Search the timer by id
    for (nextTimer = timersList.next; nextTimer != &timersList; nextTimer = nextTimer->next) {
        if(uTimerID == nextTimer->timerId) {
            // Remove the timer
            nextTimer->next->prev = nextTimer->prev;
            nextTimer->prev->next = nextTimer->next;
            timerToFree = nextTimer;
            break;
        }
    }
    if(timersList.next == &timersList) {
        // The list is empty
        // Leave the thread?
        timerThreadToEnd = TRUE;
    }
    pthread_mutex_unlock(&timerEventsLock);

    if(timerToFree)
        free(timerToFree);

    return 0; //No error
}

static void insertTimer(struct timerEvent * newTimer) {
    struct timerEvent * nextTimer;
    // Search where to insert this new timer
    for (nextTimer = timersList.next; nextTimer != &timersList; nextTimer = nextTimer->next) {
        if(newTimer->triggerTime < nextTimer->triggerTime)
            break;
    }
    // Insert this new timer
    newTimer->next = nextTimer;
    newTimer->prev = nextTimer->prev;
    nextTimer->prev->next = newTimer;
    nextTimer->prev = newTimer;
}

static void dumpTimers() {
    TIMER_LOGD("dumpTimers()");
    for (struct timerEvent * nextTimer = timersList.next; nextTimer != &timersList; nextTimer = nextTimer->next) {
        TIMER_LOGD("\ttimerId: %d (%x), uDelay: %d, dwUser: %d, fuEvent: %d, triggerTime: %ld)",
                nextTimer->timerId, nextTimer, nextTimer->uDelay, nextTimer->dwUser, nextTimer->fuEvent, nextTimer->triggerTime);
    }
}

static void timerThreadStart(LPVOID lpThreadParameter) {
    pthread_mutex_lock(&timerEventsLock);
    while (!timerThreadToEnd) {
        TIMER_LOGD("timerThreadStart() %ld", GetTickCount64());

        int sleep_time;
        for (;;) {
            struct timerEvent *timer = (timersList.next == &timersList ? NULL : timersList.next);
            if (!timer) {
                sleep_time = -1;
                break;
            }

            sleep_time = timer->triggerTime - GetTickCount64();
            if (sleep_time > 0)
                break;

            // Remove this timer from the linked list
            timer->next->prev = timer->prev;
            timer->prev->next = timer->next;

            struct timerEvent *timerToFree;
            if(timer->fuEvent == TIME_PERIODIC) {
                timer->triggerTime += timer->uDelay;
                /* Re-insert the timer */
                insertTimer(timer);
                timerToFree = NULL;
            } else
                timerToFree = timer;

            // Copy the timer data because we unlock the mutex!
            int timerId = timer->timerId;
            LPTIMECALLBACK fptc = timer->fptc;
            DWORD_PTR dwUser = timer->dwUser;

            pthread_mutex_unlock(&timerEventsLock);

            // Call the timer callback now!
            fptc((UINT)timerId, 0, (DWORD) dwUser, 0, 0);

            pthread_mutex_lock(&timerEventsLock);
            if(timerToFree)
                free(timerToFree);
        }

#if defined(TIMER_LOGD)
        dumpTimers();
#endif

        if (sleep_time < 0)
            break;
        if (sleep_time == 0)
            continue;

        pthread_mutex_unlock(&timerEventsLock);
        Sleep(sleep_time);
        pthread_mutex_lock(&timerEventsLock);
    }
    timerThreadId = 0;
    pthread_mutex_unlock(&timerEventsLock);
    jniDetachCurrentThread();
}

MMRESULT timeSetEvent(UINT uDelay, UINT uResolution, LPTIMECALLBACK fptc, DWORD_PTR dwUser, UINT fuEvent) {
    TIMER_LOGD("timeSetEvent(uDelay: %d, fuEvent: %d)", uDelay, fuEvent);

    struct timerEvent * newTimer = (struct timerEvent *)malloc(sizeof(struct timerEvent));

    pthread_mutex_lock(&timerEventsLock);

    newTimer->timerId = ++lastTimerId;
    newTimer->uDelay = uDelay; // In ms
    newTimer->uResolution = uResolution;
    newTimer->fptc = fptc;
    newTimer->dwUser = dwUser;
    newTimer->fuEvent = fuEvent; // TIME_ONESHOT / TIME_PERIODIC

    newTimer->triggerTime = GetTickCount64() + (ULONGLONG)uDelay;

    insertTimer(newTimer);

    timerThreadToEnd = FALSE;

    if(!timerThreadId) {
        // If not yet created, create the thread which will handle all the timers
        if (pthread_create(&timerThreadId, NULL, (void *(*)(void *)) timerThreadStart, NULL) != 0) {
            LOGD("timeSetEvent() ERROR in pthread_create, errno: %d (EAGAIN 11 / EINVAL 22 / EPERM 1)", errno);
            //        EAGAIN Insufficient resources to create another thread.
            //        EINVAL Invalid settings in attr.
            //        ENOMEM No permission to set the scheduling policy and parameters specified in attr.
            pthread_mutex_unlock(&timerEventsLock);
            return NULL;
        }
    }

    TIMER_LOGD("timeSetEvent() -> timerId: [%d]", newTimer->timerId);
    pthread_mutex_unlock(&timerEventsLock);
    return (MMRESULT) newTimer->timerId; // No error
}
#endif

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
        lpSystemTime->wYear = (WORD) (1900 + tm.tm_year);
        lpSystemTime->wMonth = (WORD) (1 + tm.tm_mon);
        lpSystemTime->wDayOfWeek = (WORD) (tm.tm_wday);
        lpSystemTime->wDay = (WORD) (tm.tm_mday);
        lpSystemTime->wHour = (WORD) (tm.tm_hour);
        lpSystemTime->wMinute = (WORD) (tm.tm_min);
        lpSystemTime->wSecond = (WORD) (tm.tm_sec);
        lpSystemTime->wMilliseconds = (WORD) (ts.tv_nsec / 1e6);
    }
}
ULONGLONG GetTickCount64(VOID) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now))
        return 0;
    return (ULONGLONG) ((ULONGLONG)now.tv_sec * 1000UL + (ULONGLONG)now.tv_nsec / 1000000UL);
}
DWORD GetTickCount(VOID) {
    return (DWORD)GetTickCount64();
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
    if(currentDialogBoxMode == DialogBoxMode_GET_USRPRG32
    || currentDialogBoxMode == DialogBoxMode_GET_USRPRG42
    || currentDialogBoxMode == DialogBoxMode_SET_USRPRG32
    || currentDialogBoxMode == DialogBoxMode_SET_USRPRG42) {
        itemDataCount = 0;
        selItemDataCount = 0;
    }
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
#ifndef IDD_USERCODE
    #define IDD_USERCODE 121
#endif
INT_PTR DialogBoxParam(HINSTANCE hInstance, LPCTSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam) {
    //TODO
    if(lpTemplateName == MAKEINTRESOURCE(IDD_CHOOSEKML)) {
        if(chooseCurrentKmlMode == ChooseKmlMode_UNKNOWN) {
        } else if(chooseCurrentKmlMode == ChooseKmlMode_FILE_NEW) {
            lstrcpy(szCurrentKml, szChosenCurrentKml);
        } else if(chooseCurrentKmlMode == ChooseKmlMode_FILE_OPEN) {
            if(!getFirstKMLFilenameForType(Chipset.type, szCurrentKml, sizeof(szCurrentKml) / sizeof(szCurrentKml[0]))) {
                showAlert(_T("Cannot find the KML template file, sorry."), 0);
                return -1;
            }
//            else {
//                showAlert(_T("Cannot find the KML template file, so, try another one."), 0); //TODO is it right?
//            }
        }
    } else if(lpTemplateName == MAKEINTRESOURCE(IDD_KMLLOG)) {
        lpDialogFunc(NULL, WM_INITDIALOG, 0, 0);
    } else if(lpTemplateName == MAKEINTRESOURCE(IDD_USERCODE)) {
        if(currentDialogBoxMode == DialogBoxMode_GET_USRPRG32
        || currentDialogBoxMode == DialogBoxMode_GET_USRPRG42) {
            itemDataCount = 0;
            selItemDataCount = 0;
            lpDialogFunc(NULL, WM_INITDIALOG, 0, 0);
            LPTSTR lpLabel = labels;
            for(int i = 0; i < itemDataCount; i++) {
                _tcscpy(lpLabel, itemString[i]);
                lpLabel += _tcslen(itemString[i]) * sizeof(TCHAR);
                *lpLabel = 9; // Separator (TAB)
                lpLabel++;
            }
            *lpLabel = 0; // End of string
            lpDialogFunc(NULL, WM_COMMAND, IDCANCEL, 0);
        } else if(currentDialogBoxMode == DialogBoxMode_SET_USRPRG32
        || currentDialogBoxMode == DialogBoxMode_SET_USRPRG42) {
            itemDataCount = 0;
            lpDialogFunc(NULL, WM_INITDIALOG, 0, 0);
            lpDialogFunc(NULL, WM_COMMAND, IDOK, 0);
        }
    }
    return IDOK;
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

// IO

BOOL GetOverlappedResult(HANDLE hFile, LPOVERLAPPED lpOverlapped, LPDWORD lpNumberOfBytesTransferred, BOOL bWait) {
    //TODO
    return FALSE;
}
BOOL WaitCommEvent(HANDLE hFile, LPDWORD lpEvtMask, LPOVERLAPPED lpOverlapped) {
    //TODO
    return FALSE;
}
BOOL ClearCommError(HANDLE hFile, LPDWORD lpErrors, LPCOMSTAT lpStat) {
    //TODO
    return FALSE;
}
BOOL SetCommTimeouts(HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts) {
    //TODO
    return FALSE;
}
BOOL SetCommMask(HANDLE hFile, DWORD dwEvtMask) {
    //TODO
    return FALSE;
}
BOOL SetCommState(HANDLE hFile, LPDCB lpDCB) {
    //TODO
    return FALSE;
}
BOOL PurgeComm(HANDLE hFile, DWORD dwFlags) {
    //TODO
    return FALSE;
}
BOOL SetCommBreak(HANDLE hFile) {
    //TODO
    return FALSE;
}
BOOL ClearCommBreak(HANDLE hFile) {
    //TODO
    return FALSE;
}

int WSAGetLastError() {
    //TODO
    // Win9x break with WSAEINTR (a blocking socket call was canceled)
//    if(errno == ECANCELED)
//        return WSAEINTR;
    return 0;
}

int WSAStartup(WORD wVersionRequested, LPWSADATA lpWSAData) {
    return 0;
}

int WSACleanup() {
    return 0;
}