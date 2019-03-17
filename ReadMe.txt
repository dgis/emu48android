DESCRIPTION

This project ports the Windows application Emu48 written in C to Android.
It uses the Android NDK. The former Emu48 source code remains untouched because of a thin win32 emulation layer above Linux/NDK!
This win32 layer will allow to easily update from the original Emu48 source code.
It can open or save the exact same state files (state.e48/e49) than the original Windows application!

Some KML files with theirs faceplates are embedded in the application but it is still possible to open a KML file and its dependencies by selecting a folder.

The application does not request any permission (because it opens the files or the KML folders using the content:// scheme).

The application is distributed with the same license under GPL and you can find the source code here:
https://github.com/dgis/emu48android


QUICK START

1. From the left side, slide your finger to open the menu.
2. Touch the "New..." menu item.
3. Select a predefined faceplate (or select a custom KML script folder).
4. And the calculator should now be opened.


NOTES

- When using a custom KML script by selecting a folder, you must take care of the case sensitivity of its dependency files.


NOT WORKING YET

- Disassembler
- Debugger
- Macro
- Infrared Printer
- Serial Ports (Wire or Ir)


CHANGES

Version 1.3alpha (2019-03-xx)

- Fix the red and blue color inversion.


Version 1.2 (2019-03-14)

- Use the KML Global color as background color.
- Set the extension .e49 when "Saving as" a state file with model 'Q'.
- Fix a crash after opening several times a state file.
- Fix On-D diagnostic not working for 48gII/49G/49g+/50g (rom 2.15 not good, change for 2.10).
- Improve the scrolling issue found in Emu48 1.59+.
- Rewrite the StretchBlt() function to improve the pixel rendering.
- Allow to build the project with "gradlew build".
- Fix issues with back button in the Settings, Help and About.
- Build with Android 4.4 support (Not sure the settings work).
- Prevent empty MRU.
- Allow to go back from the settings in Android 4.4 and may be more recent versions.
- Save the Port 2 at the same time we save the state file.
- Change the name from "Emu48 for Android" for "Emu48".
- Always prompt to save when closing.
- Fix MostRecentUsed file issue.
- Update the core source code to Emu48 1.60+.


Version 1.1 (2019-03-01)

- Update the KML scripts and the images from Eric Rechlin.
- Fix crash when changing the main image.
- Fix an issue when KML file is not found.


Version 1.0 (2019-02-28)

- First public version available on the store.


LICENSES

Android version by Régis COSNIER.
This program is based on Emu48 for Windows version, copyrighted by Christoph Gießelink & Sébastien Carlier, with the addition of a win32 layer to run on Android.

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

Note: some included files are not covered by the GPL; these include ROM image files (copyrighted by HP), KML files and faceplate images (copyrighted by their authors).
The Eric's Real scripts ("real*.kml" and "real*.bmp") are embedded in this application with the kind permission of Eric Rechlin.


TODO

- Sometimes, it seems to be in authentic calculator slow speed and it should not be.
- Add a separation between the pixels (Suggestion from Jaime Meza)
- Sometimes the "busy" annunciator gets stuck
- Add KML script loading dependencies fallback to the inner ROM (and may be KML include?)
- Add haptic feedback when touch a button
- Add a true fullscreen mode under the status bar and the bottom buttons
- Improve the access to the menu
- Change the logo following the template


BUILD

Emu48 for Android is built with Android Studio 3.3 (2019).
And to generate an installable APK file with a real Android device, it MUST be signed.

Either use Android Studio:
* In menu "Build"
* Select "Generate Signed Bundle / APK..."
* Select "APK", then "Next"
* "Create new..." (or use an existing key store file)
* Enter "Key store password", "Key alias" and "Key password", then "Next"
* Select a "Destination folder:"
* Select the "Build Variants:" "release"
* Select the "Signature Versions:" "V1" (V1 only)
* Finish

Or in the command line, build the signed APK:
* In the root folder, create a keystore.jks file with:
** keytool -genkey -keystore ./keystore.jks -keyalg RSA -validity 9125 -alias key0
** keytool -genkeypair -v -keystore ./keystore.jks -keyalg RSA -validity 9125 -alias key0
* create the file ./keystore.properties , with the following properties:
    storeFile=../keystore.jks
    storePassword=myPassword
    keyAlias=key0
    keyPassword=myPassword
* gradlew build
* The APK should be in the folder app/build/outputs/apk/release

Then, you should be able to use this fresh APK file with an Android device.
