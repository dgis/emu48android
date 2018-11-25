//
// Created by cosnier on 12/11/2018.
//
#include <jni.h>
#include <stdio.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

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

JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_MainActivity_emu48Start(JNIEnv *env, jobject thisz, jobject assetMgr) {
    assetManager = AAssetManager_fromJava(env, assetMgr);
    emu48Start();
}
