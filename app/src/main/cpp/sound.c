/*
 *   sound.c
 *
 *   This file is part of Emu48
 *
 *   Copyright (C) 2013 Christoph Gießelink
 *
 */
#include "pch.h"
#include "Emu48.h"

// #define DEBUG_SOUND						// switch for sound debug purpose
// #define SINE_APPROX						// sine signal approximation

#define SAMPLES_PER_SEC		44100			// sound sampling rate
#define MILLISEC_PER_BUFFER	20				// time period of each sound buffer

#define NO_OF_BUFFERS		3				// number of reserve buffers before playing

typedef struct _MSAMPLE
{
	LPBYTE pbyData;
	DWORD  dwBufferLength;
	DWORD  dwPosition;
	// buffer admin part
	DWORD  dwIndex;							// index to count no. of sample buffers
	struct _MSAMPLE* pNext;					// pointer to next sample buffer
} MSAMPLE, *PMSAMPLE;

DWORD dwWaveVol = 64;						// wave sound volume
DWORD dwWaveTime = MILLISEC_PER_BUFFER;		// time period (in ms) of each sound buffer

static HWAVEOUT hWaveDevice = NULL;			// handle to the waveform-audio output device
static HANDLE   hThreadWave = NULL;			// thread handle of sound message handler
static DWORD    dwThreadWaveId = 0;			// thread id of sound message handler
static UINT     uHeaders = 0;				// no. of sending wave headers
static PMSAMPLE psHead = NULL;				// head of sound samples
static PMSAMPLE psTail = NULL;				// tail of sound samples

static CRITICAL_SECTION csSoundLock;		// critical section for sound emulation
static DWORD dwSoundBufferLength;			// sound buffer length for the given time period

static VOID FlushSample(VOID);

//
// sound message handler thread
//
static DWORD WINAPI SoundWndProc(LPVOID pParam)
{
	MSG msg;

	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (msg.message == MM_WOM_DONE)
		{
			HWAVEOUT hwo = (HWAVEOUT) msg.wParam;
			PWAVEHDR pwh = (PWAVEHDR) msg.lParam;

			VERIFY(waveOutUnprepareHeader(hwo,pwh,sizeof(*pwh)) == MMSYSERR_NOERROR);
			free(pwh->lpData);				// free waveform data
			free(pwh);						// free wavefom header

			_ASSERT(uHeaders > 0);
			--uHeaders;						// finished header

			FlushSample();					// check for new sample

			if (uHeaders == 0)				// no wave headers in transmission
			{
				bSoundSlow = FALSE;			// no sound slow down
				bEnableSlow = TRUE;			// reenable CPU slow down possibility
			}
		}
	}
	return 0;
	UNREFERENCED_PARAMETER(pParam);
}

//
// create sound message handler thread
//
static BOOL CreateWaveThread(VOID)
{
	_ASSERT(hThreadWave == NULL);

	// create sound message handler thread
	hThreadWave = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)&SoundWndProc,NULL,0,&dwThreadWaveId);
	return hThreadWave != NULL;
}

//
// destroy sound message handler thread
//
static VOID DestroyWaveThread(VOID)
{
	if (hThreadWave != NULL)				// sound message handler thread running
	{
		// shut down thread
		while (!PostThreadMessage(dwThreadWaveId,WM_QUIT,0,0))
			Sleep(0);
		WaitForSingleObject(hThreadWave,INFINITE);
		CloseHandle(hThreadWave);
		hThreadWave = NULL;
	}
	return;
}

//
// add sample buffer to tail of sample job list
//
static __inline VOID AddSoundBuf(PMSAMPLE psData)
{
	_ASSERT(psData != NULL);				// there must be a sample
	psData->pNext = NULL;					// last sample in job list

	// add sample to list
	EnterCriticalSection(&csSoundLock);
	{
		if (psTail == NULL)					// root
		{
			psData->dwIndex = 0;			// this is the root index

			_ASSERT(psHead == NULL);
			psHead = psTail = psData;		// add sample at head
		}
		else								// add at tail
		{
			// use next index
			psData->dwIndex = psTail->dwIndex + 1;

			psTail->pNext = psData;			// add sample at tail
			psTail = psData;
		}
	}
	LeaveCriticalSection(&csSoundLock);
	return;
}

