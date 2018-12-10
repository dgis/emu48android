/*
 *   DebugDll.c
 *
 *   This file is part of Emu48
 *
 *   Copyright (C) 2000 Christoph Gießelink
 *
 */
#include "pch.h"
#include "resource.h"
#include "Emu48.h"
#include "Opcodes.h"
#include "ops.h"
#include "debugger.h"

#include "Emu48Dll.h"

#define MAXBREAKPOINTS  256					// max. number of breakpoints

typedef struct								// type of breakpoint table
{
	UINT	nType;							// breakpoint type
	DWORD	dwAddr;							// breakpoint address
} BP_T;

static DWORD *pdwLinArray;					// last instruction linear array

static DWORD dwRefRstkp;					// reference stack pointer content

static WORD  wBreakpointCount = 0;			// number of breakpoints
static BP_T  sBreakpoint[MAXBREAKPOINTS];	// breakpoint table

static BOOL  bDbgNotify = FALSE;			// debugger notify flag
static BOOL  bDbgRpl;						// Flag for ASM or RPL breakpoint

// callback function notify debugger breakpoint
static VOID (CALLBACK *pEmuDbgNotify)(INT nBreaktype) = NULL;

// callback function notify hardware stack changed
static BOOL (CALLBACK *pEmuStackNotify)(VOID) = NULL;



//################
//#
//#    Static functions
//#
//################

//
// convert nibble register to DWORD
//
static DWORD RegToDWORD(BYTE *pReg, WORD wNib)
{
	WORD  i;
	DWORD dwRegister = 0;

	for (i = 0;i < wNib;++i)
	{
		dwRegister <<= 4;
		dwRegister += pReg[wNib-i-1];
	}
	return dwRegister;
}

//
// convert DWORD to nibble register
//
static VOID DWORDToReg(BYTE *pReg, WORD wNib, DWORD dwReg)
{
	int i;

	for (i = 0;i < wNib;++i)
	{
		pReg[i] = (BYTE) (dwReg & 0xF);
		dwReg >>= 4;						// next nibble
	}
	return;
}


//################
//#
//#    Public internal functions
//#
//################

//
// handle upper 32 bit of cpu cycle counter
//
VOID UpdateDbgCycleCounter(VOID)
{
	return;									// not necessary here, cycle counter has 64 bit in DLL version
}

//
// check for code breakpoints
//
BOOL CheckBreakpoint(DWORD dwAddr, DWORD dwRange, UINT nType)
{
	INT  i;
	BOOL bBreak = FALSE;					// don't break
	DWORD dwRomAddr = -1;					// no valid ROM address

	// stack changed notify function defined
	if (nType == BP_EXEC && pEmuStackNotify != NULL)
	{
		// hardware stack changed
		if (dwRefRstkp != -1 && dwRefRstkp != Chipset.rstkp)
			bBreak = pEmuStackNotify();		// inform debugger

		dwRefRstkp = Chipset.rstkp;			// save current stack level
	}

	// absolute ROM adress breakpoints
	if (nType == BP_EXEC)					// only on code breakpoints
	{
		LPBYTE I = FASTPTR(dwAddr);			// get opcode stream

		// adress in ROM area
		if (I >= pbyRom && I < pbyRom + dwRomSize)
			dwRomAddr = I - pbyRom;			// linear ROM address
	}

	for (i = 0; i < wBreakpointCount; ++i)	// scan all breakpoints
	{
		// check for absolute ROM adress breakpoint
		if (   sBreakpoint[i].dwAddr >= dwRomAddr && sBreakpoint[i].dwAddr < dwRomAddr + dwRange
			&& (sBreakpoint[i].nType & BP_ROM) != 0)
			return TRUE;

		// check address range and type
		if (   sBreakpoint[i].dwAddr >= dwAddr && sBreakpoint[i].dwAddr < dwAddr + dwRange
			&& (sBreakpoint[i].nType & nType) != 0)
			return TRUE;
	}
	return bBreak;
}

