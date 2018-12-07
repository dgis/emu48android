//
// Created by cosnier on 12/11/2018.
//
#include <jni.h>
#include <stdio.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/bitmap.h>

#include "pch.h"

extern void emu48Start();
extern AAssetManager * assetManager;
static jobject viewToUpdate = NULL;
jobject bitmapMainScreen;
AndroidBitmapInfo androidBitmapInfo;

extern void win32Init();

extern void draw();
extern void buttonDown(int x, int y);
extern void buttonUp(int x, int y);
extern void keyDown(int virtKey);
extern void keyUp(int virtKey);


JavaVM *java_machine;
JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    java_machine = vm;
    win32Init();
    return JNI_VERSION_1_6;
}

enum CALLBACK_TYPE {
    CALLBACK_TYPE_INVALIDATE = 0,
    CALLBACK_TYPE_WINDOW_RESIZE = 1
};

void mainViewUpdateCallback() {
    mainViewCallback(CALLBACK_TYPE_INVALIDATE, 0, 0);
}

void mainViewResizeCallback(int x, int y) {
    mainViewCallback(CALLBACK_TYPE_WINDOW_RESIZE, x, y);

    JNIEnv * jniEnv;
    int ret = (*java_machine)->GetEnv(java_machine, &jniEnv, JNI_VERSION_1_6);
    ret = AndroidBitmap_getInfo(jniEnv, bitmapMainScreen, &androidBitmapInfo);
    if (ret < 0) {
        LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
    }
}

// https://stackoverflow.com/questions/9630134/jni-how-to-callback-from-c-or-c-to-java
int mainViewCallback(int type, int param1, int param2) {
    if (viewToUpdate) {
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

        jclass viewToUpdateClass = (*jniEnv)->GetObjectClass(jniEnv, viewToUpdate);
        //jmethodID midStr = (*jniEnv)->GetMethodID(jniEnv, viewToUpdateClass, "updateCallback", "()V");
        jmethodID midStr = (*jniEnv)->GetMethodID(jniEnv, viewToUpdateClass, "updateCallback", "(III)I");
        int result = (*jniEnv)->CallIntMethod(jniEnv, viewToUpdate, midStr, type, param1, param2);
//        if(needDetach)
//            ret = (*java_machine)->DetachCurrentThread(java_machine);
        return result;
    }
}


JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_NativeLib_start(JNIEnv *env, jobject thisz, jobject assetMgr, jobject bitmapMainScreen0, jobject view) {

    viewToUpdate = (*env)->NewGlobalRef(env, view);
    bitmapMainScreen = (*env)->NewGlobalRef(env, bitmapMainScreen0);

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


JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_NativeLib_resize(JNIEnv *env, jobject thisz, jint width, jint height) {

}

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

//p Read5(0x7050E)
//   -> $1 = 461076
//p Read5(0x70914)
//   -> 31 ?