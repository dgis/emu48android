# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
        native-lib

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        #src/main/cpp/native-lib.cpp

        src/main/cpp/android-layer.c
        src/main/cpp/win32-layer.c

        src/main/cpp/apple.c
#        src/main/cpp/cursor.c
#        src/main/cpp/ddeserv.c
#        src/main/cpp/debugger.c
#        src/main/cpp/disasm.c
        src/main/cpp/dismem.c
#        src/main/cpp/display.c #-> To rewrite
        src/main/cpp/disrpl.c
#        src/main/cpp/Emu48.c #-> To rewrite
        src/main/cpp/engine.c
        src/main/cpp/external.c
        src/main/cpp/fetch.c
        src/main/cpp/files.c
        src/main/cpp/i28f160.c
        src/main/cpp/keyboard.c
#        src/main/cpp/keymacro.c
#        src/main/cpp/kml.c #-> To rewrite or TODO
        src/main/cpp/lodepng.c
        src/main/cpp/lowbat.c
        src/main/cpp/mops.c
#        src/main/cpp/mru.c
        src/main/cpp/opcodes.c
        src/main/cpp/pch.c
#        src/main/cpp/redeye.c
        src/main/cpp/rpl.c
#        src/main/cpp/serial.c
        src/main/cpp/settings.c
#        src/main/cpp/sndenum.c
#        src/main/cpp/sound.c
        src/main/cpp/stack.c
        src/main/cpp/symbfile.c
        src/main/cpp/timer.c
#        src/main/cpp/udp.c


        src/main/cpp/emu48-jni.c
)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
        native-lib

        # Links the target library to the log library
        # included in the NDK.
        ${log-lib})