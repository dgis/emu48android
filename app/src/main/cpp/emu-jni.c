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

#include <jni.h>
#include <stdio.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/bitmap.h>

#include "core/pch.h"
#include "emu.h"
#include "core/io.h"
#include "core/kml.h"
#include "win32-layer.h"

extern AAssetManager * assetManager;
static jobject mainActivity = NULL;
jobject bitmapMainScreen = NULL;
AndroidBitmapInfo androidBitmapInfo;
//RECT mainViewRectangleToUpdate = { 0, 0, 0, 0 };
enum DialogBoxMode currentDialogBoxMode;
LPBYTE pbyRomBackup = NULL;
enum ChooseKmlMode chooseCurrentKmlMode;
TCHAR szChosenCurrentKml[MAX_PATH];
TCHAR szKmlLog[10240];
TCHAR szKmlLogBackup[10240];
TCHAR szKmlTitle[10240];
BOOL securityExceptionOccured;
BOOL kmlFileNotFound = FALSE;
BOOL settingsPort2en;
BOOL settingsPort2wr;
BOOL soundAvailable = FALSE;
BOOL soundEnabled = FALSE;
BOOL serialPortSlowDown = FALSE;



extern void win32Init();

extern void draw();
extern BOOL buttonDown(int x, int y);
extern void buttonUp(int x, int y);
extern void keyDown(int virtKey);
extern void keyUp(int virtKey);


JavaVM *java_machine;
JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    java_machine = vm;
    win32Init();
    return JNI_VERSION_1_6;
}

JNIEnv *getJNIEnvironment() {
    JNIEnv * jniEnv;
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
    return jniEnv;
}

int jniDetachCurrentThread() {
    jint ret = (*java_machine)->DetachCurrentThread(java_machine);
    return ret;
}

enum CALLBACK_TYPE {
    CALLBACK_TYPE_INVALIDATE = 0,
    CALLBACK_TYPE_WINDOW_RESIZE = 1
};

// https://stackoverflow.com/questions/9630134/jni-how-to-callback-from-c-or-c-to-java
int mainViewCallback(int type, int param1, int param2/*, const TCHAR * param3, const TCHAR * param4*/) {
    if (mainActivity) {
        JNIEnv *jniEnv = getJNIEnvironment();
        if(jniEnv) {
            jclass mainActivityClass = (*jniEnv)->GetObjectClass(jniEnv, mainActivity);
            if(mainActivityClass) {
	            jmethodID midStr = (*jniEnv)->GetMethodID(jniEnv, mainActivityClass, "updateCallback", "(III)I");
	            int result = (*jniEnv)->CallIntMethod(jniEnv, mainActivity, midStr, type, param1, param2);
                (*jniEnv)->DeleteLocalRef(jniEnv, mainActivityClass);
                //if(needDetach) ret = (*java_machine)->DetachCurrentThread(java_machine);
                return result;
            }
        }
    }
    return 0;
}

void mainViewUpdateCallback() {
//	if(!IsRectEmpty(&mainViewRectangleToUpdate)) {
//		int param1 = ((mainViewRectangleToUpdate.left & 0xFFFF) << 16) | (mainViewRectangleToUpdate.top & 0xFFFF);
//		int param2 = ((mainViewRectangleToUpdate.right & 0xFFFF) << 16) | (mainViewRectangleToUpdate.bottom & 0xFFFF);
//		mainViewCallback(CALLBACK_TYPE_INVALIDATE,
//		                 param1,
//		                 param2);
//		SetRectEmpty(&mainViewRectangleToUpdate);
//	} else
		mainViewCallback(CALLBACK_TYPE_INVALIDATE,
		                 0,
		                 0);
}

void mainViewResizeCallback(int x, int y) {
    mainViewCallback(CALLBACK_TYPE_WINDOW_RESIZE, x, y);

    JNIEnv * jniEnv;
    int ret = (*java_machine)->GetEnv(java_machine, (void **) &jniEnv, JNI_VERSION_1_6);
    if(jniEnv) {
        ret = AndroidBitmap_getInfo(jniEnv, bitmapMainScreen, &androidBitmapInfo);
        if (ret < 0) {
            LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
        }
    }
}

// Must be called in the main thread
int openFileFromContentResolver(const TCHAR * fileURL, int writeAccess) {
    int result = -1;
    JNIEnv *jniEnv = getJNIEnvironment();
    if(jniEnv) {
        jclass mainActivityClass = (*jniEnv)->GetObjectClass(jniEnv, mainActivity);
        if(mainActivityClass) {
            jmethodID midStr = (*jniEnv)->GetMethodID(jniEnv, mainActivityClass, "openFileFromContentResolver", "(Ljava/lang/String;I)I");
            jstring utfFileURL = (*jniEnv)->NewStringUTF(jniEnv, fileURL);
            result = (*jniEnv)->CallIntMethod(jniEnv, mainActivity, midStr, utfFileURL, writeAccess);
            (*jniEnv)->DeleteLocalRef(jniEnv, mainActivityClass);
        }
    }
    return result;
}
int openFileInFolderFromContentResolver(const TCHAR * filename, const TCHAR * folderURL, int writeAccess) {
    int result = -1;
    JNIEnv *jniEnv = getJNIEnvironment();
    if(jniEnv) {
        jclass mainActivityClass = (*jniEnv)->GetObjectClass(jniEnv, mainActivity);
        if(mainActivityClass) {
            jmethodID midStr = (*jniEnv)->GetMethodID(jniEnv, mainActivityClass, "openFileInFolderFromContentResolver", "(Ljava/lang/String;Ljava/lang/String;I)I");
            jstring utfFilename = (*jniEnv)->NewStringUTF(jniEnv, filename);
            jstring utfFolderURL = (*jniEnv)->NewStringUTF(jniEnv, folderURL);
            result = (*jniEnv)->CallIntMethod(jniEnv, mainActivity, midStr, utfFilename, utfFolderURL, writeAccess);
            (*jniEnv)->DeleteLocalRef(jniEnv, mainActivityClass);
        }
    }
    return result;
}
int closeFileFromContentResolver(int fd) {
    int result = -1;
    JNIEnv *jniEnv = getJNIEnvironment();
    if(jniEnv) {
        jclass mainActivityClass = (*jniEnv)->GetObjectClass(jniEnv, mainActivity);
        if(mainActivityClass) {
            jmethodID midStr = (*jniEnv)->GetMethodID(jniEnv, mainActivityClass, "closeFileFromContentResolver", "(I)I");
            result = (*jniEnv)->CallIntMethod(jniEnv, mainActivity, midStr, fd);
            (*jniEnv)->DeleteLocalRef(jniEnv, mainActivityClass);
        }
    }
    return result;
}

int showAlert(const TCHAR * messageText, int flags) {
    JNIEnv *jniEnv = getJNIEnvironment();
    if(jniEnv) {
        jclass mainActivityClass = (*jniEnv)->GetObjectClass(jniEnv, mainActivity);
        if(mainActivityClass) {
            jmethodID midStr = (*jniEnv)->GetMethodID(jniEnv, mainActivityClass, "showAlert", "(Ljava/lang/String;)V");
            jstring messageUTF = (*jniEnv)->NewStringUTF(jniEnv, messageText);
            (*jniEnv)->CallVoidMethod(jniEnv, mainActivity, midStr, messageUTF);
            (*jniEnv)->DeleteLocalRef(jniEnv, mainActivityClass);
        }
    }
    return IDOK;
}

void sendMenuItemCommand(int menuItem) {
    JNIEnv *jniEnv = getJNIEnvironment();
    if(jniEnv) {
        jclass mainActivityClass = (*jniEnv)->GetObjectClass(jniEnv, mainActivity);
        if(mainActivityClass) {
            jmethodID midStr = (*jniEnv)->GetMethodID(jniEnv, mainActivityClass, "sendMenuItemCommand", "(I)V");
            (*jniEnv)->CallVoidMethod(jniEnv, mainActivity, midStr, menuItem);
            (*jniEnv)->DeleteLocalRef(jniEnv, mainActivityClass);
        }
    }
}

