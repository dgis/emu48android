/*
 *   Emu48Dll.h
 *
 *   This file is part of Emu48
 *
 *   Copyright (C) 2000 Christoph Gießelink
 *
 */

#define DECLSPEC __declspec(dllexport)

//////////////////////////////////
//
//  breakpoint type definitions
//
//////////////////////////////////

#define BP_EXEC		0x01					// code breakpoint
#define BP_READ		0x02					// read memory breakpoint
#define BP_WRITE	0x04					// write memory breakpoint
#define BP_RPL      0x08					// RPL breakpoint
#define BP_ACCESS	(BP_READ|BP_WRITE)		// read/write memory breakpoint

#define BP_ROM		0x8000					// absolute ROM adress breakpoint

//////////////////////////////////
//
//  REGISTER ACCESS API
//
//////////////////////////////////

#define EMU_REGISTER_PC      0
#define EMU_REGISTER_D0      1
#define EMU_REGISTER_D1      2
#define EMU_REGISTER_DUMMY   3
#define EMU_REGISTER_AL      4
#define EMU_REGISTER_AH      5
#define EMU_REGISTER_BL      6
#define EMU_REGISTER_BH      7
#define EMU_REGISTER_CL      8
#define EMU_REGISTER_CH      9
#define EMU_REGISTER_DL     10
#define EMU_REGISTER_DH     11
#define EMU_REGISTER_R0L    12
#define EMU_REGISTER_R0H    13
#define EMU_REGISTER_R1L    14
#define EMU_REGISTER_R1H    15
#define EMU_REGISTER_R2L    16
#define EMU_REGISTER_R2H    17
#define EMU_REGISTER_R3L    18
#define EMU_REGISTER_R3H    19
#define EMU_REGISTER_R4L    20
#define EMU_REGISTER_R4H    21
#define EMU_REGISTER_R5L    22
#define EMU_REGISTER_R5H    23
#define EMU_REGISTER_R6L    24
#define EMU_REGISTER_R6H    25
#define EMU_REGISTER_R7L    26
#define EMU_REGISTER_R7H    27
#define EMU_REGISTER_FLAGS  28
#define EMU_REGISTER_OUT    29
#define EMU_REGISTER_IN     30
#define EMU_REGISTER_VIEW1  31
#define EMU_REGISTER_VIEW2  32
#define EMU_REGISTER_RSTKP  63
#define EMU_REGISTER_RSTK0  64
#define EMU_REGISTER_RSTK1  65
#define EMU_REGISTER_RSTK2  66
#define EMU_REGISTER_RSTK3  67
#define EMU_REGISTER_RSTK4  68
#define EMU_REGISTER_RSTK5  69
#define EMU_REGISTER_RSTK6  70
#define EMU_REGISTER_RSTK7  71
#define EMU_REGISTER_CLKL   72
#define EMU_REGISTER_CLKH   73
#define EMU_REGISTER_CRC    74

/**
 *  "FLAGS" register format :
 *
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |               ST              |S|x|x|x|K|I|C|M|  HST  |   P   |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  M : Mode (0:Hex, 1:Dec)
 *  C : Carry
 *  I : Interrupt pending
 *  K : KDN Interrupts Enabled
 *  S : Shutdn Flag (read only)
 *  x : reserved
 */

/****************************************************************************
* EmuCreate
*****************************************************************************
*
* @func   start Emu48 and load Ram file into emulator, if Ram file don't
*         exist create a new one and save it under the given name
*
* @xref   none
*
* @rdesc  BOOL: FALSE = OK, TRUE = Error
*
****************************************************************************/

EXTERN_C DECLSPEC BOOL CALLBACK EmuCreate(
	LPCTSTR lpszFilename);					// @parm String with RAM filename

/****************************************************************************
* EmuCreateEx
*****************************************************************************
*
* @func   start Emu48 and load Ram and Port2 file into emulator, if Ram file
*         don't exist create a new one and save it under the given name
*
* @xref   none
*
* @rdesc  BOOL: FALSE = OK, TRUE = Error
*
****************************************************************************/

EXTERN_C DECLSPEC BOOL CALLBACK EmuCreateEx(
	LPCTSTR lpszFilename,					// @parm String with RAM filename
	LPCTSTR lpszPort2Name);					// @parm String with Port2 filename
											//       or NULL for using name inside INI file

/****************************************************************************
* EmuDestroy
*****************************************************************************
*
* @func   close Emu48, free all memory
*
* @xref   none
*
* @rdesc  BOOL: FALSE = OK, TRUE = Error
*
****************************************************************************/

