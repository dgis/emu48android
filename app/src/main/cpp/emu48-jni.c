//
// Created by cosnier on 12/11/2018.
//
#include <jni.h>
#include <stdio.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/bitmap.h>

#include "core/pch.h"
#include "core/Emu48.h"
#include "core/io.h"

extern void emu48Start();
extern AAssetManager * assetManager;
static jobject viewToUpdate = NULL;
static jobject mainActivity = NULL;
jobject bitmapMainScreen;
AndroidBitmapInfo androidBitmapInfo;
TCHAR   szChosenCurrentKml[MAX_PATH];

extern void win32Init();

extern void draw();
extern void buttonDown(int x, int y);
extern void buttonUp(int x, int y);
extern void keyDown(int virtKey);
extern void keyUp(int virtKey);

extern void OnObjectLoad();
extern void OnObjectSave();
extern void OnViewCopy();
extern void OnViewReset();
extern void OnBackupSave();
extern void OnBackupRestore();
extern void OnBackupDelete();


JNIEnv *getJNIEnvironment();

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
    ret = (*java_machine)->GetEnv(java_machine, &jniEnv, JNI_VERSION_1_6);
    if (ret == JNI_EDETACHED) {
        // GetEnv: not attached
        ret = (*java_machine)->AttachCurrentThread(java_machine, &jniEnv, NULL);
        if (ret == JNI_OK) {
            needDetach = TRUE;
        }
    }
    return jniEnv;
}

enum CALLBACK_TYPE {
    CALLBACK_TYPE_INVALIDATE = 0,
    CALLBACK_TYPE_WINDOW_RESIZE = 1
};

void mainViewUpdateCallback() {
    mainViewCallback(CALLBACK_TYPE_INVALIDATE, 0, 0, NULL, NULL);
}

void mainViewResizeCallback(int x, int y) {
    mainViewCallback(CALLBACK_TYPE_WINDOW_RESIZE, x, y, NULL, NULL);

    JNIEnv * jniEnv;
    int ret = (*java_machine)->GetEnv(java_machine, &jniEnv, JNI_VERSION_1_6);
    ret = AndroidBitmap_getInfo(jniEnv, bitmapMainScreen, &androidBitmapInfo);
    if (ret < 0) {
        LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
    }
}

// https://stackoverflow.com/questions/9630134/jni-how-to-callback-from-c-or-c-to-java
int mainViewCallback(int type, int param1, int param2, const TCHAR * param3, const TCHAR * param4) {
    if (viewToUpdate) {
        JNIEnv *jniEnv = getJNIEnvironment();
        jclass viewToUpdateClass = (*jniEnv)->GetObjectClass(jniEnv, viewToUpdate);
        jmethodID midStr = (*jniEnv)->GetMethodID(jniEnv, viewToUpdateClass, "updateCallback", "(IIILjava/lang/String;Ljava/lang/String;)I");
        jstring utfParam3 = (*jniEnv)->NewStringUTF(jniEnv, param3);
        jstring utfParam4 = (*jniEnv)->NewStringUTF(jniEnv, param4);
        int result = (*jniEnv)->CallIntMethod(jniEnv, viewToUpdate, midStr, type, param1, param2, utfParam3, utfParam4);
//      if(needDetach) ret = (*java_machine)->DetachCurrentThread(java_machine);
        return result;
    }
}

// Must be called in the main thread
int openFileFromContentResolver(const TCHAR * url, int writeAccess) {
    JNIEnv *jniEnv = getJNIEnvironment();
    jclass mainActivityClass = (*jniEnv)->GetObjectClass(jniEnv, mainActivity);
    jmethodID midStr = (*jniEnv)->GetMethodID(jniEnv, mainActivityClass, "openFileFromContentResolver", "(Ljava/lang/String;I)I");
    jstring utfUrl = (*jniEnv)->NewStringUTF(jniEnv, url);
    int result = (*jniEnv)->CallIntMethod(jniEnv, mainActivity, midStr, utfUrl, writeAccess);
    return result;
}

JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_NativeLib_setConfiguration(JNIEnv *env, jobject thisz,
        jint settingsRealspeed, jint settingsGrayscale, jint settingsAutosave,
        jint settingsAutosaveonexit, jint settingsObjectloadwarning, jint settingsAlwaysdisplog,
        jint settingsPort1en, jint settingsPort1wr,
        jint settingsPort2len, jint settingsPort2wr, jstring settingsPort2load) {

    bRealSpeed = settingsRealspeed;
    bAutoSave = settingsAutosave;
    bAutoSaveOnExit = settingsAutosaveonexit;
    bLoadObjectWarning = settingsObjectloadwarning;
    bAlwaysDisplayLog = settingsAlwaysdisplog;

    SetSpeed(bRealSpeed);			// set speed

    // LCD grayscale checkbox has been changed
    if (bGrayscale != (BOOL)settingsGrayscale) {
        UINT nOldState = SwitchToState(SM_INVALID);
        SetLcdMode(!bGrayscale);	// set new display mode
        SwitchToState(nOldState);
    }

    //SettingsMemoryProc
    LPCTSTR szActPort2Filename = _T("");

    BOOL bPort2CfgChange = FALSE;
    BOOL bPort2AttChange = FALSE;

    // port1
    if (Chipset.Port1Size && (cCurrentRomType!='X' || cCurrentRomType!='2' || cCurrentRomType!='Q'))   // CdB for HP: add apples
    {
        UINT nOldState = SwitchToState(SM_SLEEP);
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
        SwitchToState(nOldState);
    }
    // HP48SX/GX port2 change settings detection
    if (cCurrentRomType=='S' || cCurrentRomType=='G' || cCurrentRomType==0)
    {
        //bPort2IsShared = settingsPort2isshared;
        const char * szNewPort2Filename = NULL;
        const char *settingsPort2loadUTF8 = NULL;
        if(settingsPort2load) {
            settingsPort2loadUTF8 = (*env)->GetStringUTFChars(env, settingsPort2load , NULL);
            szNewPort2Filename = settingsPort2loadUTF8;
        } else
            szNewPort2Filename = _T("SHARED.BIN");

        if(_tcscmp(szPort2Filename, szNewPort2Filename) != 0) {
            _tcscpy(szPort2Filename, szNewPort2Filename);
            szActPort2Filename = szPort2Filename;
            bPort2CfgChange = TRUE;	// slot2 configuration changed

            // R/W port
            if (   *szActPort2Filename != 0
                   && (BOOL) settingsAlwaysdisplog != bPort2Writeable)
            {
                bPort2AttChange = TRUE;	// slot2 file R/W attribute changed
                bPort2CfgChange = TRUE;	// slot2 configuration changed
            }
        }
        if(settingsPort2loadUTF8)
            (*env)->ReleaseStringUTFChars(env, settingsPort2load, settingsPort2loadUTF8);
    }

    if (bPort2CfgChange)			// slot2 configuration changed
    {
        UINT nOldState = SwitchToState(SM_INVALID);

        UnmapPort2();				// unmap port2

//        if (bPort2AttChange)		// slot2 R/W mode changed
//        {
//            DWORD dwFileAtt;
//
//            SetCurrentDirectory(szEmuDirectory);
//            dwFileAtt = GetFileAttributes(szActPort2Filename);
//            if (dwFileAtt != 0xFFFFFFFF)
//            {
//                if (IsDlgButtonChecked(hDlg,IDC_PORT2WR))
//                    dwFileAtt &= ~FILE_ATTRIBUTE_READONLY;
//                else
//                    dwFileAtt |= FILE_ATTRIBUTE_READONLY;
//
//                SetFileAttributes(szActPort2Filename,dwFileAtt);
//            }
//            SetCurrentDirectory(szCurrentDirectory);
//        }

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
        SwitchToState(nOldState);
    }
}


JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_NativeLib_start(JNIEnv *env, jobject thisz, jobject assetMgr, jobject bitmapMainScreen0, jobject activity, jobject view) {

    szChosenCurrentKml[0] = '\0';

    bitmapMainScreen = (*env)->NewGlobalRef(env, bitmapMainScreen0);
    mainActivity = (*env)->NewGlobalRef(env, activity);
    viewToUpdate = (*env)->NewGlobalRef(env, view);


    int ret = AndroidBitmap_getInfo(env, bitmapMainScreen, &androidBitmapInfo);
    if (ret < 0) {
        LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
    }

    assetManager = AAssetManager_fromJava(env, assetMgr);





    DWORD dwThreadId;

    // read emulator settings
    GetCurrentDirectory(ARRAYSIZEOF(szCurrentDirectory),szCurrentDirectory);
    ReadSettings();

    _tcscpy(szCurrentDirectory, "");
    _tcscpy(szEmuDirectory, "assets/calculators/");
    _tcscpy(szRomDirectory, "assets/calculators/");
    _tcscpy(szPort2Filename, "");

    hWindowDC = CreateCompatibleDC(NULL);

    // initialization
    LARGE_INTEGER    lAppStart2;					// high performance counter value at Appl. start

    QueryPerformanceFrequency(&lFreq);		// init high resolution counter
    QueryPerformanceCounter(&lAppStart);
    Sleep(1000);
    QueryPerformanceCounter(&lAppStart2);

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

    ResumeThread(hThread);					// start thread
    while (nState!=nNextState) Sleep(0);	// wait for thread initialized
}

JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_NativeLib_stop(JNIEnv *env, jobject thisz) {

    if (viewToUpdate) {
        (*env)->DeleteGlobalRef(env, viewToUpdate);
        viewToUpdate = NULL;
    }
    if(bitmapMainScreen) {
        (*env)->DeleteGlobalRef(env, bitmapMainScreen);
        bitmapMainScreen = NULL;
    }
}


//JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_NativeLib_resize(JNIEnv *env, jobject thisz, jint width, jint height) {
//
//}

JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_NativeLib_draw(JNIEnv *env, jobject thisz) {
    draw();
}
JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_NativeLib_buttonDown(JNIEnv *env, jobject thisz, jint x, jint y) {
    buttonDown(x, y);
}
JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_NativeLib_buttonUp(JNIEnv *env, jobject thisz, jint x, jint y) {
    buttonUp(x, y);
}
JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_NativeLib_keyDown(JNIEnv *env, jobject thisz, jint virtKey) {
    keyDown(virtKey);
}
JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_NativeLib_keyUp(JNIEnv *env, jobject thisz, jint virtKey) {
    keyUp(virtKey);
}



//JNIEXPORT jstring JNICALL Java_com_regis_cosnier_emu48_NativeLib_getCurrentFilename(JNIEnv *env, jobject thisz) {
//    jstring result = (*env)->NewStringUTF(env, szBufferFilename);
//    return result;
//}
//JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_NativeLib_setCurrentFilename(JNIEnv *env, jobject thisz, jstring newFilename) {
//    const char *newFilenameUTF8 = (*env)->GetStringUTFChars(env, newFilename , NULL) ;
//    _tcscpy(szBufferFilename, newFilenameUTF8);
//    (*env)->ReleaseStringUTFChars(env, newFilename, newFilenameUTF8);
//}

JNIEXPORT jint JNICALL Java_com_regis_cosnier_emu48_NativeLib_getCurrentModel(JNIEnv *env, jobject thisz) {
    return cCurrentRomType;
}