//
// notify debugger that emulation stopped
//
VOID NotifyDebugger(INT nType)				// update registers
{
	nDbgState = DBG_STEPINTO;				// state "step into"
	dwDbgStopPC = -1;						// disable "cursor stop address"

	_ASSERT(pEmuDbgNotify);					// notify function defined from caller
	pEmuDbgNotify(nType);					// notify caller
	bDbgNotify = TRUE;						// emulation stopped
	return;
}

//
// disable debugger
//
VOID DisableDebugger(VOID)
{
	nDbgState = DBG_OFF;					// disable debugger
	bInterrupt = TRUE;						// exit opcode loop
	SetEvent(hEventDebug);
	return;
}


//################
//#
//#    Dummy functions for linking
//#
//################

//
// entry from message loop
//
LRESULT OnToolDebug(VOID)
{
	return 0;
}

//
// load breakpoint list
//
VOID LoadBreakpointList(HANDLE hFile)
{
	return;
	UNREFERENCED_PARAMETER(hFile);
}

//
// save breakpoint list
//
VOID SaveBreakpointList(HANDLE hFile)
{
	return;
	UNREFERENCED_PARAMETER(hFile);
}

//
// create a copy of the breakpoint list
//
VOID CreateBackupBreakpointList(VOID)
{
	return;
}

//
// restore the breakpoint list from the copy
//
VOID RestoreBackupBreakpointList(VOID)
{
	return;
}


//################
//#
//#    Public external functions
//#
//################

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

DECLSPEC BOOL CALLBACK EmuInitLastInstr(
	WORD wNoInstr,							// @parm number of saved instructions,
											//       0 = frees the memory buffer
	DWORD *pdwArray)						// @parm pointer to linear array
{
	if (pdwInstrArray)						// circular buffer defined
	{
		EnterCriticalSection(&csDbgLock);
		{
			free(pdwInstrArray);			// free memory
			pdwInstrArray = NULL;
		}
		LeaveCriticalSection(&csDbgLock);
	}

	if (wNoInstr)							// new size
	{
		pdwLinArray = pdwArray;				// save address of linear array
		wInstrSize = wNoInstr + 1;			// size of circular buffer
		wInstrWp = wInstrRp = 0;			// init write/read pointer

		pdwInstrArray = malloc(wInstrSize*sizeof(*pdwInstrArray));
		if (pdwInstrArray == NULL)			// allocation error
			return TRUE;
	}
	return FALSE;
}

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

DECLSPEC BOOL CALLBACK EmuGetLastInstr(
	WORD *pwNoEntries)						// @parm return number of valid entries in array
{
	WORD i;

	if (pdwInstrArray == NULL) return TRUE;	// circular buffer not defined

	// copy data to linear buffer
	*pwNoEntries = 0;
	for (i = wInstrRp; i != wInstrWp; i = (i + 1) % wInstrSize)
		pdwLinArray[(*pwNoEntries)++] = pdwInstrArray[i];

	return FALSE;
}

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

DECLSPEC BOOL CALLBACK EmuRun(VOID)
{
	BOOL bErr = (nDbgState == DBG_RUN);
	if (nDbgState != DBG_RUN)				// emulation stopped
	{
		bDbgNotify = FALSE;					// reset debugger notify flag
		nDbgState = DBG_RUN;				// state "run"
		SetEvent(hEventDebug);				// run emulation
	}
	return bErr;
}

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

DECLSPEC VOID CALLBACK EmuRunPC(
	DWORD dwAddressPC)						// @parm breakpoint address
{
	if (nDbgState != DBG_RUN)				// emulation stopped
	{
		dwDbgStopPC = dwAddressPC;			// stop address
		EmuRun();							// run emulation
	}
	return;
}

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