TCHAR lastKMLFilename[MAX_PATH];

BOOL getFirstKMLFilenameForType(BYTE chipsetType) {
	JNIEnv *jniEnv = getJNIEnvironment();
	if(jniEnv) {
		jclass mainActivityClass = (*jniEnv)->GetObjectClass(jniEnv, mainActivity);
		if(mainActivityClass) {
			jmethodID midStr = (*jniEnv)->GetMethodID(jniEnv, mainActivityClass, "getFirstKMLFilenameForType", "(C)I");
			int result = (*jniEnv)->CallIntMethod(jniEnv, mainActivity, midStr, (char)chipsetType);
			(*jniEnv)->DeleteLocalRef(jniEnv, mainActivityClass);
			return result ? TRUE : FALSE;
		}
	}
	return FALSE;
}


void clipboardCopyText(const TCHAR * text) {
    JNIEnv *jniEnv = getJNIEnvironment();
    if(jniEnv) {
        jclass mainActivityClass = (*jniEnv)->GetObjectClass(jniEnv, mainActivity);
        if(mainActivityClass) {
            jmethodID midStr = (*jniEnv)->GetMethodID(jniEnv, mainActivityClass, "clipboardCopyText", "(Ljava/lang/String;)V");
            jint length = _tcslen(text);
            unsigned short * utf16String =  malloc(2 * (length + 1));
            for (int i = 0; i <= length; ++i)
                utf16String[i] = ((unsigned char *)text)[i];
            jstring messageUTF = (*jniEnv)->NewString(jniEnv, utf16String, length);
            (*jniEnv)->CallVoidMethod(jniEnv, mainActivity, midStr, messageUTF);
            (*jniEnv)->DeleteLocalRef(jniEnv, mainActivityClass);
            free(utf16String);
        }
    }
}
const TCHAR * clipboardPasteText() {
    JNIEnv *jniEnv = getJNIEnvironment();
    if(jniEnv) {
        jclass mainActivityClass = (*jniEnv)->GetObjectClass(jniEnv, mainActivity);
        if(mainActivityClass) {
            jmethodID midStr = (*jniEnv)->GetMethodID(jniEnv, mainActivityClass, "clipboardPasteText", "()Ljava/lang/String;");
            jobject result = (*jniEnv)->CallObjectMethod(jniEnv, mainActivity, midStr);
            (*jniEnv)->DeleteLocalRef(jniEnv, mainActivityClass);
            if(result) {
                const jchar *strReturn = (*jniEnv)->GetStringChars(jniEnv, result, 0);
                jsize length = (*jniEnv)->GetStringLength(jniEnv, result);
                TCHAR * pasteText = (TCHAR *) GlobalAlloc(0, length + 2);
                for (int i = 0; i <= length; ++i)
                    pasteText[i] = strReturn[i] & 0xFF;
                pasteText[length] = 0;
                (*jniEnv)->ReleaseStringChars(jniEnv, result, strReturn);
                return pasteText;
            }
        }
    }
    return NULL;
}

void performHapticFeedback() {
    JNIEnv *jniEnv = getJNIEnvironment();
    if(jniEnv) {
        jclass mainActivityClass = (*jniEnv)->GetObjectClass(jniEnv, mainActivity);
        if(mainActivityClass) {
            jmethodID midStr = (*jniEnv)->GetMethodID(jniEnv, mainActivityClass, "performHapticFeedback", "()V");
            (*jniEnv)->CallVoidMethod(jniEnv, mainActivity, midStr);
            (*jniEnv)->DeleteLocalRef(jniEnv, mainActivityClass);
        }
    }
}

void sendByteUdp(unsigned char byteSent) {
    JNIEnv *jniEnv = getJNIEnvironment();
    if(jniEnv) {
        jclass mainActivityClass = (*jniEnv)->GetObjectClass(jniEnv, mainActivity);
        if(mainActivityClass) {
            jmethodID midStr = (*jniEnv)->GetMethodID(jniEnv, mainActivityClass, "sendByteUdp", "(I)V");
            (*jniEnv)->CallVoidMethod(jniEnv, mainActivity, midStr, (int)byteSent);
            (*jniEnv)->DeleteLocalRef(jniEnv, mainActivityClass);
        }
    }
}

void setKMLIcon(int imageWidth, int imageHeight, LPBYTE buffer, int bufferSize) {
    JNIEnv *jniEnv = getJNIEnvironment();
    if(jniEnv) {
        jclass mainActivityClass = (*jniEnv)->GetObjectClass(jniEnv, mainActivity);
        if(mainActivityClass) {
            jmethodID midStr = (*jniEnv)->GetMethodID(jniEnv, mainActivityClass, "setKMLIcon", "(II[B)V");

            jbyteArray pixels = NULL;
            if(buffer) {
                pixels = (*jniEnv)->NewByteArray(jniEnv, bufferSize);
                (*jniEnv)->SetByteArrayRegion(jniEnv, pixels, 0, bufferSize, (jbyte *) buffer);
            }
            (*jniEnv)->CallVoidMethod(jniEnv, mainActivity, midStr, imageWidth, imageHeight, pixels);
            (*jniEnv)->DeleteLocalRef(jniEnv, mainActivityClass);
        }
    }
}

int openSerialPort(const TCHAR * serialPort) {
	int result = -1;
	JNIEnv *jniEnv = getJNIEnvironment();
	if(jniEnv) {
		jclass mainActivityClass = (*jniEnv)->GetObjectClass(jniEnv, mainActivity);
		if(mainActivityClass) {
			jmethodID midStr = (*jniEnv)->GetMethodID(jniEnv, mainActivityClass, "openSerialPort", "(Ljava/lang/String;)I");
			jstring utfFileURL = (*jniEnv)->NewStringUTF(jniEnv, serialPort);
			result = (*jniEnv)->CallIntMethod(jniEnv, mainActivity, midStr, utfFileURL);
			(*jniEnv)->DeleteLocalRef(jniEnv, mainActivityClass);
		}
	}
	return result;
}

int closeSerialPort(int serialPortId) {
	int result = -1;
	JNIEnv *jniEnv = getJNIEnvironment();
	if(jniEnv) {
		jclass mainActivityClass = (*jniEnv)->GetObjectClass(jniEnv, mainActivity);
		if(mainActivityClass) {
			jmethodID midStr = (*jniEnv)->GetMethodID(jniEnv, mainActivityClass, "closeSerialPort", "(I)I");
			result = (*jniEnv)->CallIntMethod(jniEnv, mainActivity, midStr, serialPortId);
			(*jniEnv)->DeleteLocalRef(jniEnv, mainActivityClass);
		}
	}
	return result;
}

int setSerialPortParameters(int serialPortId, int baudRate) {
	int result = -1;
	JNIEnv *jniEnv = getJNIEnvironment();
	if(jniEnv) {
		jclass mainActivityClass = (*jniEnv)->GetObjectClass(jniEnv, mainActivity);
		if(mainActivityClass) {
			jmethodID midStr = (*jniEnv)->GetMethodID(jniEnv, mainActivityClass, "setSerialPortParameters", "(II)I");
			result = (*jniEnv)->CallIntMethod(jniEnv, mainActivity, midStr, serialPortId, baudRate);
			(*jniEnv)->DeleteLocalRef(jniEnv, mainActivityClass);
		}
	}
	return result;
}

