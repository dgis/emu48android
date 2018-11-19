#include "pch.h"
#include "Emu48.h"

// Redeye.c
VOID IrPrinter(BYTE c) {
}

// Serial.c
BOOL CommOpen(LPTSTR strWirePort,LPTSTR strIrPort) {
    return 0;
}
VOID CommClose(VOID) {
}
VOID CommSetBaud(VOID) {
}
BOOL UpdateUSRQ(VOID) {
    return 0;
}
VOID CommTxBRK(VOID) {
}
VOID CommTransmit(VOID) {
}
VOID CommReceive(VOID) {
}

// Sound.c
DWORD dwWaveVol;
DWORD dwWaveTime;
VOID  SoundOut(CHIPSET* w, WORD wOut) {
}
VOID  SoundBeep(DWORD dwFrequency, DWORD dwDuration) {
}