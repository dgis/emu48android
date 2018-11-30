//
// Created by cosnier on 12/11/2018.
//
#include <jni.h>
#include <stdio.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/bitmap.h>

#include "pch.h"

JNIEXPORT jstring JNICALL
Java_com_regis_cosnier_emu48_MainActivity_stringFromJNI(
                JNIEnv *env,
                jobject thisz) {
//    std::string hello = "Hello from C++";
//    return env->NewStringUTF(hello.c_str());
    return (*env)->NewStringUTF(env, "Hello from JNI !");
}

extern void emu48Start();
extern AAssetManager * assetManager;

JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_MainActivity_emu48Start(JNIEnv *env, jobject thisz, jobject assetMgr, jobject bitmapMainScreen) {

    AndroidBitmapInfo androidBitmapInfo;
    void * pixels;
    int ret;
    if ((ret = AndroidBitmap_getInfo(env, bitmapMainScreen, &androidBitmapInfo)) < 0) {
        LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
        return;
    }
//    if ((ret = AndroidBitmap_lockPixels(env, bitmapMainScreen, &pixels)) < 0) {
//        LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
//    }
//
//    AndroidBitmap_unlockPixels(env, bitmapMainScreen);

    assetManager = AAssetManager_fromJava(env, assetMgr);
    emu48Start();
}