int readSerialPort(int serialPortId, LPBYTE buffer, int nNumberOfBytesToRead) {
	int nNumberOfReadBytes = 0;
	JNIEnv *jniEnv = getJNIEnvironment();
	if(jniEnv && buffer && nNumberOfBytesToRead > 0) {
		jclass mainActivityClass = (*jniEnv)->GetObjectClass(jniEnv, mainActivity);
		if(mainActivityClass) {
			jmethodID midStr = (*jniEnv)->GetMethodID(jniEnv, mainActivityClass, "readSerialPort", "(II)[B");
			jbyteArray readBytes = (jbyteArray)(*jniEnv)->CallObjectMethod(jniEnv, mainActivity, midStr, serialPortId, nNumberOfBytesToRead);
			nNumberOfReadBytes = (*jniEnv)->GetArrayLength(jniEnv, readBytes);
			jbyte* elements = (*jniEnv)->GetByteArrayElements(jniEnv, readBytes, NULL);
			if (elements) {
				for(int i = 0; i < nNumberOfReadBytes; i++)
					buffer[i] = elements[i];
				(*jniEnv)->ReleaseByteArrayElements(jniEnv, readBytes, elements, JNI_ABORT);
			}
			(*jniEnv)->DeleteLocalRef(jniEnv, mainActivityClass);
		}
	}
	return nNumberOfReadBytes;
}

int writeSerialPort(int serialPortId, LPBYTE buffer, int bufferSize) {
	int result = 0;
	JNIEnv *jniEnv = getJNIEnvironment();
	if(jniEnv) {
		jclass mainActivityClass = (*jniEnv)->GetObjectClass(jniEnv, mainActivity);
		if(mainActivityClass) {
			jmethodID midStr = (*jniEnv)->GetMethodID(jniEnv, mainActivityClass, "writeSerialPort", "(I[B)I");

			jbyteArray javaBuffer = NULL;
			if(buffer) {
				javaBuffer = (*jniEnv)->NewByteArray(jniEnv, bufferSize);
				(*jniEnv)->SetByteArrayRegion(jniEnv, javaBuffer, 0, bufferSize, (jbyte *) buffer);
			}
			result = (*jniEnv)->CallIntMethod(jniEnv, mainActivity, midStr, serialPortId, javaBuffer);
			(*jniEnv)->DeleteLocalRef(jniEnv, mainActivityClass);
		}
	}
	return result;
}

int serialPortPurgeComm(int serialPortId, int dwFlags) {
	int result = 0;
	JNIEnv *jniEnv = getJNIEnvironment();
	if(jniEnv) {
		jclass mainActivityClass = (*jniEnv)->GetObjectClass(jniEnv, mainActivity);
		if(mainActivityClass) {
			jmethodID midStr = (*jniEnv)->GetMethodID(jniEnv, mainActivityClass, "serialPortPurgeComm", "(II)I");
			result = (*jniEnv)->CallIntMethod(jniEnv, mainActivity, midStr, serialPortId);
			(*jniEnv)->DeleteLocalRef(jniEnv, mainActivityClass);
		}
	}
	return result;
}

int serialPortSetBreak(int serialPortId) {
	int result = 0;
	JNIEnv *jniEnv = getJNIEnvironment();
	if(jniEnv) {
		jclass mainActivityClass = (*jniEnv)->GetObjectClass(jniEnv, mainActivity);
		if(mainActivityClass) {
			jmethodID midStr = (*jniEnv)->GetMethodID(jniEnv, mainActivityClass, "serialPortSetBreak", "(I)I");
			result = (*jniEnv)->CallIntMethod(jniEnv, mainActivity, midStr, serialPortId);
			(*jniEnv)->DeleteLocalRef(jniEnv, mainActivityClass);
		}
	}
	return result;
}

int serialPortClearBreak(int serialPortId) {
	int result = 0;
	JNIEnv *jniEnv = getJNIEnvironment();
	if(jniEnv) {
		jclass mainActivityClass = (*jniEnv)->GetObjectClass(jniEnv, mainActivity);
		if(mainActivityClass) {
			jmethodID midStr = (*jniEnv)->GetMethodID(jniEnv, mainActivityClass, "serialPortClearBreak", "(I)I");
			result = (*jniEnv)->CallIntMethod(jniEnv, mainActivity, midStr, serialPortId);
			(*jniEnv)->DeleteLocalRef(jniEnv, mainActivityClass);
		}
	}
	return result;
}


JNIEXPORT void JNICALL Java_org_emulator_calculator_NativeLib_start(JNIEnv *env, jobject thisz, jobject assetMgr, jobject activity) {

    chooseCurrentKmlMode = ChooseKmlMode_UNKNOWN;
    szChosenCurrentKml[0] = '\0';

    mainActivity = (*env)->NewGlobalRef(env, activity);

    assetManager = AAssetManager_fromJava(env, assetMgr);



    // OnCreate
    InitializeCriticalSection(&csGDILock);
    InitializeCriticalSection(&csLcdLock);
    InitializeCriticalSection(&csKeyLock);
    InitializeCriticalSection(&csIOLock);
    InitializeCriticalSection(&csT1Lock);
    InitializeCriticalSection(&csT2Lock);
    InitializeCriticalSection(&csTxdLock);
    InitializeCriticalSection(&csRecvLock);
    InitializeCriticalSection(&csSlowLock);
    InitializeCriticalSection(&csDbgLock);





    DWORD dwThreadId;

    // read emulator settings
    GetCurrentDirectory(ARRAYSIZEOF(szCurrentDirectory),szCurrentDirectory);
    //ReadSettings();
    //bRomWriteable = FALSE;

    _tcscpy(szCurrentDirectory, "");
    _tcscpy(szEmuDirectory, "assets/calculators/");
    _tcscpy(szRomDirectory, "assets/calculators/");
    _tcscpy(szPort2Filename, "");

    // initialization
    QueryPerformanceFrequency(&lFreq);		// init high resolution counter
    QueryPerformanceCounter(&lAppStart);

    hWnd = CreateWindow();
    //hWindowDC = CreateCompatibleDC(NULL);
    hWindowDC = GetDC(hWnd);



    szCurrentKml[0] = 0;					// no KML file selected
    SetSpeed(bRealSpeed);					// set speed
	//MruInit(4);								// init MRU entries

    // create auto event handle
    hEventShutdn = CreateEvent(NULL,FALSE,FALSE,NULL);
    if (hEventShutdn == NULL)
    {
        AbortMessage(_T("Event creation failed."));
//		DestroyWindow(hWnd);
        return;
    }

    nState     = SM_RUN;					// init state must be <> nNextState
    nNextState = SM_INVALID;				// go into invalid state
    hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&WorkerThread, NULL, CREATE_SUSPENDED, &dwThreadId);
    if (hThread == NULL)
    {
        CloseHandle(hEventShutdn);			// close event handle
        AbortMessage(_T("Thread creation failed."));
//		DestroyWindow(hWnd);
        return;
    }

    soundEnabled = soundAvailable = SoundOpen(uWaveDevId);					// open waveform-audio output device

    ResumeThread(hThread);					// start thread
    while (nState!=nNextState) Sleep(0);	// wait for thread initialized
}

JNIEXPORT void JNICALL Java_org_emulator_calculator_NativeLib_stop(JNIEnv *env, jobject thisz) {

    if (hThread)
		SwitchToState(SM_RETURN);	// exit emulation thread

    ReleaseDC(hWnd, hWindowDC);
    DestroyWindow(hWnd);
	hWindowDC = NULL;						// hWindowDC isn't valid any more
	hWnd = NULL;

	// Prevent crash+++
	// FORTIFY: pthread_mutex_destroy called on a destroyed mutex (0x<sanitized>)
	//  DeleteCriticalSection(&csGDILock);
	//	DeleteCriticalSection(&csLcdLock);
	//	DeleteCriticalSection(&csKeyLock);
	//	DeleteCriticalSection(&csIOLock);
	//	DeleteCriticalSection(&csT1Lock);
	//	DeleteCriticalSection(&csT2Lock);
	//	DeleteCriticalSection(&csTxdLock);
	//	DeleteCriticalSection(&csRecvLock);
	//	DeleteCriticalSection(&csSlowLock);
	//	DeleteCriticalSection(&csDbgLock);
	// Prevent crash---

    SoundClose();							// close waveform-audio output device
    soundEnabled = FALSE;

    if(bitmapMainScreen) {
        (*env)->DeleteGlobalRef(env, bitmapMainScreen);
        bitmapMainScreen = NULL;
    }
}


