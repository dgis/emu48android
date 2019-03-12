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

- When using a custom KML script by selecting a folder, you must take care of the case sensitivity of its dependencies.


NOT WORKING YET

- Disassembler
- Debugger
- Macro
- Infrared Printer
- Serial Ports (Wire or Ir)


CHANGES

Version 1.2alpha (2019-03-XX)

- Use the KML Global color as background color.
- Set the extension .e49 when "Saving as" a state file with model 'Q'.
- Fix a crash after opening several times a state file.
- Fix On-D diagnostic not working for 48gII/49G/49g+/50g (rom 2.15 not good, change for 2.10).
- Improve the scrolling issue found in Emu48 1.59+.
- Rewrite the StretchBlt() function to improve the pixel rendering.
- Allow to build the project with "gradlew build".
- Fix issues with back button in the Settings, Help and About.
- Build with Android 4.4 support.
- Prevent empty MRU.
- Allow to go back from the settings in Android 4.4 and may be more recent versions.
- Save the Port 2 at the same time we save the state file.


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
- But in the MRU (the MRU is not always well refreshed)
- Issue with random settings in Android 4.4 emulator
- Add a separation between the pixels (Suggestion from Jaime Meza)
- Sometimes the "busy" annunciator gets stuck
- Add KML script loading dependencies fallback to the inner ROM (and may be KML include?)
- Add haptic feedback when touch a button
- Add a true fullscreen mode under the status bar and the bottom buttons
- Improve the access to the menu
- Change the logo following the template

DONE
- Bug Red and Blue seems inverted
- Multitouch
- Choose KML/Change KML/NewDocument
- Load/Save object
- Permission issues when reopening document after an OS restart
- Android UI Settings
- Change settings per settings
- Add icons
- Autosave
- Add help and about
- Change KML
- Bug Re open file not working when permanent link
- Put the KML title in the header of the menu in the drawer
- Fix Reset calculator
- Support clipboard
- Support screen copy
- Add HP48SX icon
- Settings show KML log should be true by default.
- Sound
- Bug: No refresh with the clock (Fix timers)
- Support 8bits images
- Add recent files
- Bug: Port2 not saved
- Fix Process not closed when leaving
- Bug: Port1 is not enable from the state.e48 file!
- Bug no preference event sometimes!
- Bug with android back button acts as the hp48 back button!
- Add new KMLs and ROMs.
- If KML not found in OpenDocument, allow to choose a new one
- Allow to fill the screen
- Option to allow rotation
- Find a new open source package name (org.emulator.forty.eight)
- Improve button support with HDC operations
- Improve loading errors (and see the errors in a log)
- Allow to load external KML/BMP/ROM files
- Fix sound error at the initialization
- Open Emu48 with a state (content://) file shared with it
- Add sound switch settings
- Pixel alignment (pixel squeeze?) issue
- Build with Android 4.4