JNIEXPORT jint JNICALL Java_com_regis_cosnier_emu48_NativeLib_onFileNew(JNIEnv *env, jobject thisz, jstring kmlFilename) {
    //OnFileNew();
    if (bDocumentAvail)
    {
        SwitchToState(SM_INVALID);
        if(bAutoSave) {
            SaveDocument();
        }
    }

    const char *filenameUTF8 = (*env)->GetStringUTFChars(env, kmlFilename , NULL) ;
    _tcscpy(szChosenCurrentKml, filenameUTF8);
    (*env)->ReleaseStringUTFChars(env, kmlFilename, filenameUTF8);

    BOOL result = NewDocument();

    mainViewResizeCallback(nBackgroundW, nBackgroundH);
    draw();
    if (bStartupBackup) SaveBackup();        // make a RAM backup at startup

    if (pbyRom) SwitchToState(SM_RUN);

    return result;
}
JNIEXPORT jint JNICALL Java_com_regis_cosnier_emu48_NativeLib_onFileOpen(JNIEnv *env, jobject thisz, jstring stateFilename) {
    //OnFileOpen();
    if (bDocumentAvail)
    {
        SwitchToState(SM_INVALID);
        if(bAutoSave) {
            SaveDocument();
        }
    }
    const char *stateFilenameUTF8 = (*env)->GetStringUTFChars(env, stateFilename , NULL) ;
    _tcscpy(szBufferFilename, stateFilenameUTF8);
    BOOL result = OpenDocument(szBufferFilename);
    if (result)
        MruAdd(szBufferFilename);
    mainViewResizeCallback(nBackgroundW, nBackgroundH);
    if (pbyRom) SwitchToState(SM_RUN);
    draw();
    (*env)->ReleaseStringUTFChars(env, stateFilename, stateFilenameUTF8);
    return result;
}
JNIEXPORT jint JNICALL Java_com_regis_cosnier_emu48_NativeLib_onFileSave(JNIEnv *env, jobject thisz) {
    // szBufferFilename must be set before calling that!!!
    //OnFileSave();
    BOOL result = FALSE;
    if (bDocumentAvail)
    {
        SwitchToState(SM_INVALID);
        result = SaveDocument();
        SwitchToState(SM_RUN);
    }
    return result;
}
JNIEXPORT jint JNICALL Java_com_regis_cosnier_emu48_NativeLib_onFileSaveAs(JNIEnv *env, jobject thisz, jstring newStateFilename) {
    const char *newStateFilenameUTF8 = (*env)->GetStringUTFChars(env, newStateFilename , NULL) ;

    BOOL result = FALSE;
    if (bDocumentAvail)
    {
        SwitchToState(SM_INVALID);
        _tcscpy(szBufferFilename, newStateFilenameUTF8);
        result = SaveDocumentAs(szBufferFilename);
        if (result)
            MruAdd(szCurrentFilename);
        else {
            // ERROR !!!!!!!!!
        }
        SwitchToState(SM_RUN);
    }

    (*env)->ReleaseStringUTFChars(env, newStateFilename, newStateFilenameUTF8);
    return result;
}

JNIEXPORT jint JNICALL Java_com_regis_cosnier_emu48_NativeLib_onFileClose(JNIEnv *env, jobject thisz) {
    //OnFileClose();
    if (bDocumentAvail)
    {
        SwitchToState(SM_INVALID);
        if(bAutoSave)
            SaveDocument();
        ResetDocument();
        SetWindowTitle(NULL);
        mainViewResizeCallback(nBackgroundW, nBackgroundH);
        draw();
        return TRUE;
    }
    return FALSE;
}

JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_NativeLib_onObjectLoad(JNIEnv *env, jobject thisz) {
    OnObjectLoad();
}

JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_NativeLib_onObjectSave(JNIEnv *env, jobject thisz) {
    OnObjectSave();
}

JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_NativeLib_onViewCopy(JNIEnv *env, jobject thisz) {
    OnViewCopy();
}

JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_NativeLib_onStackCopy(JNIEnv *env, jobject thisz) {
    OnStackCopy();
}

JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_NativeLib_onStackPaste(JNIEnv *env, jobject thisz) {
    OnStackPaste();
}

JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_NativeLib_onViewReset(JNIEnv *env, jobject thisz) {
    OnViewReset();
}

JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_NativeLib_onBackupSave(JNIEnv *env, jobject thisz) {
    OnBackupSave();
}

JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_NativeLib_onBackupRestore(JNIEnv *env, jobject thisz) {
    OnBackupRestore();
}

JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_NativeLib_onBackupDelete(JNIEnv *env, jobject thisz) {
    OnBackupDelete();
}

//p Read5(0x7050E)
//   -> $1 = 461076
//p Read5(0x70914)
//   -> 31 ?