DECLSPEC VOID CALLBACK EmuStep(VOID)
{
	if (nDbgState != DBG_RUN)				// emulation stopped
	{
		bDbgNotify = FALSE;					// reset debugger notify flag
		nDbgState = DBG_STEPINTO;			// state "step into"
		SetEvent(hEventDebug);				// run emulation
	}
	return;
}

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

DECLSPEC VOID CALLBACK EmuStepOver(VOID)
{
	if (nDbgState != DBG_RUN)				// emulation stopped
	{
		LPBYTE I;

		bDbgNotify = FALSE;					// reset debugger notify flag

		I = FASTPTR(Chipset.pc);

		dwDbgRstkp = Chipset.rstkp;			// save stack level

		nDbgState = DBG_STEPINTO;			// state "step into"
		if (I[0] == 0x7)					// GOSUB  7aaa
		{
			nDbgState = DBG_STEPOVER;		// state "step over"
		}
		if (I[0] == 0x8)					// GOSUBL or GOSBVL
		{
			if (I[1] == 0xE)				// GOSUBL 8Eaaaa
			{
				nDbgState = DBG_STEPOVER;	// state "step over"
			}
			if (I[1] == 0xF)				// GOSBVL 8Eaaaaa
			{
				nDbgState = DBG_STEPOVER;	// state "step over"
			}
		}
		SetEvent(hEventDebug);				// run emulation
	}
	return;
}

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

DECLSPEC VOID CALLBACK EmuStepOut(VOID)
{
	if (nDbgState != DBG_RUN)				// emulation stopped
	{
		bDbgNotify = FALSE;					// reset debugger notify flag
		dwDbgRstkp = (Chipset.rstkp-1)&7;	// save stack data
		dwDbgRstk  = Chipset.rstk[dwDbgRstkp];
		nDbgState = DBG_STEPOUT;			// state "step out"
		SetEvent(hEventDebug);				// run emulation
	}
	return;
}

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

DECLSPEC BOOL CALLBACK EmuStop(VOID)
{
	if (pEmuDbgNotify && nDbgState != DBG_STEPINTO) // emulation running
	{
		bDbgNotify = FALSE;					// reset debugger notify flag
		nDbgState = DBG_STEPINTO;			// state "step into"
		if (Chipset.Shutdn)					// cpu thread stopped
			SetEvent(hEventShutdn);			// goto debug session
		while (!bDbgNotify) Sleep(0);		// wait until emulation stopped
	}
	return nDbgState != DBG_STEPINTO;
}

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

DECLSPEC VOID CALLBACK EmuCallBackDebugNotify(
    VOID (CALLBACK *EmuDbgNotify)(INT nBreaktype)) // @parm CallBack function notify Debug breakpoint
{
	if(EmuDbgNotify != NULL)				// debugger enabled
	{
		dwDbgStopPC = -1;					// no stop address for goto cursor
		dwDbgRplPC = -1;					// no stop address for RPL breakpoint
		pEmuDbgNotify = EmuDbgNotify;		// set new handler
		nDbgState = DBG_RUN;				// enable debugger
	}
	else
	{
		nDbgState = DBG_OFF;				// disable debugger
		pEmuDbgNotify = EmuDbgNotify;		// remove handler
		bInterrupt = TRUE;					// exit opcode loop
		SetEvent(hEventDebug);
	}
	return;
}

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

DECLSPEC VOID CALLBACK EmuCallBackStackNotify(
	BOOL (CALLBACK *EmuStackNotify)(VOID))	// @parm CallBack function notify stack changed
{
	dwRefRstkp = -1;						// reference stack pointer not initialized
	pEmuStackNotify = EmuStackNotify;		// set new handler
	return;
}

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