//
// remove sample buffer from head of sample job list
//
static __inline BOOL GetSoundBuf(PMSAMPLE *ppsData)
{
	BOOL bSucc;

	EnterCriticalSection(&csSoundLock);
	{
		if ((bSucc = (psHead != NULL)))		// data in head
		{
			*ppsData = psHead;				// get sample
			psHead = psHead->pNext;			// and remove it from head
			if (psHead == NULL)				// was last one in head
			{
				psTail = NULL;				// so tail is also the last one
			}
		}
	}
	LeaveCriticalSection(&csSoundLock);
	return bSucc;
}

//
// number of sample buffers in sample job list
//
static DWORD GetSoundBufSize(VOID)
{
	DWORD dwNoSamples;

	EnterCriticalSection(&csSoundLock);
	{
		// no. of samples in buffer
		dwNoSamples = (psTail == NULL)
					? 0
					: (psTail->dwIndex - psHead->dwIndex) + 1;
	}
	LeaveCriticalSection(&csSoundLock);
	return dwNoSamples;
}

//
// allocate new sample buffer and add the
// buffer to the tail of the sample job list
//
static __inline BOOL AllocSample(PMSAMPLE *ppsData)
{
	// alloc new sample buffer
	*ppsData = (PMSAMPLE) malloc(sizeof(**ppsData));

	if (*ppsData != NULL)
	{
		(*ppsData)->dwPosition = 0;			// begin of buffer
		(*ppsData)->dwBufferLength = dwSoundBufferLength;
		(*ppsData)->pbyData = (LPBYTE) malloc((*ppsData)->dwBufferLength);
		if ((*ppsData)->pbyData != NULL)
		{
			// buffers allocated
			_ASSERT(*ppsData != NULL && (*ppsData)->pbyData != NULL);

			AddSoundBuf(*ppsData);			// add sample buffer to list
		}
		else
		{
			free(*ppsData);					// data alloc failed, delete sample buffer
			*ppsData = NULL;
		}
	}
	return *ppsData != NULL;
}

//
// write samples to sample buffer
//
static BOOL AddSamples(DWORD dwSamples, BYTE byLevel)
{
	PMSAMPLE psData;
	DWORD    dwBufSamples;
	#if defined SINE_APPROX
		INT  w,s,ss,x,y;
	#endif

	BOOL bSucc = TRUE;

	if (dwSamples == 0) return TRUE;		// nothing to add

	#if defined SINE_APPROX
		// calculate constants
		w = (INT) (byLevel - 0x80);			// max. wave level
		s = (INT) dwSamples;				// interval length (pi)
		ss = s * s;							// interval length ^ 2
		x = 1;								// sample no.
	#endif

	EnterCriticalSection(&csSoundLock);
	{
		psData = psTail;					// get last sample buffer

		do
		{
			// number of free sound samples in current buffer
			dwBufSamples = (psData != NULL)
						 ? (psData->dwBufferLength - psData->dwPosition)
						 : 0;

			if (dwBufSamples == 0)			// sample buffer is full
			{
				// alloc new sample buffer
				VERIFY(bSucc = AllocSample(&psData));
				if (!bSucc) break;

				_ASSERT(   psData != NULL
						&& psData->pbyData != NULL
						&& psData->dwPosition == 0);
				dwBufSamples = psData->dwBufferLength;
			}

			if (dwSamples < dwBufSamples)	// free sample buffer is larger then needed
				dwBufSamples = dwSamples;	// fill only the necessary no. of samples

			dwSamples -= dwBufSamples;		// remaining samples after buffer fill

			// fill buffer with level for beep
			#if defined SINE_APPROX
				for (; dwBufSamples > 0; --dwBufSamples)
				{
					// sine approximation function
					y = w - w * (4 * x * (x - s) + ss ) / ss;
					++x;					// next sample

					psData->pbyData[psData->dwPosition++] = (BYTE) (y + 0x80);
				}
			#else
				FillMemory(&psData->pbyData[psData->dwPosition],dwBufSamples,byLevel);
				psData->dwPosition += dwBufSamples;
			#endif
		}
		while (dwSamples > 0);
	}
	LeaveCriticalSection(&csSoundLock);
	return bSucc;
}

//
// write sample buffer from head of sample job list
// to waveform-audio output device and delete the
// sample buffer control from head of sample job list
//
static VOID FlushSample(VOID)
{
	PMSAMPLE psData;

	_ASSERT(hWaveDevice != NULL);

	if (GetSoundBuf(&psData) == TRUE)		// fetch sample buffer
	{
		PWAVEHDR pwh;

		// allocate new wave header
		if ((pwh = (PWAVEHDR) malloc(sizeof(*pwh))) != NULL)
		{
			pwh->lpData = (LPSTR) psData->pbyData;
			pwh->dwBufferLength = psData->dwPosition;
			pwh->dwBytesRecorded = 0;
			pwh->dwUser = 0;
			pwh->dwFlags = 0;
			pwh->dwLoops = 0;

			++uHeaders;						// add header

			// prepare sample
			VERIFY(waveOutPrepareHeader(hWaveDevice,pwh,sizeof(*pwh)) == MMSYSERR_NOERROR);

			// send sample
			VERIFY(waveOutWrite(hWaveDevice,pwh,sizeof(*pwh)) == MMSYSERR_NOERROR);
		}
		free(psData);						// delete sample buffer
	}
	return;
}

