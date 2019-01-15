# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

include_directories(src/main/cpp/win32)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
        native-lib

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).

        src/main/cpp/core/apple.c
#        src/main/cpp/core/cursor.c
#        src/main/cpp/core/ddeserv.c
#        src/main/cpp/core/debugger.c
        src/main/cpp/core/disasm.c
        src/main/cpp/core/dismem.c
        src/main/cpp/core/display.c
        src/main/cpp/core/disrpl.c
#        src/main/cpp/core/Emu48.c #-> To rewrite
        src/main/cpp/core/engine.c
        src/main/cpp/core/external.c
        src/main/cpp/core/fetch.c
        src/main/cpp/core/files.c
        src/main/cpp/core/i28f160.c
        src/main/cpp/core/keyboard.c
        src/main/cpp/core/keymacro.c
        src/main/cpp/core/kml.c
        src/main/cpp/core/lodepng.c
        src/main/cpp/core/lowbat.c
        src/main/cpp/core/mops.c
        src/main/cpp/core/mru.c
        src/main/cpp/core/opcodes.c
        src/main/cpp/core/pch.c
#        src/main/cpp/core/redeye.c
        src/main/cpp/core/rpl.c
#        src/main/cpp/core/serial.c
        src/main/cpp/core/settings.c
#        src/main/cpp/core/sndenum.c
        src/main/cpp/core/sound.c
        src/main/cpp/core/stack.c
        src/main/cpp/core/symbfile.c
        src/main/cpp/core/timer.c
#        src/main/cpp/core/udp.c

        src/main/cpp/win32-layer.c
        src/main/cpp/android-layer.c
        src/main/cpp/android-emu48.c # Emu48.c rewrite
        src/main/cpp/emu48-jni.c
)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
        native-lib
        android
        jnigraphics
        OpenSLES
        log)