DECLSPEC DWORD CALLBACK EmuGetRegister(
	UINT uRegister)							// @parm index of register
{
	DWORD dwResult;

	switch(uRegister)
	{
	case EMU_REGISTER_PC:
		dwResult = Chipset.pc;
		break;
	case EMU_REGISTER_D0:
		dwResult = Chipset.d0;
		break;
	case EMU_REGISTER_D1:
		dwResult = Chipset.d1;
		break;
	case EMU_REGISTER_DUMMY:
	case EMU_REGISTER_R5L:
	case EMU_REGISTER_R5H:
	case EMU_REGISTER_R6L:
	case EMU_REGISTER_R6H:
	case EMU_REGISTER_R7L:
	case EMU_REGISTER_R7H:
		dwResult = 0;						// dummy return
		break;
	case EMU_REGISTER_AL:
		dwResult = RegToDWORD(&Chipset.A[0],8);
		break;
	case EMU_REGISTER_AH:
		dwResult = RegToDWORD(&Chipset.A[8],8);
		break;
	case EMU_REGISTER_BL:
		dwResult = RegToDWORD(&Chipset.B[0],8);
		break;
	case EMU_REGISTER_BH:
		dwResult = RegToDWORD(&Chipset.B[8],8);
		break;
	case EMU_REGISTER_CL:
		dwResult = RegToDWORD(&Chipset.C[0],8);
		break;
	case EMU_REGISTER_CH:
		dwResult = RegToDWORD(&Chipset.C[8],8);
		break;
	case EMU_REGISTER_DL:
		dwResult = RegToDWORD(&Chipset.D[0],8);
		break;
	case EMU_REGISTER_DH:
		dwResult = RegToDWORD(&Chipset.D[8],8);
		break;
	case EMU_REGISTER_R0L:
		dwResult = RegToDWORD(&Chipset.R0[0],8);
		break;
	case EMU_REGISTER_R0H:
		dwResult = RegToDWORD(&Chipset.R0[8],8);
		break;
	case EMU_REGISTER_R1L:
		dwResult = RegToDWORD(&Chipset.R1[0],8);
		break;
	case EMU_REGISTER_R1H:
		dwResult = RegToDWORD(&Chipset.R1[8],8);
		break;
	case EMU_REGISTER_R2L:
		dwResult = RegToDWORD(&Chipset.R2[0],8);
		break;
	case EMU_REGISTER_R2H:
		dwResult = RegToDWORD(&Chipset.R2[8],8);
		break;
	case EMU_REGISTER_R3L:
		dwResult = RegToDWORD(&Chipset.R3[0],8);
		break;
	case EMU_REGISTER_R3H:
		dwResult = RegToDWORD(&Chipset.R3[8],8);
		break;
	case EMU_REGISTER_R4L:
		dwResult = RegToDWORD(&Chipset.R4[0],8);
		break;
	case EMU_REGISTER_R4H:
		dwResult = RegToDWORD(&Chipset.R4[8],8);
		break;
	case EMU_REGISTER_FLAGS:
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
		dwResult = RegToDWORD(Chipset.ST,4);
		dwResult <<= 1;
		dwResult |= Chipset.Shutdn ? 1 : 0;
		dwResult <<= (3+1);
		dwResult |= Chipset.intk ? 1 : 0;
		dwResult <<= 1;
		dwResult |= Chipset.inte ? 1 : 0;
		dwResult <<= 1;
		dwResult |= Chipset.carry ? 1 : 0;
		dwResult <<= 1;
		dwResult |= Chipset.mode_dec ? 1 : 0;
		dwResult <<= 4;
		dwResult |= Chipset.HST;
		dwResult <<= 4;
		dwResult |= Chipset.P;
		break;
	case EMU_REGISTER_OUT:
		dwResult = Chipset.out;
		break;
	case EMU_REGISTER_IN:
		dwResult = Chipset.in;
		break;
	case EMU_REGISTER_VIEW1:
		dwResult = ((Chipset.Bank_FF >> 1) & 0x3F) >> 4;
		break;
	case EMU_REGISTER_VIEW2:
		dwResult = ((Chipset.Bank_FF >> 1) & 0x3F) & 0xF;
		break;
	case EMU_REGISTER_RSTKP:
		dwResult = Chipset.rstkp;
		break;
	case EMU_REGISTER_RSTK0:
		dwResult = Chipset.rstk[0];
		break;
	case EMU_REGISTER_RSTK1:
		dwResult = Chipset.rstk[1];
		break;
	case EMU_REGISTER_RSTK2:
		dwResult = Chipset.rstk[2];
		break;
	case EMU_REGISTER_RSTK3:
		dwResult = Chipset.rstk[3];
		break;
	case EMU_REGISTER_RSTK4:
		dwResult = Chipset.rstk[4];
		break;
	case EMU_REGISTER_RSTK5:
		dwResult = Chipset.rstk[5];
		break;
	case EMU_REGISTER_RSTK6:
		dwResult = Chipset.rstk[6];
		break;
	case EMU_REGISTER_RSTK7:
		dwResult = Chipset.rstk[7];
		break;
	case EMU_REGISTER_CLKL:
		dwResult = (DWORD) (Chipset.cycles & 0xFFFFFFFF);
		break;
	case EMU_REGISTER_CLKH:
		dwResult = (DWORD) (Chipset.cycles >> 32);
		break;
	case EMU_REGISTER_CRC:
		dwResult = Chipset.crc;
		break;
	default:
		dwResult = 0;						// default return
		_ASSERT(FALSE);						// illegal entry
	}
	return dwResult;
}

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