JNIEXPORT void JNICALL Java_org_emulator_calculator_NativeLib_changeBitmap(JNIEnv *env, jobject thisz, jobject bitmapMainScreen0) {

    if(bitmapMainScreen) {
        (*env)->DeleteGlobalRef(env, bitmapMainScreen);
        bitmapMainScreen = NULL;
    }

    bitmapMainScreen = (*env)->NewGlobalRef(env, bitmapMainScreen0);
    int ret = AndroidBitmap_getInfo(env, bitmapMainScreen, &androidBitmapInfo);
    if (ret < 0) {
        LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
    }
}

JNIEXPORT void JNICALL Java_org_emulator_calculator_NativeLib_draw(JNIEnv *env, jobject thisz) {
    draw();
}
JNIEXPORT jboolean JNICALL Java_org_emulator_calculator_NativeLib_buttonDown(JNIEnv *env, jobject thisz, jint x, jint y) {
    return buttonDown(x, y) ? JNI_TRUE : JNI_FALSE;
}
JNIEXPORT void JNICALL Java_org_emulator_calculator_NativeLib_buttonUp(JNIEnv *env, jobject thisz, jint x, jint y) {
    buttonUp(x, y);
}
JNIEXPORT void JNICALL Java_org_emulator_calculator_NativeLib_keyDown(JNIEnv *env, jobject thisz, jint virtKey) {
    keyDown(virtKey);
}
JNIEXPORT void JNICALL Java_org_emulator_calculator_NativeLib_keyUp(JNIEnv *env, jobject thisz, jint virtKey) {
    keyUp(virtKey);
}



JNIEXPORT jboolean JNICALL Java_org_emulator_calculator_NativeLib_isDocumentAvailable(JNIEnv *env, jobject thisz) {
    return bDocumentAvail ? JNI_TRUE : JNI_FALSE;
}
JNIEXPORT jstring JNICALL Java_org_emulator_calculator_NativeLib_getCurrentFilename(JNIEnv *env, jobject thisz) {
    jstring result = (*env)->NewStringUTF(env, szCurrentFilename);
    return result;
}

JNIEXPORT jint JNICALL Java_org_emulator_calculator_NativeLib_getCurrentModel(JNIEnv *env, jobject thisz) {
    return cCurrentRomType;
}

JNIEXPORT jboolean JNICALL Java_org_emulator_calculator_NativeLib_isBackup(JNIEnv *env, jobject thisz) {
    return (jboolean) (bBackup ? JNI_TRUE : JNI_FALSE);
}

JNIEXPORT jstring JNICALL Java_org_emulator_calculator_NativeLib_getKMLLog(JNIEnv *env, jobject thisz) {
    jstring result = (*env)->NewStringUTF(env, szKmlLog);
    return result;
}

JNIEXPORT jstring JNICALL Java_org_emulator_calculator_NativeLib_getKMLTitle(JNIEnv *env, jobject thisz) {
    jstring result = (*env)->NewStringUTF(env, szKmlTitle);
    return result;
}

JNIEXPORT jstring JNICALL Java_org_emulator_calculator_NativeLib_getCurrentKml(JNIEnv *env, jobject thisz) {
	jstring result = (*env)->NewStringUTF(env, szCurrentKml);
	return result;
}

JNIEXPORT void JNICALL Java_org_emulator_calculator_NativeLib_setCurrentKml(JNIEnv *env, jobject thisz, jstring currentKml) {
	const char *currentKmlUTF8 = (*env)->GetStringUTFChars(env, currentKml, NULL);
	_tcscpy(szCurrentKml, currentKmlUTF8);
	(*env)->ReleaseStringUTFChars(env, currentKml, currentKmlUTF8);
}

JNIEXPORT jstring JNICALL Java_org_emulator_calculator_NativeLib_getEmuDirectory(JNIEnv *env, jobject thisz) {
	jstring result = (*env)->NewStringUTF(env, szEmuDirectory);
	return result;
}

JNIEXPORT void JNICALL Java_org_emulator_calculator_NativeLib_setEmuDirectory(JNIEnv *env, jobject thisz, jstring emuDirectory) {
	const char *emuDirectoryUTF8 = (*env)->GetStringUTFChars(env, emuDirectory, NULL);
	_tcscpy(szEmuDirectory, emuDirectoryUTF8);
	_tcscpy(szRomDirectory, emuDirectoryUTF8);
	(*env)->ReleaseStringUTFChars(env, emuDirectory, emuDirectoryUTF8);
}

JNIEXPORT jboolean JNICALL Java_org_emulator_calculator_NativeLib_getPort1Plugged(JNIEnv *env, jobject thisz) {
    return (jboolean) ((Chipset.cards_status & PORT1_PRESENT) != 0);
}

JNIEXPORT jboolean JNICALL Java_org_emulator_calculator_NativeLib_getPort1Writable(JNIEnv *env, jobject thisz) {
    return (jboolean) ((Chipset.cards_status & PORT1_WRITE) != 0);
}

JNIEXPORT jboolean JNICALL Java_org_emulator_calculator_NativeLib_getSoundEnabled(JNIEnv *env, jobject thisz) {
    return (jboolean) soundAvailable;
}

JNIEXPORT jint JNICALL Java_org_emulator_calculator_NativeLib_getGlobalColor(JNIEnv *env, jobject thisz) {
    return (jint) dwTColor;
}

JNIEXPORT jint JNICALL Java_org_emulator_calculator_NativeLib_getMacroState(JNIEnv *env, jobject thisz) {
    return nMacroState;
}


