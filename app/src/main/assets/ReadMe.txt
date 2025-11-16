DESCRIPTION

This project ports the Windows application Emu48 written in C to Android.
It uses the Android NDK. The former Emu48 source code (written by Sébastien Carlier and Christoph Giesselink) remains untouched because of a thin win32 emulation layer above Linux/NDK!
This win32 layer will allow to easily update from the original Emu48 source code.
It can open or save the exact same state files (state.e48/e49) than the original Windows application!

Some KML files with theirs faceplates are embedded in the application but it is still possible to open a KML file and its dependencies by selecting a folder.

The application does not request any permission (because it opens the files or the KML folders using the content:// scheme).

The application is distributed with the same license under GPL and you can find the source code here:
https://github.com/dgis/emu48android


QUICK START

1. Click on the 3 dots button at the top left (or from the left side, slide your finger to open the menu).
2. Touch the "New..." menu item.
3. Select a default calculator (or "[Select a Custom KML script folder...]" where you have copied the KML scripts and ROM files (Android 11 may not be able to use the Download folder)).
4. And the calculator should now be opened.


NOTES

- The Help menu displays Emu48's original help HTML page and may not accurately reflect the behavior of this Android version.
- When using a custom KML script by selecting a folder, you must take care of the case sensitivity of its dependency files.
- Starting with the version 1.4, a RAM card generator for the port 2 of the HP48SX and HP48GX has been added.
  Like with the MKSHARED.EXE on Windows, you can generate the card in a file (i.e.: SHARED.BIN).
  And then, to use with the HP48SX or the HP48GX, you must select this generated file in the "Settings/Port2 File".
- By default when you create a new HP49/50 with the embedded readonly file "rom.49g",
  everything that you store in port 2 is lost just because the file "rom.49g" is READONLY.
  Since version 2.0, it is now possible from the menu to manage the Flash ROM file (it will fully replaces the ROM).
- To speed up printing, set the 'delay' to 0 in the calculator's print options.
- The serial ports, wire or infrared (infrared limited to 2400 baud) can now be used through the USB port in mode OTG.
  It uses the library https://github.com/mik3y/usb-serial-for-android, which allows to plug most of the serial USB adapters (only tested with Prolific and Ch34x),
  without the need to be root. If it is not automatic, please, activate the OTG mode in your Android device, and then,
  you should be able to see it in the Emu48 settings.
  It is still experimental and I see some issues when sending characters from the emulator to a real HP48 or HP49 with the kermit protocol.
  The communication can be artificially slowed down in this direction using an option.
  If the adapter is unplugged then plugged back in, you may need to call OPENIO/CLOSEIO.


NOT WORKING YET

- Disassembler
- Debugger


LINKS

- Original Emu48 for Windows from Christoph Giesselink and Sébastien Carlier: https://hp.giesselink.com/emu48.htm
- Emu48+ for Windows from Eric Rechlin and Cyrille de Brebission: https://www.hpcalc.org/details/6523
- Droid48 Reader app: https://play.google.com/store/apps/details?id=com.drehersoft.droid48reader or https://www.hpcalc.org/details/7366
- Smart Charlemagne (HP48SX Skins for Android): https://www.hpmuseum.org/forum/thread-14197-post-125336.html or https://www.hpcalc.org/details/9115
- The Museum of HP Calculators Forum: https://www.hpmuseum.org/forum/thread-12540.html


CHANGES

Version 2.9 (2025-11-15)

- Updated source code with Emu48 version 1.67+.
- Fix new UI constraints with Android 15.
- Increase Haptic feedback max duration.


Version 2.8 (2024-10-29)

- Updated source code with Emu48 version 1.66+.
- Fix an USB serial issue with Android 13+ (fix #23).
- Extend the KML MenuItem commands for Android only with: 201 (CUSTOM_PIXEL_BORDER_ON), 202 (CUSTOM_PIXEL_BORDER_OFF), 203 (CUSTOM_PIXEL_BORDER_TOGGLE) (fix #28).


Version 2.7 (2024-06-14)

- Updated source code with Emu48 version 1.65+. This new version improve the serial communication.
- Fix haptic feedback with Android 12 (API deprecation).
- Patch the ROM files to prevent the calculator to sleep, but not for HP 48gII/49G/50g (Fix #22).
- Fix a potential crash about the permission to access the files.
- Fix an issue when creating a new Flash ROM file from a custom KML file.
- Require at least Android 5.0 (4.4 previously).


Version 2.6 (2022-08-19)

- Updated source code from Eric Rechlin's Emu48 version 1.64+ that was merged from Christoph Gießelink's Emu48 version 1.65. This new version improve the serial communication.


Version 2.5 (2022-03-03)

- Allow to load RLE4, RLE8 and monochrome BMP images.
- Optimize the number of draw calls when displaying the LCD pixel borders.


Version 2.4 (2021-12-08)

- Updated source code from Eric Rechlin's Emu48 version 1.63+ that was merged from Christoph Gießelink's Emu48 version 1.64.


Version 2.3 (2021-10-19)

- Add an experimental serial port support (via USB OTG).
- Show KML log on request.
- Allows pressing a calculator button with the right button of the mouse and prevents its release to allow the On+A+F key combination (with Android version >= 5.0).
- Update the embedded help file "Emu48.html" to the latest version.
- Open an external web browser when you click an external links in the Help.
- Add Real blue 50g faceplate based on my calculator and on the KML script from Eric Rechlin.
- Display the graphic tab of the printer without antialiasing.
- Fix a crash about the Most Recently Used state files.
- Fix an issue with "Copy Screen".


Version 2.2 (2020-12-09)

- The KML folder is now well saved when changing the KML script for a custom one via the menu "Change KML Script...".
- Fix an issue when the permission to read the KML folder has been lost.


Version 2.1 (2020-11-23)

- Fix an issue which prevents to save all the settings (Save in onPause instead of onStop).


Version 2.0 (2020-11-15)

- Updated source code from Eric Rechlin's Emu48 version 1.62+ that was merged from Christoph Gießelink's Emu48 version 1.63.
- For the HP49/50 port 2, it is now possible to load a new Flash ROM file (It fully replaces the ROM).
- Replaces the haptic feedback switch with a slider to adjust the vibration duration.
- Fix transparency issue (RGB -> BGR).
- Fix a printer issue from Christoph Gießelink's HP82240B Printer Simulator version 1.12.
- Fix the KML button Type 3 with a Background offset which was not display at the right location (Fix #15).
- Fix a timer issue.
- Fix a freeze with a hp48 sx or gx, when switching on/off several times port 1 and 2!


Version 1.9 (2020-09-07)

- If the KML folder does not exist (like the first time), prompt the user to choose a new KML folder.
- If the memory card file for the port 2 cannot be found, prompt the user to choose a new memory card file.
- Move the KML folder in the JSON settings embedded in the state file because Windows cannot open the state file with KML url longer than 256 byte.
- Prevent to auto save before launching the "Open...", "Save As...", "Load Object...", "Save Object...", etc...
- Prevent app not responding (ANR) in NativeLib.buttonUp().
- In the menu header, switch the pixel format RGB to BGR when an icon of type BMP is defined in the KML script.


Version 1.8 (2020-05-24)

- Intercept the ESC keyboard key to allow the use of the BACK soft key.
- Add LCD pixel borders.
- Add support for the dark theme.
- Fix issues with Linux build (Fix #11).
- Remove the non loadable file from the MRU file list (Fix #13).
- Hide the menu [Default KML script folder] when the default is already displayed (Fix #5).
- Fix: Overlapping window source position when Background/Offset is not (0,0).
- Wrap the table of content in the former help documentation.
- Save the settings at the end of the state file.
- Transform all child activities with dialog fragments (to prevent unwanted state save).
- Fix an issue with the numpad keys which send the arrow keys and the numbers at the same time.
- Fix a major issue which prevented to open a state file (with a custom KML script) with Android 10.
- Optimize the speed with -Ofast option.


Version 1.7 (2019-12-12)

- Updated source code from Eric Rechlin's Emu48 version 1.61+ that was merged from Christoph Gießelink's Emu48 version 1.62.
- Allow to take a screenshot of the fullscreen including the skin.
- Add the KML Icon if present in the navigation menu header (only support PNG or 32bits BMP in the ICO file).
- Add an optional overlapping LCD part stuck to the screen when swiping the 2 calc parts.
- Improve loading speed by caching the KML folder.
- Support the transparency in the KML Global Color.
- Improve the New and Save menus.
- Sound volume can be adjusted by number by touching the number.


Version 1.6 (2019-07-15)

- Add option to prevent the pinch zoom.
- Prevent the white bottom bar when both options "Hide the status/navigations bar" and "Hide the menu button" are set (Github Fix: #9).
- Prevent the BACK/ESCAPE key to end the application only from a hardware keyboard (Github Fix: #10).


Version 1.5 (2019-07-11)

- Add the Ir printer simulator based on the Christoph Giesselink's HP82240B Printer Simulator for Windows.
- Add the macro support.
- Refactor the code for easier code sharing between Emu48, Emu42 and Emu71.
- Fix: Bad text characters when copy/paste the stack.
- Fix: Selecting an empty KML folder prevent to select the default embedded KML folder (Github Fix: #5)!
- Fix a crash with waveOutClose().
- Fix an issue with the Pan and zoom which was possible after closing the calc.
- Prevent the ESC key from leaving the application (Github Fix: #6).
- Map the keyboard DELETE key like it should (Github Fix: #6).
- Map the +, -, * and / keys catching the typed character instead of the virtual key (Github Fix: #6).
- Fix the printer icon in the Eric's script "real49gp-lc.kml" and "real50g-lc.kml".
- Improve the swipe gesture.
- Fix the On+D tests for hp49/50 by mocking "serial.c".


Version 1.4 (2019-06-08)

- Add an optional menu button in the top left corner.
- Add a RAM card generator for the port 2 of the HP48SX and HP48GX.
- Add the possibility to hide the status and/or the navigation bars.
- Rewrite the timers engine (Hoping this fixes the issue with the stuck "busy" annunciator.)
- Update the Win32 layer from Emu42 dev.
- Fix the authentic speed issue at the first start.
- Fix the non working Restore/Delete backup.
- Prevent to load/save object and copy/paste with HP39/40.
- Add a volume slider in the settings.
- Add a rotation option.
- Warn the user about the KML folder selection if this is Android < 5.0


Version 1.3 (2019-04-04)

WARNING: WITH VERSION 1.3, THE STATUS FILE HAS BEEN MODIFIED AND IS NOW FULLY COMPATIBLE WITH THE WINDOWS VERSION AS IT SHOULD HAVE BEEN.
HOWEVER, BEFORE THE UPDATE, BACK UP YOUR DATA BECAUSE YOU COULD LOSE THEM.

- Fix the red and blue color inversion.
- Add the HP 50g (Calypso 2K/4K) KML script from Carl Reinke (4K is a slow because there is no hardware acceleration).
- Fix a bug about the timer delay in timeSetEvent().
- Fix deprecated classes in the settings.
- Add an optional haptic feedback when touching a button.
- Fix the intermittent slow down due to the end of playing a sound.
- Fix blank screen when switching the KML faceplate.
- Add more traces in the win32 log.
- Allow to select the background color (missing a custom color).
- Thanks to Christoph Giesselink about the state file compatibility issue (sizeof(BOOL) should be 4 and not 1).
- Allow to fully switch the sound off.


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
The Eric's Real scripts ("real*.kml" and "real*.bmp/png") are embedded in this application with the kind permission of Eric Rechlin.

Portions of this source code (about the usb-serial) were originally created by Google Inc. in 2011-2013 and Mike Wakerly in 2013.
