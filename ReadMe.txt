ABOUT
This project ports the Windows application Emu48 written in C to Android.
It uses the Android NDK. The former Emu48 source code remains untouch because of a thin win32 emulation layer above Linux/NDK!
This win32 layer will allow to easily update from the original Emu48 source code.
It can open or save the exact same state files (state.e48/e49) than the original Windows application!

NOT WORKING YET
- Macro
- Infrared Printer
- Serial Ports (Wire or Ir)
- Disassembler
- Debugger

TODO
- Bug: custom KML/Save/ChangeKML/Close -> state not saved
- Improve the access to the menu
- Change the logo following the template
- Open Emu48 with a state file shared with it (Can not work)

DONE
- Bug Red and Blue seems inverted.
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



LICENSE

Android version by Régis COSNIER.
This program is based on Emu48 for Windows version, copyrighted by Christoph Gießelink & Sébastien Carlier, with the addition of a win32 layer to run on Android.

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

Note: some included files are not covered by the GPL; these include ROM image files (copyrighted by HP), KML files and faceplate images (copyrighted by their authors).