EXTERN_C DECLSPEC BOOL CALLBACK EmuDestroy(VOID);

/****************************************************************************
* EmuAcceleratorTable
*****************************************************************************
*
* @func   load accelerator table of emulator
*
* @xref   none
*
* @rdesc  HACCEL: handle of the loaded accelerator table
*
****************************************************************************/

EXTERN_C DECLSPEC HACCEL CALLBACK EmuAcceleratorTable(
	HWND *phEmuWnd);							// @parm return of emulator window handle

/****************************************************************************
* EmuCallBackClose
*****************************************************************************
*
* @func   init CallBack handler to notify caller when Emu48 window close
*
* @xref   none
*
* @rdesc  VOID
*
****************************************************************************/

EXTERN_C DECLSPEC VOID CALLBACK EmuCallBackClose(
	VOID (CALLBACK *EmuClose)(VOID));		// @parm CallBack function notify caller Emu48 closed

/****************************************************************************
* EmuCallBackDocumentNotify
*****************************************************************************
*
* @func   init CallBack handler to notify caller for actual document file
*
* @xref   none
*
* @rdesc  VOID
*
****************************************************************************/

EXTERN_C DECLSPEC VOID CALLBACK EmuCallBackDocumentNotify(
	VOID (CALLBACK *EmuDocumentNotify)(LPCTSTR lpszFilename)); // @parm CallBack function notify document filename

/****************************************************************************
* EmuLoadRamFile
*****************************************************************************
*
* @func   load Ram file into emulator
*
* @xref   none
*
* @rdesc  BOOL: FALSE = OK, TRUE = Error (old file reloaded)
*
****************************************************************************/

EXTERN_C DECLSPEC BOOL CALLBACK EmuLoadRamFile(
	LPCTSTR lpszFilename);					// @parm String with RAM filename

/****************************************************************************
* EmuSaveRamFile
*****************************************************************************
*
* @func   save the current emulator Ram to file
*
* @xref   none
*
* @rdesc  BOOL: FALSE = OK, TRUE = Error (old file reloaded)
*
****************************************************************************/

EXTERN_C DECLSPEC BOOL CALLBACK EmuSaveRamFile(VOID);

/****************************************************************************
* EmuLoadObject
*****************************************************************************
*
* @func   load object file to stack
*
* @xref   none
*
* @rdesc  BOOL: FALSE = OK, TRUE = Error
*
****************************************************************************/

EXTERN_C DECLSPEC BOOL CALLBACK EmuLoadObject(
	LPCTSTR lpszObjectFilename);			// @parm String with object filename

/****************************************************************************
* EmuSaveObject
*****************************************************************************
*
* @func   save object on stack to file
*
* @xref   none
*
* @rdesc  BOOL: FALSE = OK, TRUE = Error
*
****************************************************************************/

EXTERN_C DECLSPEC BOOL CALLBACK EmuSaveObject(
	LPCTSTR lpszObjectFilename);			// @parm String with object filename

/****************************************************************************
* EmuCalculatorType
*****************************************************************************
*
* @func   get ID of current calculator type
*
* @xref   none
*
* @rdesc  BYTE: '6' = HP38G with 64KB RAM
*               'A' = HP38G
*               'E' = HP39/40G
*               'S' = HP48SX
*               'G' = HP48GX
*               'X' = HP49G
*               'P' = HP39G+
*               '2' = HP48GII
*               'Q' = HP49G+
*
****************************************************************************/

EXTERN_C DECLSPEC BYTE CALLBACK EmuCalculatorType(VOID);

/****************************************************************************
* EmuSimulateKey
*****************************************************************************
*
* @func   simulate a key action
*
* @xref   none
*
* @rdesc  VOID
*
****************************************************************************/

EXTERN_C DECLSPEC VOID CALLBACK EmuSimulateKey(
	BOOL bKeyState,							// @parm TRUE = pressed, FALSE = released
	UINT out,								// @parm key out line
	UINT in);								// @parm key in  line

/****************************************************************************
* EmuPressOn
*****************************************************************************
*
* @func   press On key (emulation must run)
*
* @xref   none
*
* @rdesc  VOID
*
****************************************************************************/

EXTERN_C DECLSPEC VOID CALLBACK EmuPressOn(
	BOOL bKeyState);						// @parm TRUE = pressed, FALSE = released

/****************************************************************************
* EmuInitLastInstr
*****************************************************************************
*
* @func   init a circular buffer area for saving the last instruction
*         addresses
*
* @xref   none
*
* @rdesc  BOOL: FALSE = OK, TRUE = Error
*
****************************************************************************/

