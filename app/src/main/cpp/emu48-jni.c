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

//extern void OnFileNew();
//extern void OnFileOpen();
//extern void OnFileSave();
//extern void OnFileSaveAs();
//extern void OnFileClose();
extern void OnObjectLoad();
extern void OnObjectSave();
extern void OnViewCopy();
//extern void OnStackCopy();
//extern void OnStackPaste();
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
    emu48Start();
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

JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_NativeLib_onFileNew(JNIEnv *env, jobject thisz, jstring kmlFilename) {
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

    if (NewDocument()) SetWindowTitle(_T("Untitled"));

    mainViewResizeCallback(nBackgroundW, nBackgroundH);
    draw();
    if (bStartupBackup) SaveBackup();		// make a RAM backup at startup

    if (pbyRom) SwitchToState(SM_RUN);
}
JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_NativeLib_onFileOpen(JNIEnv *env, jobject thisz, jstring stateFilename) {
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
    if (OpenDocument(szBufferFilename))
        MruAdd(szBufferFilename);
    mainViewResizeCallback(nBackgroundW, nBackgroundH);
    if (pbyRom) SwitchToState(SM_RUN);
    draw();
    (*env)->ReleaseStringUTFChars(env, stateFilename, stateFilenameUTF8);
}
JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_NativeLib_onFileSave(JNIEnv *env, jobject thisz) {
    // szBufferFilename must be set before calling that!!!
    //OnFileSave();
    if (bDocumentAvail)
    {
        SwitchToState(SM_INVALID);
        SaveDocument();
        SwitchToState(SM_RUN);
    }

}
JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_NativeLib_onFileSaveAs(JNIEnv *env, jobject thisz, jstring newStateFilename) {
    const char *newStateFilenameUTF8 = (*env)->GetStringUTFChars(env, newStateFilename , NULL) ;

    if (bDocumentAvail)
    {
        SwitchToState(SM_INVALID);
        _tcscpy(szBufferFilename, newStateFilenameUTF8);
        if (SaveDocumentAs(szBufferFilename))
            MruAdd(szCurrentFilename);
        else {
            // ERROR !!!!!!!!!
        }
        SwitchToState(SM_RUN);
    }

    (*env)->ReleaseStringUTFChars(env, newStateFilename, newStateFilenameUTF8);
}

JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_NativeLib_onFileClose(JNIEnv *env, jobject thisz) {
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
    }
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