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