DECLSPEC VOID CALLBACK EmuSetRegister(
	UINT uRegister,							// @parm index of register
	DWORD dwValue)							// @parm new 32 bit value
{
	switch(uRegister)
	{
	case EMU_REGISTER_PC:
		Chipset.pc = dwValue;
		break;
	case EMU_REGISTER_D0:
		Chipset.d0 = dwValue;
		break;
	case EMU_REGISTER_D1:
		Chipset.d1 = dwValue;
		break;
	case EMU_REGISTER_DUMMY:
	case EMU_REGISTER_R5L:
	case EMU_REGISTER_R5H:
	case EMU_REGISTER_R6L:
	case EMU_REGISTER_R6H:
	case EMU_REGISTER_R7L:
	case EMU_REGISTER_R7H:
		break;								// dummy return
	case EMU_REGISTER_AL:
		DWORDToReg(&Chipset.A[0],8,dwValue);
		break;
	case EMU_REGISTER_AH:
		DWORDToReg(&Chipset.A[8],8,dwValue);
		break;
	case EMU_REGISTER_BL:
		DWORDToReg(&Chipset.B[0],8,dwValue);
		break;
	case EMU_REGISTER_BH:
		DWORDToReg(&Chipset.B[8],8,dwValue);
		break;
	case EMU_REGISTER_CL:
		DWORDToReg(&Chipset.C[0],8,dwValue);
		break;
	case EMU_REGISTER_CH:
		DWORDToReg(&Chipset.C[8],8,dwValue);
		break;
	case EMU_REGISTER_DL:
		DWORDToReg(&Chipset.D[0],8,dwValue);
		break;
	case EMU_REGISTER_DH:
		DWORDToReg(&Chipset.D[8],8,dwValue);
		break;
	case EMU_REGISTER_R0L:
		DWORDToReg(&Chipset.R0[0],8,dwValue);
		break;
	case EMU_REGISTER_R0H:
		DWORDToReg(&Chipset.R0[8],8,dwValue);
		break;
	case EMU_REGISTER_R1L:
		DWORDToReg(&Chipset.R1[0],8,dwValue);
		break;
	case EMU_REGISTER_R1H:
		DWORDToReg(&Chipset.R1[8],8,dwValue);
		break;
	case EMU_REGISTER_R2L:
		DWORDToReg(&Chipset.R2[0],8,dwValue);
		break;
	case EMU_REGISTER_R2H:
		DWORDToReg(&Chipset.R2[8],8,dwValue);
		break;
	case EMU_REGISTER_R3L:
		DWORDToReg(&Chipset.R3[0],8,dwValue);
		break;
	case EMU_REGISTER_R3H:
		DWORDToReg(&Chipset.R3[8],8,dwValue);
		break;
	case EMU_REGISTER_R4L:
		DWORDToReg(&Chipset.R4[0],8,dwValue);
		break;
	case EMU_REGISTER_R4H:
		DWORDToReg(&Chipset.R4[8],8,dwValue);
		break;
	case EMU_REGISTER_FLAGS:
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
		Chipset.P = (BYTE) (dwValue & 0xF);
		dwValue >>= 4;
		Chipset.HST = (BYTE) (dwValue & 0xF);
		dwValue >>= 4;
		Chipset.mode_dec = (dwValue & 0x1) ? TRUE : FALSE;
		dwValue >>= 1;
		Chipset.carry = (dwValue & 0x1) ? TRUE : FALSE;
		dwValue >>= 1;
		Chipset.inte = (dwValue & 0x1) ? TRUE : FALSE;
		dwValue >>= 1;
		Chipset.intk = (dwValue & 0x1) ? TRUE : FALSE;
		dwValue >>= (1+4);
		DWORDToReg(Chipset.ST,sizeof(Chipset.ST),dwValue);
		PCHANGED;
		break;
	case EMU_REGISTER_OUT:
		Chipset.out = (WORD) (dwValue & 0xFFFF);
		break;
	case EMU_REGISTER_IN:
		Chipset.in = (WORD) (dwValue & 0xFFFF);
		break;
	case EMU_REGISTER_VIEW1:
		Chipset.Bank_FF &= 0x1F;
		Chipset.Bank_FF |= (dwValue & 0x03) << (4+1);
		RomSwitch(Chipset.Bank_FF);
		break;
	case EMU_REGISTER_VIEW2:
		Chipset.Bank_FF &= 0x61;
		Chipset.Bank_FF |= (dwValue & 0x0F) << 1;
		RomSwitch(Chipset.Bank_FF);
		break;
	case EMU_REGISTER_RSTKP:
		Chipset.rstkp = dwValue;
		break;
	case EMU_REGISTER_RSTK0:
		Chipset.rstk[0] = dwValue;
		break;
	case EMU_REGISTER_RSTK1:
		Chipset.rstk[1] = dwValue;
		break;
	case EMU_REGISTER_RSTK2:
		Chipset.rstk[2] = dwValue;
		break;
	case EMU_REGISTER_RSTK3:
		Chipset.rstk[3] = dwValue;
		break;
	case EMU_REGISTER_RSTK4:
		Chipset.rstk[4] = dwValue;
		break;
	case EMU_REGISTER_RSTK5:
		Chipset.rstk[5] = dwValue;
		break;
	case EMU_REGISTER_RSTK6:
		Chipset.rstk[6] = dwValue;
		break;
	case EMU_REGISTER_RSTK7:
		Chipset.rstk[7] = dwValue;
		break;
	case EMU_REGISTER_CLKL:
	case EMU_REGISTER_CLKH:
		break;								// not allowed to change
	case EMU_REGISTER_CRC:
		Chipset.crc = (WORD) (dwValue & 0xFFFF);
		break;
	default:
		_ASSERT(FALSE);						// illegal entry
	}
	return;
}

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

