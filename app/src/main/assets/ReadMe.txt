DESCRIPTION

This project ports the Windows application Emu48 written in C to Android.
It uses the Android NDK. The former Emu48 source code remains untouch because of a thin win32 emulation layer above Linux/NDK!
This win32 layer will allow to easily update from the original Emu48 source code.
It can open or save the exact same state files (state.e48/e49) than the original Windows application!


QUICK START

1. Touch the top left button and select "Open a new image".
2. The image is analyzed and should appear with the numbered areas.
3. To check the result, you can zoom with two fingers or pan with one.
4. The top right button opens the parameters panel which allows changing the number of color, the size of the numbers, etc...
5. To print the result, touch the top left button and select "Export to PDF".


NOT WORKING YET

- Disassembler
- Debugger
- Macro
- Infrared Printer
- Serial Ports (Wire or Ir)


CHANGES

Version 1.0 (2019-02-XX)

- First public version available on the store.


LICENSES

Android version by Régis COSNIER.
This program is based on Emu48 for Windows version, copyrighted by Christoph Gießelink & Sébastien Carlier, with the addition of a win32 layer to run on Android.

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

Note: some included files are not covered by the GPL; these include ROM image files (copyrighted by HP), KML files and faceplate images (copyrighted by their authors).
