/*
 *   udp.c
 *
 *   This file is part of Emu48
 *
 *   Copyright (C) 2011 Christoph Gießelink
 *
 */
#include "pch.h"
#include "Emu48.h"

TCHAR szUdpServer[1024] = _T("localhost");
WORD  wUdpPort = 5025;						// scpi-raw

static SOCKADDR_IN sServer = { AF_INET, 0, { 255, 255, 255, 255 } };

VOID ResetUdp(VOID)
{
	sServer.sin_addr.s_addr = INADDR_NONE;	// invalidate saved UDP address
	return;
}

BOOL SendByteUdp(BYTE byData)
{
	SOCKET  sClient;

	BOOL bErr = TRUE;

	// IP address not specified
	if (sServer.sin_addr.s_addr == INADDR_NONE)
	{
		LPSTR lpszIpAddr;

		#if defined _UNICODE
			DWORD dwLength = lstrlen(szUdpServer) + 1;

			if ((lpszIpAddr = (LPSTR) _alloca(dwLength)) == NULL)
				return TRUE;				// server ip address not found

			WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK,
								szUdpServer, dwLength,
								lpszIpAddr, dwLength, NULL, NULL);
		#else
			lpszIpAddr = szUdpServer;
		#endif

		// try to interpret string as IPv4 address
		sServer.sin_addr.s_addr = inet_addr(lpszIpAddr);

		// not a valid ip address -> try to get ip address from name server
		if (sServer.sin_addr.s_addr == INADDR_NONE)
		{
			PHOSTENT host = gethostbyname(lpszIpAddr);
			if (host == NULL)
			{
				return TRUE;				// server ip address not found
			}

			sServer.sin_addr.s_addr = ((PIN_ADDR) host->h_addr_list[0])->s_addr;
		}
	}

	// create UDP socket
	if ((sClient = socket(AF_INET, SOCK_DGRAM, 0)) != INVALID_SOCKET)
	{
		sServer.sin_port = htons(wUdpPort);

		// transmit data byte
		bErr = sendto(sClient, (LPCCH) &byData, sizeof(byData), 0, (LPSOCKADDR) &sServer, sizeof(sServer)) == SOCKET_ERROR;
		closesocket(sClient);
	}
	return bErr;
}
