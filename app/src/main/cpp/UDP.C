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

static IN_ADDR ip_addr = { 255, 255, 255, 255 };

VOID ResetUdp(VOID)
{
	ip_addr.s_addr = INADDR_NONE;			// invalidate saved UDP address
	return;
}

BOOL SendByteUdp(BYTE byData)
{
	WSADATA     wsd;
	SOCKET      sClient;
	SOCKADDR_IN sServer;

	BOOL bErr = TRUE;

	VERIFY(WSAStartup(MAKEWORD(1,1),&wsd) == 0);

	if (ip_addr.s_addr == INADDR_NONE)		// IP address not specified
	{
		LPSTR lpszIpAddr;

		#if defined _UNICODE
			DWORD dwLength = lstrlen(szUdpServer) + 1;

			if ((lpszIpAddr = (LPSTR) _alloca(dwLength)) == NULL)
				return TRUE;				// server not found

			WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK,
								szUdpServer, dwLength,
								lpszIpAddr, dwLength, NULL, NULL);
		#else
			lpszIpAddr = szUdpServer;
		#endif

		ip_addr.s_addr = inet_addr(lpszIpAddr);

		// not a valid ip address -> try to get ip address from name server
		if (ip_addr.s_addr == INADDR_NONE)
		{
			PHOSTENT host = gethostbyname(lpszIpAddr);
			if (host == NULL)
			{
				return TRUE;				// server not found
			}

			ip_addr.s_addr = ((PIN_ADDR) host->h_addr_list[0])->s_addr;
		}
	}

	// create UDP socket
	if ((sClient = socket(AF_INET, SOCK_DGRAM, 0)) != INVALID_SOCKET)
	{
		sServer.sin_family = AF_INET;
		sServer.sin_port = htons(wUdpPort);
		sServer.sin_addr.s_addr = ip_addr.s_addr;

		// transmit data byte
		bErr = sendto(sClient, (LPCCH) &byData, sizeof(byData), 0, (LPSOCKADDR) &sServer, sizeof(sServer)) == SOCKET_ERROR;
		closesocket(sClient);
	}

	WSACleanup();							// cleanup network stack
	return bErr;
}