EXTERN_C DECLSPEC BOOL CALLBACK EmuInitLastInstr(
	WORD wNoInstr,							// @parm number of saved instructions,
											//       0 = frees the memory buffer
	DWORD *pdwArray);						// @parm pointer to linear array

/****************************************************************************
* EmuGetLastInstr
*****************************************************************************
*
* @func   return number of valid entries in the last instruction array,
*         each entry contents a PC address, array[0] contents the oldest,
*         array[*pwNoEntries-1] the last PC address
*
* @xref   none
*
* @rdesc  BOOL: FALSE = OK, TRUE = Error
*
****************************************************************************/

EXTERN_C DECLSPEC BOOL CALLBACK EmuGetLastInstr(
	WORD *pwNoEntries);						// @parm return number of valid entries in array

/****************************************************************************
* EmuRun
*****************************************************************************
*
* @func   run emulation
*
* @xref   none
*
* @rdesc  BOOL: FALSE = OK
*               TRUE  = Error, Emu48 is running
*
****************************************************************************/

EXTERN_C DECLSPEC BOOL CALLBACK EmuRun(VOID);

/****************************************************************************
* EmuRunPC
*****************************************************************************
*
* @func   run emulation until stop address
*
* @xref   none
*
* @rdesc  VOID
*
****************************************************************************/

EXTERN_C DECLSPEC VOID CALLBACK EmuRunPC(
	DWORD dwAddressPC);						// @parm breakpoint address

/****************************************************************************
* EmuStep
*****************************************************************************
*
* @func   execute one ASM instruction and return to caller
*
* @xref   none
*
* @rdesc  VOID
*
****************************************************************************/

EXTERN_C DECLSPEC VOID CALLBACK EmuStep(VOID);

/****************************************************************************
* EmuStepOver
*****************************************************************************
*
* @func   execute one ASM instruction but skip GOSUB, GOSUBL, GOSBVL
*         subroutines
*
* @xref   none
*
* @rdesc  VOID
*
****************************************************************************/

EXTERN_C DECLSPEC VOID CALLBACK EmuStepOver(VOID);

/****************************************************************************
* EmuStepOut
*****************************************************************************
*
* @func   run emulation until a RTI, RTN, RTNC, RTNCC, RTNNC, RTNSC, RTNSXN,
*         RTNYES instruction is found above the current stack level
*
* @xref   none
*
* @rdesc  VOID
*
****************************************************************************/

EXTERN_C DECLSPEC VOID CALLBACK EmuStepOut(VOID);

/****************************************************************************
* EmuStop
*****************************************************************************
*
* @func   break emulation
*
* @xref   none
*
* @rdesc  BOOL: FALSE = OK
*               TRUE  = Error, no debug notify handler
*
****************************************************************************/

EXTERN_C DECLSPEC BOOL CALLBACK EmuStop(VOID);

/****************************************************************************
* EmuCallBackDebugNotify
*****************************************************************************
*
* @func   init CallBack handler to notify caller on debugger breakpoint
*
* @xref   none
*
* @rdesc  VOID
*
****************************************************************************/

EXTERN_C DECLSPEC VOID CALLBACK EmuCallBackDebugNotify(
    VOID (CALLBACK *EmuDbgNotify)(INT nBreaktype)); // @parm CallBack function notify Debug breakpoint

/****************************************************************************
* EmuCallBackStackNotify
*****************************************************************************
*
* @func   init CallBack handler to notify caller on hardware stack change;
*         if the CallBack function return TRUE, emulation stops behind the
*         opcode changed hardware stack content, otherwise, if the CallBack
*         function return FALSE, no breakpoint is set
*
* @xref   none
*
* @rdesc  VOID
*
****************************************************************************/

EXTERN_C DECLSPEC VOID CALLBACK EmuCallBackStackNotify(
	BOOL (CALLBACK *EmuStackNotify)(VOID));	// @parm CallBack function notify stack changed

/****************************************************************************
* EmuGetRegister
*****************************************************************************
*
* @func   read a 32 bit register
*
* @xref   none
*
* @rdesc  DWORD: 32 bit value of register
*
****************************************************************************/

EXTERN_C DECLSPEC DWORD CALLBACK EmuGetRegister(
	UINT uRegister);						// @parm index of register

/****************************************************************************
* EmuSetRegister
*****************************************************************************
*
* @func   write a 32 bit register
*
* @xref   none
*
* @rdesc  VOID
*
****************************************************************************/

