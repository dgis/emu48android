ABOUT
This project ports the Windows application Emu48 written in C to Android.
It uses the Android NDK. The former Emu48 source code remains untouch because a thin win32 emulation layer above Linux/NDK!
This win32 layer will allow to easily update from the original Emu48 source code.
It can open or save the exact same state files (state.e48/e49) than the original Windows application!

NOT WORKING
- Sound
- Disassembler
- Debugger
- Macro
- Infrared Printer
- Serial Ports (Wire or Ir)

TODO
- Add help and about
- Change KML
- Support 8bits images
- Put the KML title in the header of the menu in the drawer
- Bug: No refresh with the clock
- Option to allow rotation
- Option to auto hide the menu
- Bitmap corruption when touching the buttons
- Android UI Settings
- Sound

DONE
- Bug Red and Blue seems inverted.
- Multitouch
- Choose KML/Change KML/NewDocument
- Load/Save object
- Permission issues when reopening document after an OS restart
- Change settings per settings
- Add icons
- Autosave
