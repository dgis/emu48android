//
// Created by cosnier on 12/11/2018.
//
#include <jni.h>
#include <stdio.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/bitmap.h>

#include "pch.h"

JavaVM *java_machine;
JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    java_machine = vm;
    return JNI_VERSION_1_6;
}

extern void emu48Start();
extern AAssetManager * assetManager;
static jobject viewToUpdate = NULL;
jobject bitmapMainScreen;

// https://stackoverflow.com/questions/9630134/jni-how-to-callback-from-c-or-c-to-java
void mainViewUpdateCallback() {
    if (viewToUpdate) {
        JNIEnv *jni;
        jint result = (*java_machine)->AttachCurrentThread(java_machine, &jni, NULL);
        jclass viewToUpdateClass = (*jni)->GetObjectClass(jni, viewToUpdate);
        jmethodID midStr = (*jni)->GetMethodID(jni, viewToUpdateClass, "updateCallback", "()V");
        (*jni)->CallVoidMethod(jni, viewToUpdate, midStr);
        result = (*java_machine)->DetachCurrentThread(java_machine);
    }
}


JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_NativeLib_start(JNIEnv *env, jobject thisz, jobject assetMgr, jobject bitmapMainScreen0, jobject view) {

    viewToUpdate = (*env)->NewGlobalRef(env, view);
    bitmapMainScreen = (*env)->NewGlobalRef(env, bitmapMainScreen0);

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