EXTERN_C DECLSPEC VOID CALLBACK EmuSetRegister(
	UINT uRegister,							// @parm index of register
	DWORD dwValue);							// @parm new 32 bit value

/****************************************************************************
* EmuGetMem
*****************************************************************************
*
* @func   read one nibble from the specified mapped address
*
* @xref   none
*
* @rdesc  VOID
*
****************************************************************************/

EXTERN_C DECLSPEC VOID CALLBACK EmuGetMem(
	DWORD dwMapAddr,						// @parm mapped address of Saturn CPU
	BYTE  *pbyValue);						// @parm readed nibble

/****************************************************************************
* EmuSetMem
*****************************************************************************
*
* @func   write one nibble to the specified mapped address
*
* @xref   none
*
* @rdesc  VOID
*
****************************************************************************/

EXTERN_C DECLSPEC VOID CALLBACK EmuSetMem(
	DWORD dwMapAddr,						// @parm mapped address of Saturn CPU
	BYTE  byValue);							// @parm nibble to write

/****************************************************************************
* EmuGetRom
*****************************************************************************
*
* @func   return size and base address of mapped ROM
*
* @xref   none
*
* @rdesc  LPBYTE: base address of ROM (pointer to original data)
*
****************************************************************************/

EXTERN_C DECLSPEC LPBYTE CALLBACK EmuGetRom(
	DWORD *pdwRomSize);						// @parm return size of ROM in nibbles

/****************************************************************************
* EmuSetBreakpoint
*****************************************************************************
*
* @func   set ASM code/data breakpoint
*
* @xref   none
*
* @rdesc  BOOL: TRUE  = Error, Breakpoint table full
*               FALSE = OK, Breakpoint set
*
****************************************************************************/

EXTERN_C DECLSPEC BOOL CALLBACK EmuSetBreakpoint(
	DWORD dwAddress,						// @parm breakpoint address to set
	UINT  nBreakpointType);					// @parm breakpoint type to set

/****************************************************************************
* EmuClearBreakpoint
*****************************************************************************
*
* @func   clear ASM code/data breakpoint
*
* @xref   none
*
* @rdesc  BOOL: TRUE  = Error, Breakpoint not found
*               FALSE = OK, Breakpoint cleared
*
****************************************************************************/

EXTERN_C DECLSPEC BOOL CALLBACK EmuClearBreakpoint(
	DWORD dwAddress,						// @parm breakpoint address to clear
	UINT  nBreakpointType);					// @parm breakpoint type to clear

/****************************************************************************
* EmuClearAllBreakpoints
*****************************************************************************
*
* @func   clear all breakpoints
*
* @xref   none
*
* @rdesc  VOID
*
****************************************************************************/

EXTERN_C DECLSPEC VOID CALLBACK EmuClearAllBreakpoints(VOID);

/****************************************************************************
* EmuEnableNop3Breakpoint
*****************************************************************************
*
* @func   enable/disable NOP3 breakpoint
*         stop emulation at Opcode 420 for GOC + (next line)
*
* @xref   none
*
* @rdesc  VOID
*
****************************************************************************/

EXTERN_C DECLSPEC VOID CALLBACK EmuEnableNop3Breakpoint(
	BOOL bEnable);							// @parm stop on NOP3 opcode

/****************************************************************************
* EmuEnableDocodeBreakpoint
*****************************************************************************
*
* @func   enable/disable DOCODE breakpoint
*
* @xref   none
*
* @rdesc  VOID
*
****************************************************************************/

EXTERN_C DECLSPEC VOID CALLBACK EmuEnableDoCodeBreakpoint(
	BOOL bEnable);							// @parm stop on DOCODE entry

/****************************************************************************
* EmuEnableRplBreakpoint
*****************************************************************************
*
* @func   enable/disable RPL breakpoint
*
* @xref   none
*
* @rdesc  VOID
*
****************************************************************************/

EXTERN_C DECLSPEC VOID CALLBACK EmuEnableRplBreakpoint(
	BOOL bEnable);							// @parm stop on RPL exit

/****************************************************************************
* EmuEnableSkipInterruptCode
*****************************************************************************
*
* @func   enable/disable skip single step execution inside the interrupt
*         handler, this option has no effect on code and data breakpoints
*
* @xref   none
*
* @rdesc  VOID
*
****************************************************************************/

EXTERN_C DECLSPEC VOID CALLBACK EmuEnableSkipInterruptCode(
	BOOL bEnable);							// @parm TRUE = skip code instructions
											//              inside interrupt service routine
