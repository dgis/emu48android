/*
 *   DdeServ.c
 *
 *   This file is part of Emu48
 *
 *   Copyright (C) 1998 Christoph Gießelink
 *
 */
#include "pch.h"
#include "Emu48.h"
#include "io.h"

HDDEDATA CALLBACK DdeCallback(UINT iType,UINT iFmt,HCONV hConv,
							  HSZ hsz1,HSZ hsz2,HDDEDATA hData,
							  DWORD dwData1,DWORD dwData2)
{
	TCHAR    *psz,szBuffer[32];
	HDDEDATA hReturn;
	LPBYTE   lpData,lpHeader;
	DWORD    dwAddress,dwSize,dwLoop,dwIndex;
	UINT     nStkLvl;
	BOOL     bSuccess;

	// disable stack loading items on HP38G, HP39/40G
	BOOL bStackEnable = cCurrentRomType!='6' && cCurrentRomType!='A' && cCurrentRomType!='E' && cCurrentRomType!='P'; // CdB for HP: add P type

	switch (iType)
	{
		case XTYP_CONNECT:
			// get service name
			DdeQueryString(idDdeInst,hsz2,szBuffer,ARRAYSIZEOF(szBuffer),0);
			if (0 != lstrcmp(szBuffer,szAppName))
				return (HDDEDATA) FALSE;
			// get topic name
			DdeQueryString(idDdeInst,hsz1,szBuffer,ARRAYSIZEOF(szBuffer),0);
			return (HDDEDATA) (INT_PTR) (0 == lstrcmp(szBuffer,szTopic));

		case XTYP_POKE:
			// quit on models without stack or illegal data format or not in running state
			if (!bStackEnable || iFmt != uCF_HpObj || nState != SM_RUN)
				return (HDDEDATA) DDE_FNOTPROCESSED;

			// get item name
			DdeQueryString(idDdeInst,hsz2,szBuffer,ARRAYSIZEOF(szBuffer),0);
			nStkLvl = _tcstoul(szBuffer,&psz,10);
			if (*psz != 0 || nStkLvl < 1)	// invalid number format
				return (HDDEDATA) DDE_FNOTPROCESSED;

			SuspendDebugger();				// suspend debugger
			bDbgAutoStateCtrl = FALSE;		// disable automatic debugger state control

			if (!(Chipset.IORam[BITOFFSET]&DON)) // HP off
			{
				// turn on HP
				KeyboardEvent(TRUE,0,0x8000);
				Sleep(dwWakeupDelay);
				KeyboardEvent(FALSE,0,0x8000);
			}

			if (WaitForSleepState())		// wait for cpu SHUTDN then sleep state
			{
				hReturn = DDE_FNOTPROCESSED;
				goto cancel;
			}

			while (nState!=nNextState) Sleep(0);
			_ASSERT(nState==SM_SLEEP);

			bSuccess = FALSE;

			// get data and size
			lpData = DdeAccessData(hData,&dwSize);

			// has object length header
			if (lpData && dwSize >= sizeof(DWORD))
			{
				dwIndex = *(LPDWORD) lpData; // object length

				if (dwIndex <= dwSize - sizeof(DWORD))
				{
					// reserve unpacked object length memory
					LPBYTE pbyMem = (LPBYTE) malloc(dwIndex * 2);

					if (pbyMem != NULL)
					{
						// copy data and write to stack
						CopyMemory(pbyMem+dwIndex,lpData+sizeof(DWORD),dwIndex);
						bSuccess = (WriteStack(nStkLvl,pbyMem,dwIndex) == S_ERR_NO);
						free(pbyMem);		// free memory
					}
				}
			}

			DdeUnaccessData(hData);

			SwitchToState(SM_RUN);			// run state
			while (nState!=nNextState) Sleep(0);
			_ASSERT(nState==SM_RUN);

			if (bSuccess == FALSE)
			{
				hReturn = DDE_FNOTPROCESSED;
				goto cancel;
			}

			KeyboardEvent(TRUE,0,0x8000);
			Sleep(dwWakeupDelay);
			KeyboardEvent(FALSE,0,0x8000);
			// wait for sleep mode
			while (Chipset.Shutdn == FALSE) Sleep(0);
			hReturn = (HDDEDATA) DDE_FACK;

cancel:
			bDbgAutoStateCtrl = TRUE;		// enable automatic debugger state control
			ResumeDebugger();
			return hReturn;

		case XTYP_REQUEST:
			// quit on models without stack or illegal data format or not in running state
			if (!bStackEnable || iFmt != uCF_HpObj || nState != SM_RUN)
				return NULL;

			// get item name
			DdeQueryString(idDdeInst,hsz2,szBuffer,ARRAYSIZEOF(szBuffer),0);
			nStkLvl = _tcstoul(szBuffer,&psz,10);
			if (*psz != 0 || nStkLvl < 1)	// invalid number format
				return NULL;

			if (WaitForSleepState())		// wait for cpu SHUTDN then sleep state
				return NULL;

			while (nState!=nNextState) Sleep(0);
			_ASSERT(nState==SM_SLEEP);

			dwAddress = RPL_Pick(nStkLvl);	// pick address of stack level "item" object
			if (dwAddress == 0)
			{
				SwitchToState(SM_RUN);		// run state
				return NULL;
			}
			dwLoop = dwSize = (RPL_SkipOb(dwAddress) - dwAddress + 1) / 2;

			lpHeader = (Chipset.type == 'X' || Chipset.type == 'Q' || Chipset.type == '2') ? (LPBYTE) BINARYHEADER49 : (LPBYTE) BINARYHEADER48;

			// length of binary header
			dwIndex = (DWORD) strlen((LPCSTR) lpHeader);

			// size of objectsize + header + object
			dwSize += dwIndex + sizeof(DWORD);

			// reserve memory
			if ((lpData = (LPBYTE) malloc(dwSize)) == NULL)
			{
				SwitchToState(SM_RUN);		// run state
				return NULL;
			}

			// save data length
			*(DWORD *)lpData = dwLoop + dwIndex;
			
			// copy header
			memcpy(lpData + sizeof(DWORD),lpHeader,dwIndex);

			// copy data
			for (dwIndex += sizeof(DWORD);dwLoop--;++dwIndex,dwAddress += 2)
				lpData[dwIndex] = Read2(dwAddress);

			// write data
			hReturn = DdeCreateDataHandle(idDdeInst,lpData,dwSize,0,hsz2,iFmt,0);
			free(lpData);

			SwitchToState(SM_RUN);			// run state
			while (nState!=nNextState) Sleep(0);
			_ASSERT(nState==SM_RUN);

			return hReturn;
	}
	return NULL;
	UNREFERENCED_PARAMETER(hConv);
	UNREFERENCED_PARAMETER(dwData1);
	UNREFERENCED_PARAMETER(dwData2);
}
