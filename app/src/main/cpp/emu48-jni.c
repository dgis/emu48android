//
// Created by cosnier on 12/11/2018.
//
#include <jni.h>
#include <stdio.h>

JNIEXPORT jstring JNICALL
Java_com_regis_cosnier_emu48_MainActivity_stringFromJNI(
                JNIEnv *env,
                jobject thisz) {
//    std::string hello = "Hello from C++";
//    return env->NewStringUTF(hello.c_str());
    return (*env)->NewStringUTF(env, "Hello from JNI !");
}

extern void emu48Start();
#include <asset_manager.h>
#include <asset_manager_jni.h>

JNIEXPORT void JNICALL Java_com_regis_cosnier_emu48_MainActivity_emu48Start(JNIEnv *env, jobject thisz, jobject assetManager) {
    AAssetManager * mgr = (AAssetManager *)assetManager;
    emu48Start();
}
