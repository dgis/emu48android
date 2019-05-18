#include "core/pch.h"
#include "core/Emu48.h"

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

// udp.c
TCHAR szUdpServer[1024] = _T("localhost");
WORD  wUdpPort = 5025;						// scpi-raw

VOID ResetUdp(VOID) {
    return;
}

BOOL SendByteUdp(BYTE byData) {
    return FALSE;
}

// debugger.c
VOID UpdateDbgCycleCounter(VOID) {
    return;
}
BOOL CheckBreakpoint(DWORD dwAddr, DWORD wRange, UINT nType) {
    return 0;
}
VOID NotifyDebugger(INT nType) {
    return;
}
VOID DisableDebugger(VOID) {
    return;
}
LRESULT OnToolDebug(VOID) {
    return 0;
}
VOID LoadBreakpointList(HANDLE hFile) {
    return;
}
VOID SaveBreakpointList(HANDLE hFile) {
    return;
}
VOID CreateBackupBreakpointList(VOID) {
    return;
}
VOID RestoreBackupBreakpointList(VOID) {
    return;
}