DECLSPEC VOID CALLBACK EmuGetMem(
	DWORD dwMapAddr,						// @parm mapped address of Saturn CPU
	BYTE  *pbyValue)						// @parm readed nibble
{
	Npeek(pbyValue,dwMapAddr,1);
	return;
}

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

DECLSPEC VOID CALLBACK EmuSetMem(
	DWORD dwMapAddr,						// @parm mapped address of Saturn CPU
	BYTE  byValue)							// @parm nibble to write
{
	Nwrite(&byValue,dwMapAddr,1);
	return;
}

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

DECLSPEC LPBYTE CALLBACK EmuGetRom(
	DWORD *pdwRomSize)						// @parm return size of ROM in nibbles
{
	*pdwRomSize = dwRomSize;
	return pbyRom;
}

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

DECLSPEC BOOL CALLBACK EmuSetBreakpoint(
	DWORD dwAddress,						// @parm breakpoint address to set
	UINT  nBreakpointType)					// @parm breakpoint type to set
{
	INT   i;

	_ASSERT(   nBreakpointType == BP_EXEC	// illegal breakpoint type
			|| nBreakpointType == BP_READ
			|| nBreakpointType == BP_WRITE
			|| nBreakpointType == BP_RPL
			|| nBreakpointType == BP_ACCESS
			|| nBreakpointType == BP_ROM);

	for (i = 0; i < wBreakpointCount; ++i)	// search for breakpoint
	{
		// breakpoint already set
		if (   sBreakpoint[i].dwAddr == dwAddress
			&& sBreakpoint[i].nType  == nBreakpointType)
			return FALSE;
	}

	if (wBreakpointCount >= MAXBREAKPOINTS)	// breakpoint buffer full
		return TRUE;

	sBreakpoint[wBreakpointCount].dwAddr = dwAddress;
	sBreakpoint[wBreakpointCount].nType  = nBreakpointType;
	++wBreakpointCount;
	return FALSE;
}

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