JNIEXPORT jint JNICALL Java_org_emulator_calculator_NativeLib_onFileNew(JNIEnv *env, jobject thisz, jstring kmlFilename, jstring kmlFolder) {
    if (bDocumentAvail) {
        SwitchToState(SM_INVALID);
        if(bAutoSave) {
            SaveDocument();
        }
    }

    const char *filenameUTF8 = (*env)->GetStringUTFChars(env, kmlFilename, NULL);
    chooseCurrentKmlMode = ChooseKmlMode_FILE_NEW;
    _tcscpy(szChosenCurrentKml, filenameUTF8);
    (*env)->ReleaseStringUTFChars(env, kmlFilename, filenameUTF8);

	TCHAR * documentScheme = _T("document:");
	TCHAR * documentSchemeFound = _tcsstr(szChosenCurrentKml, documentScheme);
    if(kmlFolder) {
	    const char *kmlFolderUTF8 = (*env)->GetStringUTFChars(env, kmlFolder, NULL);
        // The folder URL is separated from the script filename and comes from the JSON settings in the state file.
	    _tcscpy(szEmuDirectory, kmlFolderUTF8);
	    _tcscpy(szRomDirectory, kmlFolderUTF8);
	    (*env)->ReleaseStringUTFChars(env, kmlFolder, kmlFolderUTF8);
    } else if(documentSchemeFound) {
	    // Keep the compatibility by allowing to put the KML folder combined with the KML script filename with a document: scheme.
	    _tcscpy(szEmuDirectory, szChosenCurrentKml + _tcslen(documentScheme) * sizeof(TCHAR));
        TCHAR * filename = _tcschr(szEmuDirectory, _T('|'));
	    if(filename)
            *filename = _T('\0');
	    _tcscpy(szRomDirectory, szEmuDirectory);
    } else {
        _tcscpy(szEmuDirectory, "assets/calculators/");
        _tcscpy(szRomDirectory, "assets/calculators/");
    }

    BOOL result = NewDocument();

    chooseCurrentKmlMode = ChooseKmlMode_UNKNOWN;

    if(result) {
        mainViewResizeCallback(nBackgroundW, nBackgroundH);
        draw();

        if (bStartupBackup) SaveBackup();        // make a RAM backup at startup

        if (pbyRom) SwitchToState(SM_RUN);
    }
    return result;
}
JNIEXPORT jint JNICALL Java_org_emulator_calculator_NativeLib_onFileOpen(JNIEnv *env, jobject thisz, jstring stateFilename, jstring kmlFolder) {
    if (bDocumentAvail) {
        SwitchToState(SM_INVALID);
        if(bAutoSave) {
            SaveDocument();
        }
    }
    const char *stateFilenameUTF8 = (*env)->GetStringUTFChars(env, stateFilename, NULL);
    _tcscpy(szBufferFilename, stateFilenameUTF8);
	(*env)->ReleaseStringUTFChars(env, stateFilename, stateFilenameUTF8);

	chooseCurrentKmlMode = ChooseKmlMode_FILE_OPEN;
	if(kmlFolder) {
		const char *kmlFolderUTF8 = (*env)->GetStringUTFChars(env, kmlFolder, NULL);
		// The folder URL is separated from the script filename (not in the document: URL) and comes from the JSON settings in the state file.
		_tcscpy(szEmuDirectory, kmlFolderUTF8);
		_tcscpy(szRomDirectory, kmlFolderUTF8);
		(*env)->ReleaseStringUTFChars(env, kmlFolder, kmlFolderUTF8);
		chooseCurrentKmlMode = ChooseKmlMode_FILE_OPEN_WITH_FOLDER;
	} else {
		// We are loading a KML script from the embedded asset folder inside the Android App.
		// We directly set the variable "szEmuDirectory"/"szRomDirectory" and "szCurrentAssetDirectory" with the KML folder
		// which contain the script and its dependencies like the includes, the images and the ROMs.
		_tcscpy(szEmuDirectory, "assets/calculators/");
		_tcscpy(szRomDirectory, "assets/calculators/");
	}

	kmlFileNotFound = FALSE;
    lastKMLFilename[0] = '\0';
	// It is to open a new document, so we will trick the next KillKML() to prevent to UnmapROM()!!!
	// pbyRom is then restored in the unused win32 GetKeyboardLayoutName(), just after KillKML() but before the final InitKML(). Crazy!
	if (pbyRom) {
		pbyRomBackup = pbyRom;
		pbyRom = NULL;
	}
    BOOL result = OpenDocument(szBufferFilename);
    if(pbyRomBackup) pbyRomBackup = NULL;
    chooseCurrentKmlMode = ChooseKmlMode_UNKNOWN;
    mainViewResizeCallback(nBackgroundW, nBackgroundH);
    if(result) {
        if (pbyRom) SwitchToState(SM_RUN);
    }
    draw();
    if(securityExceptionOccured) {
        securityExceptionOccured = FALSE;
        result = -2;
    }
    if(kmlFileNotFound) {
	    kmlFileNotFound = FALSE;
	    result = -3;
    }
    return result;
}
JNIEXPORT jint JNICALL Java_org_emulator_calculator_NativeLib_onFileSave(JNIEnv *env, jobject thisz) {
    // szBufferFilename must be set before calling that!!!
    BOOL result = FALSE;
    if (bDocumentAvail)
    {
        SwitchToState(SM_INVALID);
        result = SaveDocument();
        SaveMapViewToFile(pbyPort2);
        SwitchToState(SM_RUN);
    }
    return result;
}
JNIEXPORT jint JNICALL Java_org_emulator_calculator_NativeLib_onFileSaveAs(JNIEnv *env, jobject thisz, jstring newStateFilename) {
    const char *newStateFilenameUTF8 = (*env)->GetStringUTFChars(env, newStateFilename , NULL) ;

    BOOL result = FALSE;
    if (bDocumentAvail)
    {
        SwitchToState(SM_INVALID);
        _tcscpy(szBufferFilename, newStateFilenameUTF8);
        result = SaveDocumentAs(szBufferFilename);
        SaveMapViewToFile(pbyPort2);
        SwitchToState(SM_RUN);
    }

    (*env)->ReleaseStringUTFChars(env, newStateFilename, newStateFilenameUTF8);
    return result;
}

JNIEXPORT jint JNICALL Java_org_emulator_calculator_NativeLib_onFileClose(JNIEnv *env, jobject thisz) {
    if (bDocumentAvail)
    {
        SwitchToState(SM_INVALID);
        if(bAutoSave)
            SaveDocument();
        ResetDocument();
        SetWindowTitle(NULL);
        szKmlTitle[0] = '\0';
        mainViewResizeCallback(nBackgroundW, nBackgroundH);
        draw();
        return TRUE;
    }
    return FALSE;
}

JNIEXPORT jint JNICALL Java_org_emulator_calculator_NativeLib_onObjectLoad(JNIEnv *env, jobject thisz, jstring filename) {
    const char *filenameUTF8 = (*env)->GetStringUTFChars(env, filename , NULL) ;

    SuspendDebugger();						// suspend debugger
    bDbgAutoStateCtrl = FALSE;				// disable automatic debugger state control

    // calculator off, turn on
    if (!(Chipset.IORam[BITOFFSET]&DON))
    {
        KeyboardEvent(TRUE,0,0x8000);
        Sleep(dwWakeupDelay);
        KeyboardEvent(FALSE,0,0x8000);

        // wait for sleep mode
        while (Chipset.Shutdn == FALSE) Sleep(0);
    }

    if (nState != SM_RUN)
    {
        InfoMessage(_T("The emulator must be running to load an object."));
        goto cancel;
    }

    if (WaitForSleepState())				// wait for cpu SHUTDN then sleep state
    {
        InfoMessage(_T("The emulator is busy."));
        goto cancel;
    }

    _ASSERT(nState == SM_SLEEP);

//    if (bLoadObjectWarning)
//    {
//        UINT uReply = YesNoCancelMessage(
//                _T("Warning: Trying to load an object while the emulator is busy\n")
//                _T("will certainly result in a memory lost. Before loading an object\n")
//                _T("you should be sure that the calculator is in idle state.\n")
//                _T("Do you want to see this warning next time you try to load an object?"),0);
//        switch (uReply)
//        {
//            case IDYES:
//                break;
//            case IDNO:
//                bLoadObjectWarning = FALSE;
//                break;
//            case IDCANCEL:
//                SwitchToState(SM_RUN);
//                goto cancel;
//        }
//    }

//    if (!GetLoadObjectFilename(_T(HP_FILTER),_T("HP")))
//    {
//        SwitchToState(SM_RUN);
//        goto cancel;
//    }

    if (!LoadObject(filenameUTF8))
    {
        SwitchToState(SM_RUN);
        goto cancel;
    }

    SwitchToState(SM_RUN);					// run state
    while (nState!=nNextState) Sleep(0);
    _ASSERT(nState == SM_RUN);
    KeyboardEvent(TRUE,0,0x8000);
    Sleep(dwWakeupDelay);
    KeyboardEvent(FALSE,0,0x8000);
    while (Chipset.Shutdn == FALSE) Sleep(0);

    cancel:
    bDbgAutoStateCtrl = TRUE;				// enable automatic debugger state control
    ResumeDebugger();

    (*env)->ReleaseStringUTFChars(env, filename, filenameUTF8);

    return TRUE;
}

JNIEXPORT jobjectArray JNICALL Java_org_emulator_calculator_NativeLib_getObjectsToSave(JNIEnv *env, jobject thisz) {
	return 0;
}