//
// 44.1 kHz, mono, 8-bit waveform-audio output device available
//
BOOL SoundAvailable(UINT uDeviceID)
{
	WAVEOUTCAPS woc;
	return waveOutGetDevCaps(uDeviceID,&woc,sizeof(woc)) == MMSYSERR_NOERROR
		   && (woc.dwFormats & WAVE_FORMAT_4M08) != 0;
}

//
// get the device ID of the current waveform-audio output device
//
BOOL SoundGetDeviceID(UINT *puDeviceID)
{
	BOOL bSucc = FALSE;

	if (hWaveDevice)						// have sound device
	{
		bSucc = (waveOutGetID(hWaveDevice,puDeviceID) == MMSYSERR_NOERROR);
	}
	return bSucc;
}

//
// open waveform-audio output device
//
BOOL SoundOpen(UINT uDeviceID)
{
	// check if sound device is already open
	if (hWaveDevice == NULL && SoundAvailable(uDeviceID))
	{
		WAVEFORMATEX wf;
		BOOL bSucc;

		wf.wFormatTag      = WAVE_FORMAT_PCM;
		wf.nChannels       = 1;
		wf.nSamplesPerSec  = SAMPLES_PER_SEC;
		wf.wBitsPerSample  = 8;
		wf.nBlockAlign     = wf.nChannels * wf.wBitsPerSample / 8;
		wf.nAvgBytesPerSec = wf.nBlockAlign * wf.nSamplesPerSec;
		wf.cbSize          = 0;

		InitializeCriticalSection(&csSoundLock);

		// sound buffer length for the given time period
		dwSoundBufferLength = SAMPLES_PER_SEC * dwWaveTime / 1000;

		if ((bSucc = CreateWaveThread()))	// create sound message handler
		{
			// create a sound device, use the CALLBACK_THREAD flag because with the
			// CALLBACK_FUNCTION flag unfortunately the called callback function
			// can only call a specific set of Windows functions. Attempting to
			// call other functions at the wrong time will result in a deadlock.
			bSucc = (waveOutOpen(&hWaveDevice,uDeviceID,&wf,dwThreadWaveId,0,CALLBACK_THREAD) == MMSYSERR_NOERROR);
		}
		if (!bSucc)
		{
			DestroyWaveThread();			// shut down message thread
			DeleteCriticalSection(&csSoundLock);
			hWaveDevice = NULL;
		}
	}
	return hWaveDevice != NULL;
}

//
// close waveform-audio output device
//
VOID SoundClose(VOID)
{
	if (hWaveDevice != NULL)
	{
		EnterCriticalSection(&csSoundLock);
		{
			while (psHead != NULL)			// cleanup remaining sample buffers
			{
				PMSAMPLE psNext = psHead->pNext;

				free(psHead->pbyData);
				free(psHead);
				psHead = psNext;
			}
			psTail = NULL;
		}
		LeaveCriticalSection(&csSoundLock);

		// abandon all pending wave headers
		VERIFY(waveOutReset(hWaveDevice) == MMSYSERR_NOERROR);

		DestroyWaveThread();				// shut down message thread

		VERIFY(waveOutClose(hWaveDevice) == MMSYSERR_NOERROR);
		DeleteCriticalSection(&csSoundLock);
		hWaveDevice = NULL;
	}
	uHeaders = 0;							// no wave headers in transmission
	bSoundSlow = FALSE;						// no sound slow down
	bEnableSlow = TRUE;						// reenable CPU slow down possibility
	return;
}

//
// calculate the wave level from the beeper bit state
//
static BYTE WaveLevel(WORD wOut)
{
	wOut >>= 11;							// mask out beeper bit OR[11]
	return (BYTE) (wOut & 0x01) + 1;		// return 1 or 2
}