DECLSPEC BOOL CALLBACK EmuClearBreakpoint(
	DWORD dwAddress,						// @parm breakpoint address to clear
	UINT  nBreakpointType)					// @parm breakpoint type to clear
{
	INT   i;

	_ASSERT(   nBreakpointType == BP_EXEC	// illegal breakpoint type
			|| nBreakpointType == BP_READ
			|| nBreakpointType == BP_WRITE
			|| nBreakpointType == BP_RPL
			|| nBreakpointType == BP_ACCESS
			|| nBreakpointType == BP_ROM);

	for (i = 0; i < wBreakpointCount; ++i)	// search for breakpoint
	{
		// breakpoint found
		if (   sBreakpoint[i].dwAddr == dwAddress
			&& sBreakpoint[i].nType  == nBreakpointType)
		{
			// move rest to top
			for (++i; i < wBreakpointCount; ++i)
				sBreakpoint[i-1] = sBreakpoint[i];

			--wBreakpointCount;
			return FALSE;					// breakpoint found
		}
	}
	return TRUE;							// breakpoint not found
}

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

DECLSPEC VOID CALLBACK EmuClearAllBreakpoints(VOID)
{
	wBreakpointCount = 0;
	return;
}

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

DECLSPEC VOID CALLBACK EmuEnableNop3Breakpoint(
	BOOL bEnable)							// @parm stop on NOP3 opcode
{
	bDbgNOP3 = bEnable;						// change stop on NOP3 breakpoint flag
	return;
}

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

DECLSPEC VOID CALLBACK EmuEnableDoCodeBreakpoint(
	BOOL bEnable)							// @parm stop on DOCODE entry
{
	bDbgCode = bEnable;						// change stop on DOCODE breakpoint flag
	return;
}

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

DECLSPEC VOID CALLBACK EmuEnableRplBreakpoint(
	BOOL bEnable)							// @parm stop on RPL exit
{
	bDbgRPL = bEnable;						// change stop on RPL exit flag
	return;
}

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

DECLSPEC VOID CALLBACK EmuEnableSkipInterruptCode(
	BOOL bEnable)							// @parm TRUE = skip code instructions
											//              inside interrupt service routine
{
	bDbgSkipInt = bEnable;					// change skip interrupt code flag
	return;
}
