/*
 *   snddef.h
 *
 *   This file is part of Emu48
 *
 *   Copyright (C) 2015 Christoph Gieﬂelink
 *
 */

#include <initguid.h>

#if _MSC_VER >= 1600 // valid for VS2010 and later

#include <DSound.h>
#include <Dsconf.h>

#else // create the necessary definitions manually

//
// IKsPropertySet
//

#ifndef _IKsPropertySet_
#define _IKsPropertySet_

#ifdef __cplusplus
struct IKsPropertySet;
#endif // __cplusplus

typedef struct IKsPropertySet *LPKSPROPERTYSET;

DEFINE_GUID(IID_IKsPropertySet, 0x31efac30, 0x515c, 0x11d0, 0xa9, 0xaa, 0x00, 0xaa, 0x00, 0x61, 0xbe, 0x93);

#undef INTERFACE
#define INTERFACE IKsPropertySet

DECLARE_INTERFACE_(IKsPropertySet, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID, LPVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)    (THIS) PURE;
    STDMETHOD_(ULONG,Release)   (THIS) PURE;

    // IKsPropertySet methods
    STDMETHOD(Get)              (THIS_ REFGUID rguidPropSet, ULONG ulId, LPVOID pInstanceData, ULONG ulInstanceLength,
                                       LPVOID pPropertyData, ULONG ulDataLength, PULONG pulBytesReturned) PURE;
    STDMETHOD(Set)              (THIS_ REFGUID rguidPropSet, ULONG ulId, LPVOID pInstanceData, ULONG ulInstanceLength,
                                       LPVOID pPropertyData, ULONG ulDataLength) PURE;
    STDMETHOD(QuerySupport)     (THIS_ REFGUID rguidPropSet, ULONG ulId, PULONG pulTypeSupport) PURE;
};

#endif // _IKsPropertySet_

// DirectSound Configuration Component GUID {11AB3EC0-25EC-11d1-A4D8-00C04FC28ACA}
DEFINE_GUID(CLSID_DirectSoundPrivate, 0x11ab3ec0, 0x25ec, 0x11d1, 0xa4, 0xd8, 0x0, 0xc0, 0x4f, 0xc2, 0x8a, 0xca);

// DirectSound Device Properties {84624F82-25EC-11d1-A4D8-00C04FC28ACA}
DEFINE_GUID(DSPROPSETID_DirectSoundDevice, 0x84624f82, 0x25ec, 0x11d1, 0xa4, 0xd8, 0x0, 0xc0, 0x4f, 0xc2, 0x8a, 0xca);

typedef enum
{
    DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_A = 1,
    DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1 = 2,
    DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_1 = 3,
    DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_W = 4,
    DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A = 5,
    DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W = 6,
    DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_A = 7,
    DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W = 8,
} DSPROPERTY_DIRECTSOUNDDEVICE;

#ifdef UNICODE
#define DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W
#else // UNICODE
#define DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A
#endif // UNICODE

typedef enum
{
    DIRECTSOUNDDEVICE_TYPE_EMULATED,
    DIRECTSOUNDDEVICE_TYPE_VXD,
    DIRECTSOUNDDEVICE_TYPE_WDM
} DIRECTSOUNDDEVICE_TYPE;

typedef enum
{
    DIRECTSOUNDDEVICE_DATAFLOW_RENDER,
    DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE
} DIRECTSOUNDDEVICE_DATAFLOW;

typedef struct _DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA
{
    DIRECTSOUNDDEVICE_TYPE      Type;           // Device type
    DIRECTSOUNDDEVICE_DATAFLOW  DataFlow;       // Device dataflow
    GUID                        DeviceId;       // DirectSound device id
    LPTSTR                      Description;    // Device description
    LPTSTR                      Module;         // Device driver module
    LPTSTR                      Interface;      // Device interface
    ULONG                       WaveDeviceId;   // Wave device id
} DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA, *PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA;

#endif
