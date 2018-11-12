/*
 *   SndEnum.c
 *
 *   This file is part of Emu48
 *
 *   Copyright (C) 2015 Christoph Gießelink
 *
 */
#include "pch.h"
#include "Emu48.h"
#include "snddef.h"

typedef HRESULT (WINAPI *LPFNDLLGETCLASSOBJECT)(REFCLSID,REFIID,LPVOID *);
static LPFNDLLGETCLASSOBJECT pfnDllGetClassObject = NULL;

//
// create a IKsPropertySet interface
//
static __inline HRESULT DirectSoundPrivateCreate(LPKSPROPERTYSET *ppKsPropertySet)
{
	LPCLASSFACTORY pClassFactory = NULL;
	HRESULT hr;

	// create a class factory object
	#if defined __cplusplus
		hr = pfnDllGetClassObject(CLSID_DirectSoundPrivate,IID_IClassFactory,(LPVOID *) &pClassFactory);
	#else
		hr = pfnDllGetClassObject(&CLSID_DirectSoundPrivate,&IID_IClassFactory,(LPVOID *) &pClassFactory);
	#endif

	// create the DirectSoundPrivate object and query for an IKsPropertySet interface
	if (SUCCEEDED(hr))
	{
		#if defined __cplusplus
			hr = pClassFactory->CreateInstance(NULL,IID_IKsPropertySet,(LPVOID *) ppKsPropertySet);
		#else
			hr = pClassFactory->lpVtbl->CreateInstance(pClassFactory,NULL,&IID_IKsPropertySet,(LPVOID *) ppKsPropertySet);
		#endif
	}

	if (pClassFactory)						// release the class factory object
	{
		#if defined __cplusplus
			pClassFactory->Release();
		#else
			pClassFactory->lpVtbl->Release(pClassFactory);
		#endif
	}

	if (FAILED(hr) && *ppKsPropertySet)		// handle failure
	{
		#if defined __cplusplus
			(*ppKsPropertySet)->Release();
		#else
			(*ppKsPropertySet)->lpVtbl->Release(*ppKsPropertySet);
		#endif
	}
	return hr;
}

//
// get the device information about a DirectSound GUID.
//
static BOOL GetInfoFromDSoundGUID(CONST GUID *lpGUID, UINT *puWaveDeviceID)
{
	LPKSPROPERTYSET pKsPropertySet = NULL;
	HRESULT         hr;
	BOOL            bSuccess = FALSE;

	hr = DirectSoundPrivateCreate(&pKsPropertySet);
	if (SUCCEEDED(hr))
	{
		ULONG ulBytesReturned = 0;

		DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA sDirectSoundDeviceDescription;

		ZeroMemory(&sDirectSoundDeviceDescription,sizeof(sDirectSoundDeviceDescription));
		sDirectSoundDeviceDescription.DeviceId = *lpGUID;

		// get the size of the direct sound device description
		#if defined __cplusplus
			hr = pKsPropertySet->Get(
					DSPROPSETID_DirectSoundDevice,
					DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION,
					NULL,
					0,
					&sDirectSoundDeviceDescription,
					sizeof(sDirectSoundDeviceDescription),
					&ulBytesReturned
				);
		#else
			hr = pKsPropertySet->lpVtbl->Get(pKsPropertySet,
					&DSPROPSETID_DirectSoundDevice,
					DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION,
					NULL,
					0,
					&sDirectSoundDeviceDescription,
					sizeof(sDirectSoundDeviceDescription),
					&ulBytesReturned
				);
		#endif

		if (SUCCEEDED(hr) && ulBytesReturned)
		{
			PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA psDirectSoundDeviceDescription = NULL;

			// fetch the direct sound device description
			psDirectSoundDeviceDescription = (PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA) malloc(ulBytesReturned);
			if (psDirectSoundDeviceDescription != NULL)
			{
				// init structure with data from length request
				*psDirectSoundDeviceDescription = sDirectSoundDeviceDescription;

				#if defined __cplusplus
					hr = pKsPropertySet->Get(
							DSPROPSETID_DirectSoundDevice,
							DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION,
							NULL,
							0,
							psDirectSoundDeviceDescription,
							ulBytesReturned,
							&ulBytesReturned
						);
				#else
					hr = pKsPropertySet->lpVtbl->Get(pKsPropertySet,
							&DSPROPSETID_DirectSoundDevice,
							DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION,
							NULL,
							0,
							psDirectSoundDeviceDescription,
							ulBytesReturned,
							&ulBytesReturned
						);
				#endif

				if ((bSuccess = SUCCEEDED(hr)))
				{
					// the requested device ID
					*puWaveDeviceID = psDirectSoundDeviceDescription->WaveDeviceId;
				}
				free(psDirectSoundDeviceDescription);
			}
		}

		#if defined __cplusplus
			pKsPropertySet->Release();
		#else
			pKsPropertySet->lpVtbl->Release(pKsPropertySet);
		#endif
	}
	return bSuccess;
}

