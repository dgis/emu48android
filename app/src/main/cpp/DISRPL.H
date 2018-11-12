/*
 *   disrpl.h
 *
 *   This file is part of Emu48
 *
 *   Copyright (C) 2008 Christoph Gieﬂelink
 *
 */

// RPL platform type
#define RPL_P1	          (1<<0)			// Clamshell without RRP
#define RPL_P2	(RPL_P1 | (1<<1))			// Pioneer / Clamshell
#define RPL_P3	(RPL_P2 | (1<<2))			// Charlemagne
#define RPL_P4	(RPL_P3 | (1<<3))			// Alcuin
#define RPL_P5	(RPL_P4 | (1<<4))			// V'ger

extern DWORD   dwRplPlatform;				// RPL platform
extern BOOL    bRplViewName;				// show entry point name
extern BOOL    bRplViewAddr;				// show adress
extern BOOL    bRplViewBin;					// show binary data
extern BOOL    bRplViewAsm;					// show ASM code instead of hex data
extern BYTE    (*RplReadNibble)(DWORD *p);	// read nibble function pointer
extern DWORD   RplSkipObject(DWORD dwAddr);
extern LPTSTR  RplDecodeObject(DWORD dwAddr, DWORD *pdwNxtAddr);
extern LPTSTR  RplCreateObjView(DWORD dwStartAddr, DWORD dwEndAddr, BOOL bSingleObj);