JNIEXPORT jint JNICALL Java_org_emulator_calculator_NativeLib_onObjectSave(JNIEnv *env, jobject thisz, jstring filename, jbooleanArray objectsToSaveItemChecked) {

    const char *filenameUTF8 = (*env)->GetStringUTFChars(env, filename , NULL) ;

    if (nState != SM_RUN)
    {
        InfoMessage(_T("The emulator must be running to save an object."));
        return 0;
    }

    if (WaitForSleepState())				// wait for cpu SHUTDN then sleep state
    {
        InfoMessage(_T("The emulator is busy."));
        return 0;
    }

    _ASSERT(nState == SM_SLEEP);

//    if (GetSaveObjectFilename(_T(HP_FILTER),_T("HP")))
//    {
        SaveObject(filenameUTF8);
//    }

    SwitchToState(SM_RUN);

    (*env)->ReleaseStringUTFChars(env, filename, filenameUTF8);

    return TRUE;
}

JNIEXPORT void JNICALL Java_org_emulator_calculator_NativeLib_onViewCopy(JNIEnv *env, jobject thisz, jobject bitmapScreen) {

    AndroidBitmapInfo bitmapScreenInfo;
    int ret = AndroidBitmap_getInfo(env, bitmapScreen, &bitmapScreenInfo);
    if (ret < 0) {
        LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
        return;
    }

    void * pixelsDestination;
    if ((ret = AndroidBitmap_lockPixels(env, bitmapScreen, &pixelsDestination)) < 0) {
        LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
    }

    // DIB bitmap
    #define WIDTHBYTES(bits) (((bits) + 31) / 32 * 4)
    #define PALVERSION       0x300

    BITMAP bm;
    LPBITMAPINFOHEADER lpbi;
    PLOGPALETTE ppal;
    HBITMAP hBmp;
    HDC hBmpDC;
    HANDLE hClipObj;
    WORD wBits;
    DWORD dwLen, dwSizeImage;

    _ASSERT(nLcdZoom >= 1 && nLcdZoom <= 4);
    hBmp = CreateCompatibleBitmap(hLcdDC,131*nLcdZoom*nGdiXZoom,SCREENHEIGHT*nLcdZoom*nGdiYZoom);   // CdB for HP: add apples display stuff
    hBmpDC = CreateCompatibleDC(hLcdDC);
    hBmp = (HBITMAP) SelectObject(hBmpDC,hBmp);
    EnterCriticalSection(&csGDILock); // solving NT GDI problems
    {
        UINT nLines = MAINSCREENHEIGHT;

        // copy header display area
        StretchBlt(hBmpDC, 0, 0,
                   131*nLcdZoom*nGdiXZoom, Chipset.d0size*nLcdZoom*nGdiYZoom,
                   hLcdDC, Chipset.d0offset, 0,
                   131, Chipset.d0size, SRCCOPY);
        // copy main display area
        StretchBlt(hBmpDC, 0, Chipset.d0size*nLcdZoom*nGdiYZoom,
                   131*nLcdZoom*nGdiXZoom, nLines*nLcdZoom*nGdiYZoom,
                   hLcdDC, Chipset.boffset, Chipset.d0size,
                   131, nLines, SRCCOPY);
        // copy menu display area
        StretchBlt(hBmpDC, 0, (nLines+Chipset.d0size)*nLcdZoom*nGdiYZoom,
                   131*nLcdZoom*nGdiXZoom, MENUHEIGHT*nLcdZoom*nGdiYZoom,
                   hLcdDC, 0, (nLines+Chipset.d0size),
                   131, MENUHEIGHT, SRCCOPY);
        GdiFlush();
    }
    LeaveCriticalSection(&csGDILock);
    hBmp = (HBITMAP) SelectObject(hBmpDC,hBmp);

    // fill BITMAP structure for size information
    GetObject((HANDLE) hBmp, sizeof(bm), &bm);

    wBits = bm.bmPlanes * bm.bmBitsPixel;
    // make sure bits per pixel is valid
    if (wBits <= 1)
        wBits = 1;
    else if (wBits <= 4)
        wBits = 4;
    else if (wBits <= 8)
        wBits = 8;
    else // if greater than 8-bit, force to 24-bit
        wBits = 24;

    dwSizeImage = WIDTHBYTES((DWORD)bm.bmWidth * wBits) * bm.bmHeight;

    // calculate memory size to store CF_DIB data
    dwLen = sizeof(BITMAPINFOHEADER) + dwSizeImage;
    if (wBits != 24)				// a 24 bitcount DIB has no color table
    {
        // add size for color table
        dwLen += (DWORD) (1 << wBits) * sizeof(RGBQUAD);
    }





    size_t strideSource = (size_t)(4 * ((hBmp->bitmapInfoHeader->biWidth * hBmp->bitmapInfoHeader->biBitCount + 31) / 32));
    size_t strideDestination = bitmapScreenInfo.stride;
    VOID * bitmapBitsSource = (VOID *)hBmp->bitmapBits;
    VOID * bitmapBitsDestination = pixelsDestination + (hBmp->bitmapInfoHeader->biHeight - 1) * strideDestination;
    for(int y = 0; y < hBmp->bitmapInfoHeader->biHeight; y++) {
        memcpy(bitmapBitsDestination, bitmapBitsSource, strideSource);
        bitmapBitsSource += strideSource;
        bitmapBitsDestination -= strideDestination;
    }


    DeleteDC(hBmpDC);
    DeleteObject(hBmp);
    #undef WIDTHBYTES
    #undef PALVERSION


    AndroidBitmap_unlockPixels(env, bitmapScreen);
}

JNIEXPORT void JNICALL Java_org_emulator_calculator_NativeLib_onStackCopyVisible(JNIEnv *env, jobject thisz) {
}

JNIEXPORT void JNICALL Java_org_emulator_calculator_NativeLib_onStackCopy(JNIEnv *env, jobject thisz) {
    OnStackCopy();
}

JNIEXPORT void JNICALL Java_org_emulator_calculator_NativeLib_onStackPaste(JNIEnv *env, jobject thisz) {
    //TODO Memory leak -> No GlobalFree of the paste data!!!!
    OnStackPaste();
}

JNIEXPORT void JNICALL Java_org_emulator_calculator_NativeLib_onViewReset(JNIEnv *env, jobject thisz) {
    if (nState != SM_RUN)
        return;
    SwitchToState(SM_SLEEP);
    CpuReset();							// register setting after Cpu Reset
    SwitchToState(SM_RUN);
}

JNIEXPORT jint JNICALL Java_org_emulator_calculator_NativeLib_onViewScript(JNIEnv *env, jobject thisz, jstring kmlFilename, jstring kmlFolder) {

    TCHAR szKmlFile[MAX_PATH];
    SwitchToState(SM_INVALID);

    // make a copy of the current KML script file name
    lstrcpyn(szKmlFile,szCurrentKml,ARRAYSIZEOF(szKmlFile));

    const char * filenameUTF8 = (*env)->GetStringUTFChars(env, kmlFilename , NULL) ;
    _tcscpy(szCurrentKml, filenameUTF8);
    (*env)->ReleaseStringUTFChars(env, kmlFilename, filenameUTF8);

	if(kmlFolder) {
		const char * kmlFolderUTF8 = (*env)->GetStringUTFChars(env, kmlFolder, NULL);
		// The folder URL is separated from the script filename and comes from the JSON settings in the state file.
		_tcscpy(szEmuDirectory, kmlFolderUTF8);
		_tcscpy(szRomDirectory, kmlFolderUTF8);
		(*env)->ReleaseStringUTFChars(env, kmlFolder, kmlFolderUTF8);
	} else {
		// We are loading a KML script from the embedded asset folder inside the Android App.
		// We directly set the variable "szEmuDirectory"/"szRomDirectory" and "szCurrentAssetDirectory" with the KML folder
		// which contain the script and its dependencies like the includes, the images and the ROMs.
		_tcscpy(szEmuDirectory, "assets/calculators/");
		_tcscpy(szRomDirectory, "assets/calculators/");
	}

	chooseCurrentKmlMode = ChooseKmlMode_CHANGE_KML;

	// It is to open a new document, so we will trick the next KillKML() to prevent to UnmapROM()!!!
	// pbyRom is then restored in the unused win32 GetKeyboardLayoutName(), just after KillKML() but before the final InitKML(). Crazy!
	if (pbyRom) {
		pbyRomBackup = pbyRom;
		pbyRom = NULL;
	}
	BOOL bSucc = InitKML(szCurrentKml, FALSE);
	if(pbyRomBackup) pbyRomBackup = NULL;

    if(!bSucc) {
        // restore KML script file name
        lstrcpyn(szCurrentKml,szKmlFile,ARRAYSIZEOF(szCurrentKml));

        _tcsncpy(szKmlLogBackup, szKmlLog, sizeof(szKmlLog) / sizeof(TCHAR));

        // try to restore old KML script
        bSucc = InitKML(szCurrentKml, FALSE);

        _tcsncpy(szKmlLog, szKmlLogBackup, sizeof(szKmlLog) / sizeof(TCHAR));
    }
    chooseCurrentKmlMode = ChooseKmlMode_UNKNOWN;

    if(bSucc) {
        mainViewResizeCallback(nBackgroundW, nBackgroundH);
        draw();
        if (Chipset.wRomCrc != wRomCrc)		// ROM changed
        {
            CpuReset();
            Chipset.Shutdn = FALSE;			// automatic restart

            Chipset.wRomCrc = wRomCrc;		// update current ROM fingerprint
        }
        if (pbyRom) SwitchToState(SM_RUN);	// continue emulation
    } else {
        ResetDocument();					// close document
        SetWindowTitle(NULL);
    }

    return bSucc;
}

