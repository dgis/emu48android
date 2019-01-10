ABOUT
This project ports the Windows application Emu48 written in C to Android.
It uses the Android NDK. The former Emu48 source code remains untouch because of a thin win32 emulation layer above Linux/NDK!
This win32 layer will allow to easily update from the original Emu48 source code.
It can open or save the exact same state files (state.e48/e49) than the original Windows application!

NOT WORKING
- Macro
- Infrared Printer
- Serial Ports (Wire or Ir)
- Disassembler
- Debugger

TODO
- If KML not found in OpenDocument, allow to choose a new one
- Improve loading errors
- Allows to see the errors in a log
- Improve button support with HDC operations
- Bug: Port1 is not enable from the state.e48 file!
- Option to allow rotation
- Option to auto hide the menu
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