//
// callback function for DirectSoundEnumerate()
//
static BOOL CALLBACK DSEnumProc(LPGUID lpGUID,LPCTSTR lpszDesc,LPCTSTR lpszDrvName,LPVOID lpContext)
{
	HWND hWnd = (HWND) lpContext;			// window handle of the combo box

	if (lpGUID != NULL)						// NULL only for "Primary Sound Driver"
	{
		UINT uDevID;

		if (GetInfoFromDSoundGUID(lpGUID,&uDevID))
		{
			WAVEOUTCAPS woc;

			// has device the necessary capabilities?
			if (   waveOutGetDevCaps(uDevID,&woc,sizeof(woc)) == MMSYSERR_NOERROR
				&& (woc.dwFormats & WAVE_FORMAT_4M08) != 0)
			{
				// copy product name and wave device ID to combo box
				LONG i = (LONG) SendMessage(hWnd,CB_ADDSTRING,0,(LPARAM) lpszDesc);
				SendMessage(hWnd,CB_SETITEMDATA,i,uDevID);
			}
		}
	}
	return TRUE;
	UNREFERENCED_PARAMETER(lpszDrvName);
}

// set listfield for sound device combo box
VOID SetSoundDeviceList(HWND hWnd,UINT uDeviceID)
{
	typedef BOOL    (CALLBACK *LPDSENUMCALLBACK)(LPGUID, LPCTSTR, LPCTSTR, LPVOID);
	typedef HRESULT (WINAPI *LPFN_SDE)(LPDSENUMCALLBACK lpDSEnumCallback,LPVOID lpContext);
	LPFN_SDE pfnDirectSoundEnumerate = NULL;

	UINT uSelectDevice,uDevID,uDevNo;

	HMODULE hDSound = LoadLibrary(_T("dsound.dll"));

	if (hDSound != NULL)					// direct sound dll found
	{
		#if defined _UNICODE
			pfnDirectSoundEnumerate = (LPFN_SDE) GetProcAddress(hDSound,"DirectSoundEnumerateW");
		#else
			pfnDirectSoundEnumerate = (LPFN_SDE) GetProcAddress(hDSound,"DirectSoundEnumerateA");
		#endif
		pfnDllGetClassObject = (LPFNDLLGETCLASSOBJECT) GetProcAddress(hDSound,"DllGetClassObject");
	}

	SendMessage(hWnd,CB_RESETCONTENT,0,0);

	// preset selector
	uSelectDevice = (UINT) SendMessage(hWnd,CB_ADDSTRING,0,(LPARAM) _T("Standard Audio"));
	SendMessage(hWnd,CB_SETITEMDATA,uSelectDevice,WAVE_MAPPER);

	// check for direct sound interface functions
	if (pfnDirectSoundEnumerate != NULL && pfnDllGetClassObject != NULL)
	{
		// copy product name and wave device ID to combo box
		if (SUCCEEDED(pfnDirectSoundEnumerate((LPDSENUMCALLBACK) DSEnumProc,hWnd)))
		{
			UINT i;

			uDevNo = (UINT) SendMessage(hWnd,CB_GETCOUNT,0,0);
			for (i = 0; i < uDevNo; ++i)
			{
				// translate device ID to combo box position
				uDevID = (UINT) SendMessage(hWnd,CB_GETITEMDATA,i,0);

				if (uDevID == uDeviceID) uSelectDevice = i;
			}
		}
	}
	else // direct sound not available, detect over wave capabilities
	{
		WAVEOUTCAPS woc;

		uDevNo = waveOutGetNumDevs();
		for (uDevID = 0; uDevID < uDevNo; ++uDevID)
		{
			if (   waveOutGetDevCaps(uDevID,&woc,sizeof(woc)) == MMSYSERR_NOERROR
				&& (woc.dwFormats & WAVE_FORMAT_4M08) != 0)
			{
				// copy product name and wave device ID to combo box
				LONG i = (LONG) SendMessage(hWnd,CB_ADDSTRING,0,(LPARAM) woc.szPname);
				SendMessage(hWnd,CB_SETITEMDATA,i,uDevID);

				if (uDevID == uDeviceID) uSelectDevice = i;
			}
		}
	}

	// activate last selected combo box item
	SendMessage(hWnd,CB_SETCURSEL,uSelectDevice,0L);

	if (hDSound != NULL)					// direct sound dll loaded
	{
		pfnDllGetClassObject = NULL;
		VERIFY(FreeLibrary(hDSound));
	}
	return;
}