JNIEXPORT void JNICALL Java_org_emulator_calculator_NativeLib_onBackupSave(JNIEnv *env, jobject thisz) {
    UINT nOldState;
    if (pbyRom == NULL) return;
    nOldState = SwitchToState(SM_INVALID);
    SaveBackup();
    SwitchToState(nOldState);
}

JNIEXPORT void JNICALL Java_org_emulator_calculator_NativeLib_onBackupRestore(JNIEnv *env, jobject thisz) {
    SwitchToState(SM_INVALID);
    RestoreBackup();
    if (pbyRom) SwitchToState(SM_RUN);
}

JNIEXPORT void JNICALL Java_org_emulator_calculator_NativeLib_onBackupDelete(JNIEnv *env, jobject thisz) {
    ResetBackup();
}

JNIEXPORT void JNICALL Java_org_emulator_calculator_NativeLib_onToolMacroNew(JNIEnv *env, jobject thisz, jstring filename) {
    const char *filenameUTF8 = (*env)->GetStringUTFChars(env, filename , NULL);
    _tcscpy(getSaveObjectFilenameResult, filenameUTF8);
    (*env)->ReleaseStringUTFChars(env, filename, filenameUTF8);
    currentDialogBoxMode = DialogBoxMode_SAVE_MACRO;
    OnToolMacroNew();
    getSaveObjectFilenameResult[0] = 0;
    currentDialogBoxMode = DialogBoxMode_UNKNOWN;
}

JNIEXPORT void JNICALL Java_org_emulator_calculator_NativeLib_onToolMacroPlay(JNIEnv *env, jobject thisz, jstring filename) {
    const char *filenameUTF8 = (*env)->GetStringUTFChars(env, filename , NULL);
    _tcscpy(getSaveObjectFilenameResult, filenameUTF8);
    (*env)->ReleaseStringUTFChars(env, filename, filenameUTF8);
    currentDialogBoxMode = DialogBoxMode_OPEN_MACRO;
    OnToolMacroPlay();
    getSaveObjectFilenameResult[0] = 0;
    currentDialogBoxMode = DialogBoxMode_UNKNOWN;
}

JNIEXPORT void JNICALL Java_org_emulator_calculator_NativeLib_onToolMacroStop(JNIEnv *env, jobject thisz) {
    OnToolMacroStop();
}