//
// decode change of beeper OUT bits
//
VOID SoundOut(CHIPSET* w, WORD wOut)
{
	static DWORD dwLastCyc;					// last timer value at beeper bit change

	DWORD dwCycles,dwDiffSatCycles,dwDiffCycles,dwCpuFreq,dwSamples;
	BYTE  byWaveLevel;

	// sound device not opened or waveform-audio output device not available
	if (hWaveDevice == NULL)
		return;

	// actual timestamp
	dwCycles = (DWORD) (w->cycles & 0xFFFFFFFF);

	dwDiffSatCycles = dwCycles - dwLastCyc;	// time difference from syncpoint in original Saturn cycles

	// theoretical CPU frequency from given T2CYCLES
	dwCpuFreq = T2CYCLES * 16384;

	if (dwDiffSatCycles > dwCpuFreq / 2)	// frequency < 1 Hz
	{
		dwLastCyc = dwCycles;				// initial call for start beeping
		return;
	}

	// estimated CPU cycles for Clarke/Yorke chip
	dwDiffCycles = (cCurrentRomType == 'S')
				 ? (dwDiffSatCycles * 26) / 25	// Clarke * 1.04
				 : (dwDiffSatCycles * 11) / 10;	// Yorke  * 1.10

	// adjust original CPU cycles
	w->cycles += (dwDiffCycles - dwDiffSatCycles);
	dwLastCyc = (DWORD) (w->cycles & 0xFFFFFFFF); // new syncpoint

	// calculate no. of sound samples from CPU cycles, !! intermediate result maybe > 32bit !!
	dwSamples = (DWORD) ((2 * (QWORD) dwDiffCycles + 1) * SAMPLES_PER_SEC / 2 / dwCpuFreq);

	if (dwSamples == 0)						// frequency too high -> play nothing
		return;

	#if defined DEBUG_SOUND
	{
		TCHAR buffer[256];

		// calculate rounded time in us
		QWORD lDuration = 1000000 * (2 * (QWORD) dwDiffCycles + 1) / (2 * dwCpuFreq);

		wsprintf(buffer,_T("State %u: Time = %I64u us  f = %u Hz, Time = %I64u us  f = %u Hz\n"),
			wOut >> 11,lDuration,(DWORD) (1000000 / 2 / lDuration),
			(QWORD) dwSamples * 1000000 / SAMPLES_PER_SEC,SAMPLES_PER_SEC / 2 / dwSamples);
		OutputDebugString(buffer);
	}
	#endif

	// begin of beep
	if (uHeaders == 0 && GetSoundBufSize() == 0)
	{
		// use silence buffers to start output engine
		AddSamples(dwSoundBufferLength * NO_OF_BUFFERS,0x80);
	}

	// offset for wave level
	byWaveLevel = 0x80 + (BYTE) (dwWaveVol * (WaveLevel(wOut) - WaveLevel(w->out)) / 2);

	AddSamples(dwSamples,byWaveLevel);		// add samples to latest wave sample buffer

	if (GetSoundBufSize() > NO_OF_BUFFERS)	// have more than 3 wave sample buffers
	{
		FlushSample();						// send 2 of them
		FlushSample();
	}

	// ran out of buffers -> disable CPU slow down
	EnterCriticalSection(&csSlowLock);
	{
		InitAdjustSpeed();					// init variables if necessary
		bEnableSlow = (GetSoundBufSize() > 1);
	}
	LeaveCriticalSection(&csSlowLock);

	if (bSoundSlow == FALSE)
	{
		EnterCriticalSection(&csSlowLock);
		{
			InitAdjustSpeed();				// init variables if necessary
			bSoundSlow = TRUE;				// CPU slow down
		}
		LeaveCriticalSection(&csSlowLock);
	}
	return;
}

//
// beep with frequency (Hz) and duration (ms)
//
VOID SoundBeep(DWORD dwFrequency, DWORD dwDuration)
{
	QWORD lPeriods;
	DWORD dwSamples;
	BYTE  byLevel;

	// waveform-audio output device opened and have frequency
	if (hWaveDevice && dwFrequency > 0)
	{
		// samples for 1/2 of time period
		dwSamples = SAMPLES_PER_SEC / 2 / dwFrequency;

		// overall half periods
		lPeriods = (QWORD) dwFrequency * dwDuration / 500;

		while (lPeriods-- > 0)				// create sample buffers
		{
			// signal level
			byLevel = 0x80 + (BYTE) ((((DWORD) lPeriods & 1) * 2 - 1) * (dwWaveVol / 2));

			AddSamples(dwSamples,byLevel);	// add half period sample
		}

		while (GetSoundBufSize() > 0)		// samples in job list
			FlushSample();					// send sample buffer
	}
	Sleep(dwDuration);
	return;
}