JNIEXPORT jboolean JNICALL Java_org_emulator_calculator_NativeLib_onLoadFlashROM(JNIEnv *env, jobject thisz, jstring filename) {
	const char *filenameUTF8 = (*env)->GetStringUTFChars(env, filename , NULL);
	if (bDocumentAvail)
		SwitchToState(SM_INVALID);
	UnmapRom();
	BYTE cCurrentRomTypeBackup = cCurrentRomType;
	cCurrentRomType = 'Q'; // Fake the model to allow to load the ROM in RW!
	BOOL result = MapRom(filenameUTF8);
	cCurrentRomType = cCurrentRomTypeBackup;
	if(result) {
		UpdatePatches(TRUE);                    // Apply the patch again if needed (not tested!)
		if (!CrcRom(&wRomCrc))				    // build patched ROM fingerprint and check for unpacked data
			result = FALSE;
		if (result && bDocumentAvail) {
			if (Chipset.wRomCrc != wRomCrc)     // ROM changed
			{
				CpuReset();
				Chipset.Shutdn = FALSE;         // automatic restart

				Chipset.wRomCrc = wRomCrc;      // update current ROM fingerprint
			}
			if (pbyRom)
				SwitchToState(SM_RUN);          // continue emulation
		}
	}
	// If failed to load,
	// either it is when opening a new document, so it will fall back to the default Flash ROM of the KML script,
	// or it is when loading a Flash ROM, so we have to manually load the default Flash ROM by resetting (loading the same KML script (OnViewScript))
	// or it is when saving the Flash ROM, so we have to manually load the default Flash ROM by resetting (loading the same KML script (OnViewScript))
	(*env)->ReleaseStringUTFChars(env, filename, filenameUTF8);
	return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL Java_org_emulator_calculator_NativeLib_setConfiguration(JNIEnv *env, jobject thisz, jstring key, jint isDynamic, jint intValue1, jint intValue2, jstring stringValue) {
    const char *configKey = (*env)->GetStringUTFChars(env, key, NULL) ;
    const char *configStringValue = stringValue ? (*env)->GetStringUTFChars(env, stringValue, NULL) : NULL;

	//LOGE("NativeLib_setConfiguration(%s, %d, %d, %s)", configKey, intValue1, intValue2, configStringValue);

	bAutoSave = FALSE;
    bAutoSaveOnExit = FALSE;
    bLoadObjectWarning = FALSE;
    bAlwaysDisplayLog = TRUE;

    if(_tcscmp(_T("settings_realspeed"), configKey) == 0) {
        bRealSpeed = (BOOL) intValue1;
        SetSpeed(bRealSpeed);			// set speed
    } else if(_tcscmp(_T("settings_grayscale"), configKey) == 0) {
        // LCD grayscale checkOnBackupDeletebox has been changed
        if (bGrayscale != (BOOL)intValue1) {
            UINT nOldState = SwitchToState(SM_INVALID);
            SetLcdMode(!bGrayscale);	// set new display mode
            SwitchToState(nOldState);
        }
    } else if(_tcscmp(_T("settings_sound_volume"), configKey) == 0) {
        dwWaveVol = (DWORD)intValue1;
        if(soundEnabled && intValue1 == 0) {
            SoundClose();
            soundEnabled = FALSE;
        } else if(!soundEnabled && intValue1 > 0) {
            SoundOpen(uWaveDevId);
            soundEnabled = TRUE;
        }
    } else if(_tcscmp(_T("settings_macro"), configKey) == 0) {
        bMacroRealSpeed = (BOOL)intValue1;
        nMacroTimeout = 500 - intValue2;
    } else if(_tcscmp(_T("settings_port1"), configKey) == 0) {
        BOOL settingsPort1en = (BOOL) intValue1;
        BOOL settingsPort1wr = (BOOL) intValue2;
        // port1
        if (Chipset.Port1Size && (cCurrentRomType!='X' || cCurrentRomType!='2' || cCurrentRomType!='Q'))   // CdB for HP: add apples
        {
            UINT nOldState = SwitchToState(SM_SLEEP);
	        //LOGE("NativeLib_setConfiguration port1 start SwitchToState %d -> SM_SLEEP", nOldState);

	        // save old card status
            BYTE byCardsStatus = Chipset.cards_status;

            // port1 disabled?
            Chipset.cards_status &= ~(PORT1_PRESENT | PORT1_WRITE);
            if (settingsPort1en)
            {
                Chipset.cards_status |= PORT1_PRESENT;
                if (settingsPort1wr)
                    Chipset.cards_status |= PORT1_WRITE;
            }

            // changed card status in slot1?
            if (   ((byCardsStatus ^ Chipset.cards_status) & (PORT1_PRESENT | PORT1_WRITE)) != 0
                   && (Chipset.IORam[CARDCTL] & ECDT) != 0 && (Chipset.IORam[TIMER2_CTRL] & RUN) != 0
                    )
            {
                Chipset.HST |= MP;		// set Module Pulled
                IOBit(SRQ2,NINT,FALSE);	// set NINT to low
                Chipset.SoftInt = TRUE;	// set interrupt
                bInterrupt = TRUE;
            }
	        //LOGE("NativeLib_setConfiguration port1 end SwitchToState %d", nOldState);
            SwitchToState(nOldState);
	        while (nState!=nNextState) Sleep(0);
        }
    } else if(_tcscmp(_T("settings_port2"), configKey) == 0) {
        settingsPort2en = (BOOL)intValue1;
        settingsPort2wr = (BOOL)intValue2;
        const char * settingsPort2load = settingsPort2en ? configStringValue : NULL;

        LPCTSTR szActPort2Filename = _T("");
        BOOL bPort2CfgChange = FALSE;
        BOOL bPort2AttChange = FALSE;

        // HP48SX/GX port2 change settings detection
        if (cCurrentRomType=='S' || cCurrentRomType=='G' || cCurrentRomType==0) {
            if(settingsPort2en && settingsPort2load) {
                if(_tcscmp(szPort2Filename, settingsPort2load) != 0) {
                    _tcscpy(szPort2Filename, settingsPort2load);
                    bPort2CfgChange = TRUE;    // slot2 configuration changed
                }
                szActPort2Filename = szPort2Filename;

                // R/W port
                if (*szActPort2Filename != 0 && (BOOL) settingsPort2wr != bPort2Writeable) {
                    bPort2AttChange = TRUE;    // slot2 file R/W attribute changed
                    bPort2CfgChange = TRUE;    // slot2 configuration changed
                }
            } else {
                if(szPort2Filename[0] != '\0') {
                    bPort2CfgChange = TRUE;    // slot2 configuration changed
                    szPort2Filename[0] = '\0';
                }
            }
        }

        if (bPort2CfgChange)			// slot2 configuration changed
        {
            UINT nOldState = SwitchToState(SM_INVALID);
	        //LOGE("NativeLib_setConfiguration port2 start SwitchToState %d -> SM_INVALID", nOldState);

            UnmapPort2();				// unmap port2

            if (cCurrentRomType)		// ROM defined
            {
                MapPort2(szActPort2Filename);

                // port2 changed and card detection enabled
                if (   (bPort2AttChange || Chipset.wPort2Crc != wPort2Crc)
                       && (Chipset.IORam[CARDCTL] & ECDT) != 0 && (Chipset.IORam[TIMER2_CTRL] & RUN) != 0
                        )
                {
                    Chipset.HST |= MP;		// set Module Pulled
                    IOBit(SRQ2,NINT,FALSE);	// set NINT to low
                    Chipset.SoftInt = TRUE;	// set interrupt
                    bInterrupt = TRUE;
                }
                // save fingerprint of port2
                Chipset.wPort2Crc = wPort2Crc;
            }
	        //LOGE("NativeLib_setConfiguration port2 end SwitchToState %d", nOldState);
            SwitchToState(nOldState);
        }
    } else if(_tcscmp(_T("settings_serial_ports_wire"), configKey) == 0) {
	    const char * newSerialWire = _tcscmp(_T("0000:0000,0"), configStringValue) == 0 ? NO_SERIAL : configStringValue;
	    BOOL serialWireChanged = _tcscmp(szSerialWire, newSerialWire) != 0;
	    _tcsncpy(szSerialWire, newSerialWire, sizeof(szSerialWire));
	    if(CommIsOpen() && serialWireChanged) {
	    	// Not the right thread, but it seems to work.
		    CommOpen(szSerialWire, szSerialIr);
	    }
    } else if(_tcscmp(_T("settings_serial_ports_ir"), configKey) == 0) {
	    const char * newSerialIr = _tcscmp(_T("0000:0000,0"), configStringValue) == 0 ? NO_SERIAL : configStringValue;
	    BOOL serialIrChanged = _tcscmp(szSerialIr, newSerialIr) != 0;
	    _tcsncpy(szSerialIr, newSerialIr, sizeof(szSerialIr));
	    if(CommIsOpen() && serialIrChanged) {
		    // Not the right thread, but it seems to work.
		    CommOpen(szSerialWire, szSerialIr);
	    }
    } else if(_tcscmp(_T("settings_serial_slowdown"), configKey) == 0) {
	    serialPortSlowDown = (BOOL)intValue1;
    }

    if(configKey)
        (*env)->ReleaseStringUTFChars(env, key, configKey);
    if(configStringValue)
        (*env)->ReleaseStringUTFChars(env, stringValue, configStringValue);
}

JNIEXPORT jboolean JNICALL Java_org_emulator_calculator_NativeLib_isPortExtensionPossible(JNIEnv *env, jobject thisz) {
    return (uint8_t)(cCurrentRomType=='S' || cCurrentRomType=='G' || cCurrentRomType==0 ? JNI_TRUE : JNI_FALSE);
}

JNIEXPORT jint JNICALL Java_org_emulator_calculator_NativeLib_getState(JNIEnv *env, jobject thisz) {
    return nState;
}

JNIEXPORT jint JNICALL Java_org_emulator_calculator_NativeLib_getScreenPositionX(JNIEnv *env, jobject thisz) {
	return nLcdX - nBackgroundX;
}
JNIEXPORT jint JNICALL Java_org_emulator_calculator_NativeLib_getScreenPositionY(JNIEnv *env, jobject thisz) {
	return nLcdY - nBackgroundY;
}
JNIEXPORT jint JNICALL Java_org_emulator_calculator_NativeLib_getScreenWidth(JNIEnv *env, jobject thisz) {
    return 131*nLcdZoom*nGdiXZoom;
}
JNIEXPORT jint JNICALL Java_org_emulator_calculator_NativeLib_getScreenHeight(JNIEnv *env, jobject thisz) {
    return SCREENHEIGHT*nLcdZoom*nGdiYZoom;
}
JNIEXPORT jint JNICALL Java_org_emulator_calculator_NativeLib_getScreenWidthNative(JNIEnv *env, jobject thisz) {
    return 131;
}
JNIEXPORT jint JNICALL Java_org_emulator_calculator_NativeLib_getScreenHeightNative(JNIEnv *env, jobject thisz) {
    return SCREENHEIGHT;
}
JNIEXPORT jint JNICALL Java_org_emulator_calculator_NativeLib_getLCDBackgroundColor(JNIEnv *env, jobject thisz) {
	if (hLcdDC && hLcdDC->realizedPalette && hLcdDC->realizedPalette->paletteLog &&
	    hLcdDC->realizedPalette->paletteLog->palPalEntry) {
		PALETTEENTRY *palPalEntry = hLcdDC->realizedPalette->paletteLog->palPalEntry;
		return palPalEntry[0].peRed << 16 | palPalEntry[0].peGreen << 8 | palPalEntry[0].peBlue;
	}
	return -1;
}

JNIEXPORT void JNICALL Java_org_emulator_calculator_NativeLib_commEvent(JNIEnv *env, jclass clazz, jint commId, jint eventMask) {
	commEvent(commId, eventMask);
}