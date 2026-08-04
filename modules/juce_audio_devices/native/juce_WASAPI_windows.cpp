/*
  ==============================================================================

   This file is part of the JUCE framework.
   Copyright (c) Raw Material Software Limited

   JUCE is an open source framework subject to commercial or open source
   licensing.

   By downloading, installing, or using the JUCE framework, or combining the
   JUCE framework with any other source code, object code, content or any other
   copyrightable work, you agree to the terms of the JUCE End User Licence
   Agreement, and all incorporated terms including the JUCE Privacy Policy and
   the JUCE Website Terms of Service, as applicable, which will bind you. If you
   do not agree to the terms of these agreements, we will not license the JUCE
   framework to you, and you must discontinue the installation or download
   process and cease use of the JUCE framework.

   JUCE End User Licence Agreement: https://juce.com/legal/juce-8-licence/
   JUCE Privacy Policy: https://juce.com/juce-privacy-policy
   JUCE Website Terms of Service: https://juce.com/juce-website-terms-of-service/

   Or:

   You may also use this code under the terms of the AGPLv3:
   https://www.gnu.org/licenses/agpl-3.0.en.html

   THE JUCE FRAMEWORK IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL
   WARRANTIES, WHETHER EXPRESSED OR IMPLIED, INCLUDING WARRANTY OF
   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wlanguage-extension-token")

#ifndef JUCE_WASAPI_LOGGING
 #define JUCE_WASAPI_LOGGING 0
#endif

//==============================================================================
namespace WasapiClasses
{

static void logFailure ([[maybe_unused]] HRESULT hr)
{
    jassert (hr != (HRESULT) 0x800401f0); // If you hit this, it means you're trying to call from
                                          // a thread which hasn't been initialised with CoInitialize().

   #if JUCE_WASAPI_LOGGING
    if (FAILED (hr))
    {
        const char* m = nullptr;

        switch (hr)
        {
            case E_POINTER:     m = "E_POINTER"; break;
            case E_INVALIDARG:  m = "E_INVALIDARG"; break;
            case E_NOINTERFACE: m = "E_NOINTERFACE"; break;

            #define JUCE_WASAPI_ERR(desc, n) \
                case MAKE_HRESULT(1, 0x889, n): m = #desc; break;

            JUCE_WASAPI_ERR (AUDCLNT_E_NOT_INITIALIZED, 0x001)
            JUCE_WASAPI_ERR (AUDCLNT_E_ALREADY_INITIALIZED, 0x002)
            JUCE_WASAPI_ERR (AUDCLNT_E_WRONG_ENDPOINT_TYPE, 0x003)
            JUCE_WASAPI_ERR (AUDCLNT_E_DEVICE_INVALIDATED, 0x004)
            JUCE_WASAPI_ERR (AUDCLNT_E_NOT_STOPPED, 0x005)
            JUCE_WASAPI_ERR (AUDCLNT_E_BUFFER_TOO_LARGE, 0x006)
            JUCE_WASAPI_ERR (AUDCLNT_E_OUT_OF_ORDER, 0x007)
            JUCE_WASAPI_ERR (AUDCLNT_E_UNSUPPORTED_FORMAT, 0x008)
            JUCE_WASAPI_ERR (AUDCLNT_E_INVALID_SIZE, 0x009)
            JUCE_WASAPI_ERR (AUDCLNT_E_DEVICE_IN_USE, 0x00a)
            JUCE_WASAPI_ERR (AUDCLNT_E_BUFFER_OPERATION_PENDING, 0x00b)
            JUCE_WASAPI_ERR (AUDCLNT_E_THREAD_NOT_REGISTERED, 0x00c)
            JUCE_WASAPI_ERR (AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED, 0x00e)
            JUCE_WASAPI_ERR (AUDCLNT_E_ENDPOINT_CREATE_FAILED, 0x00f)
            JUCE_WASAPI_ERR (AUDCLNT_E_SERVICE_NOT_RUNNING, 0x010)
            JUCE_WASAPI_ERR (AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED, 0x011)
            JUCE_WASAPI_ERR (AUDCLNT_E_EXCLUSIVE_MODE_ONLY, 0x012)
            JUCE_WASAPI_ERR (AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL, 0x013)
            JUCE_WASAPI_ERR (AUDCLNT_E_EVENTHANDLE_NOT_SET, 0x014)
            JUCE_WASAPI_ERR (AUDCLNT_E_INCORRECT_BUFFER_SIZE, 0x015)
            JUCE_WASAPI_ERR (AUDCLNT_E_BUFFER_SIZE_ERROR, 0x016)
            JUCE_WASAPI_ERR (AUDCLNT_E_CPUUSAGE_EXCEEDED, 0x017)
            JUCE_WASAPI_ERR (AUDCLNT_E_BUFFER_ERROR, 0x018)
            JUCE_WASAPI_ERR (AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED, 0x019)
            JUCE_WASAPI_ERR (AUDCLNT_E_INVALID_DEVICE_PERIOD, 0x020)
            JUCE_WASAPI_ERR (AUDCLNT_E_ENGINE_PERIODICITY_LOCKED, 0x028)
            JUCE_WASAPI_ERR (AUDCLNT_E_ENGINE_FORMAT_LOCKED, 0x029)
            default: break;
        }

        Logger::writeToLog ("WASAPI error: " + (m != nullptr ? String (m)
                                                             : String::toHexString ((int) hr)));
    }
   #endif
}

#undef check

static bool check (HRESULT hr)
{
    logFailure (hr);
    return SUCCEEDED (hr);
}

//==============================================================================
}

#ifndef KSDATAFORMAT_SUBTYPE_PCM
 #define KSDATAFORMAT_SUBTYPE_PCM         uuidFromString ("00000001-0000-0010-8000-00aa00389b71")
 #define KSDATAFORMAT_SUBTYPE_IEEE_FLOAT  uuidFromString ("00000003-0000-0010-8000-00aa00389b71")
#endif

enum EDataFlow
{
    eRender = 0,
    eCapture = (eRender + 1),
    eAll = (eCapture + 1)
};

enum
{
    DEVICE_STATE_ACTIVE = 1
};

enum
{
    AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY = 1,
    AUDCLNT_BUFFERFLAGS_SILENT = 2
};

JUCE_IUNKNOWNCLASS (IPropertyStore, "886d8eeb-8cf2-4446-8d02-cdba1dbdcf99")
{
    JUCE_COMCALL GetCount (DWORD*) = 0;
    JUCE_COMCALL GetAt (DWORD, PROPERTYKEY*) = 0;
    JUCE_COMCALL GetValue (const PROPERTYKEY&, PROPVARIANT*) = 0;
    JUCE_COMCALL SetValue (const PROPERTYKEY&, const PROPVARIANT&) = 0;
    JUCE_COMCALL Commit() = 0;
};

JUCE_IUNKNOWNCLASS (IMMDevice, "D666063F-1587-4E43-81F1-B948E807363F")
{
    JUCE_COMCALL Activate (REFIID, DWORD, PROPVARIANT*, void**) = 0;
    JUCE_COMCALL OpenPropertyStore (DWORD, IPropertyStore**) = 0;
    JUCE_COMCALL GetId (LPWSTR*) = 0;
    JUCE_COMCALL GetState (DWORD*) = 0;
};

JUCE_IUNKNOWNCLASS (IMMEndpoint, "1BE09788-6894-4089-8586-9A2A6C265AC5")
{
    JUCE_COMCALL GetDataFlow (EDataFlow*) = 0;
};

struct IMMDeviceCollection : public IUnknown
{
    JUCE_COMCALL GetCount (UINT*) = 0;
    JUCE_COMCALL Item (UINT, IMMDevice**) = 0;
};

enum ERole
{
    eConsole = 0,
    eMultimedia = (eConsole + 1),
    eCommunications = (eMultimedia + 1)
};

JUCE_IUNKNOWNCLASS (IMMNotificationClient, "7991EEC9-7E89-4D85-8390-6C703CEC60C0")
{
    JUCE_COMCALL OnDeviceStateChanged (LPCWSTR, DWORD) = 0;
    JUCE_COMCALL OnDeviceAdded (LPCWSTR) = 0;
    JUCE_COMCALL OnDeviceRemoved (LPCWSTR) = 0;
    JUCE_COMCALL OnDefaultDeviceChanged (EDataFlow, ERole, LPCWSTR) = 0;
    JUCE_COMCALL OnPropertyValueChanged (LPCWSTR, const PROPERTYKEY) = 0;
};

JUCE_IUNKNOWNCLASS (IMMDeviceEnumerator, "A95664D2-9614-4F35-A746-DE8DB63617E6")
{
    JUCE_COMCALL EnumAudioEndpoints (EDataFlow, DWORD, IMMDeviceCollection**) = 0;
    JUCE_COMCALL GetDefaultAudioEndpoint (EDataFlow, ERole, IMMDevice**) = 0;
    JUCE_COMCALL GetDevice (LPCWSTR, IMMDevice**) = 0;
    JUCE_COMCALL RegisterEndpointNotificationCallback (IMMNotificationClient*) = 0;
    JUCE_COMCALL UnregisterEndpointNotificationCallback (IMMNotificationClient*) = 0;
};

JUCE_COMCLASS (MMDeviceEnumerator, "BCDE0395-E52F-467C-8E3D-C4579291692E");

using REFERENCE_TIME = LONGLONG;

enum AVRT_PRIORITY
{
    AVRT_PRIORITY_LOW = -1,
    AVRT_PRIORITY_NORMAL,
    AVRT_PRIORITY_HIGH,
    AVRT_PRIORITY_CRITICAL
};

enum AUDCLNT_SHAREMODE
{
    AUDCLNT_SHAREMODE_SHARED,
    AUDCLNT_SHAREMODE_EXCLUSIVE
};

// Vochlea addition: stream options, Windows 8.1+. RAW opts the stream out of the endpoint's
// APO chain - on Win11 24H2+ that chain includes Voice Focus / Windows Studio Effects, which
// adaptively reshape exactly the transients and spectra an onset/MFCC detector measures.
enum AUDCLNT_STREAMOPTIONS
{
    AUDCLNT_STREAMOPTIONS_NONE         = 0x00,
    AUDCLNT_STREAMOPTIONS_RAW          = 0x01,
    AUDCLNT_STREAMOPTIONS_MATCH_FORMAT = 0x02
};

enum AUDIO_STREAM_CATEGORY
{
    AudioCategory_Other = 0,
    AudioCategory_ForegroundOnlyMedia,
    AudioCategory_BackgroundCapableMedia,
    AudioCategory_Communications,
    AudioCategory_Alerts,
    AudioCategory_SoundEffects,
    AudioCategory_GameEffects,
    AudioCategory_GameMedia,
    AudioCategory_GameChat,
    AudioCategory_Speech,
    AudioCategory_Movie,
    AudioCategory_Media
};

struct AudioClientProperties
{
    UINT32                  cbSize;
    BOOL                    bIsOffload;
    AUDIO_STREAM_CATEGORY   eCategory;
    AUDCLNT_STREAMOPTIONS   Options; // Vochlea addition: present in the Windows 8.1+ struct
};

JUCE_IUNKNOWNCLASS (IAudioClient, "1CB9AD4C-DBFA-4c32-B178-C2F568A703B2")
{
    JUCE_COMCALL Initialize (AUDCLNT_SHAREMODE, DWORD, REFERENCE_TIME, REFERENCE_TIME, const WAVEFORMATEX*, LPCGUID) = 0;
    JUCE_COMCALL GetBufferSize (UINT32*) = 0;
    JUCE_COMCALL GetStreamLatency (REFERENCE_TIME*) = 0;
    JUCE_COMCALL GetCurrentPadding (UINT32*) = 0;
    JUCE_COMCALL IsFormatSupported (AUDCLNT_SHAREMODE, const WAVEFORMATEX*, WAVEFORMATEX**) = 0;
    JUCE_COMCALL GetMixFormat (WAVEFORMATEX**) = 0;
    JUCE_COMCALL GetDevicePeriod (REFERENCE_TIME*, REFERENCE_TIME*) = 0;
    JUCE_COMCALL Start() = 0;
    JUCE_COMCALL Stop() = 0;
    JUCE_COMCALL Reset() = 0;
    JUCE_COMCALL SetEventHandle (HANDLE) = 0;
    JUCE_COMCALL GetService (REFIID, void**) = 0;
};

JUCE_COMCLASS (IAudioClient2, "726778CD-F60A-4eda-82DE-E47610CD78AA") : public IAudioClient
{
    JUCE_COMCALL IsOffloadCapable (AUDIO_STREAM_CATEGORY, BOOL*) = 0;
    JUCE_COMCALL SetClientProperties (const AudioClientProperties*) = 0;
    JUCE_COMCALL GetBufferSizeLimits (const WAVEFORMATEX*, BOOL, REFERENCE_TIME*, REFERENCE_TIME*) = 0;
};

// Vochlea fix: stock JUCE 8.0.6 declares IAudioClient3 with IAudioClient's IID (both here and
// in the __CRT_UUID_DECL below). QueryInterface for it therefore always succeeded, handed back
// a plain IAudioClient, and the IAudioClient3 methods were called through vtable slots the
// object may not have. Real IID per the Windows SDK's audioclient.h.
JUCE_COMCLASS (IAudioClient3, "7ED4EE07-8E67-4CD4-8C1A-2B7A5987AD42") : public IAudioClient2
{
    JUCE_COMCALL GetSharedModeEnginePeriod (const WAVEFORMATEX*, UINT32*, UINT32*, UINT32*, UINT32*) = 0;
    JUCE_COMCALL GetCurrentSharedModeEnginePeriod (WAVEFORMATEX**, UINT32*) = 0;
    JUCE_COMCALL InitializeSharedAudioStream (DWORD, UINT32, const WAVEFORMATEX*, LPCGUID) = 0;
};

JUCE_IUNKNOWNCLASS (IAudioCaptureClient, "C8ADBD64-E71E-48a0-A4DE-185C395CD317")
{
    JUCE_COMCALL GetBuffer (BYTE**, UINT32*, DWORD*, UINT64*, UINT64*) = 0;
    JUCE_COMCALL ReleaseBuffer (UINT32) = 0;
    JUCE_COMCALL GetNextPacketSize (UINT32*) = 0;
};

JUCE_IUNKNOWNCLASS (IAudioRenderClient, "F294ACFC-3146-4483-A7BF-ADDCA7C260E2")
{
    JUCE_COMCALL GetBuffer (UINT32, BYTE**) = 0;
    JUCE_COMCALL ReleaseBuffer (UINT32, DWORD) = 0;
};

JUCE_IUNKNOWNCLASS (IAudioEndpointVolume, "5CDF2C82-841E-4546-9722-0CF74078229A")
{
    JUCE_COMCALL RegisterControlChangeNotify (void*) = 0;
    JUCE_COMCALL UnregisterControlChangeNotify (void*) = 0;
    JUCE_COMCALL GetChannelCount (UINT*) = 0;
    JUCE_COMCALL SetMasterVolumeLevel (float, LPCGUID) = 0;
    JUCE_COMCALL SetMasterVolumeLevelScalar (float, LPCGUID) = 0;
    JUCE_COMCALL GetMasterVolumeLevel (float*) = 0;
    JUCE_COMCALL GetMasterVolumeLevelScalar (float*) = 0;
    JUCE_COMCALL SetChannelVolumeLevel (UINT, float, LPCGUID) = 0;
    JUCE_COMCALL SetChannelVolumeLevelScalar (UINT, float, LPCGUID) = 0;
    JUCE_COMCALL GetChannelVolumeLevel (UINT, float*) = 0;
    JUCE_COMCALL GetChannelVolumeLevelScalar (UINT, float*) = 0;
    JUCE_COMCALL SetMute (BOOL, LPCGUID) = 0;
    JUCE_COMCALL GetMute (BOOL*) = 0;
    JUCE_COMCALL GetVolumeStepInfo (UINT*, UINT*) = 0;
    JUCE_COMCALL VolumeStepUp (LPCGUID) = 0;
    JUCE_COMCALL VolumeStepDown (LPCGUID) = 0;
    JUCE_COMCALL QueryHardwareSupport (DWORD*) = 0;
    JUCE_COMCALL GetVolumeRange (float*, float*, float*) = 0;
};

enum AudioSessionDisconnectReason
{
    DisconnectReasonDeviceRemoval         = 0,
    DisconnectReasonServerShutdown        = 1,
    DisconnectReasonFormatChanged         = 2,
    DisconnectReasonSessionLogoff         = 3,
    DisconnectReasonSessionDisconnected   = 4,
    DisconnectReasonExclusiveModeOverride = 5
};

enum AudioSessionState
{
    AudioSessionStateInactive = 0,
    AudioSessionStateActive   = 1,
    AudioSessionStateExpired  = 2
};

JUCE_IUNKNOWNCLASS (IAudioSessionEvents, "24918ACC-64B3-37C1-8CA9-74A66E9957A8")
{
    JUCE_COMCALL OnDisplayNameChanged (LPCWSTR, LPCGUID) = 0;
    JUCE_COMCALL OnIconPathChanged (LPCWSTR, LPCGUID) = 0;
    JUCE_COMCALL OnSimpleVolumeChanged (float, BOOL, LPCGUID) = 0;
    JUCE_COMCALL OnChannelVolumeChanged (DWORD, float*, DWORD, LPCGUID) = 0;
    JUCE_COMCALL OnGroupingParamChanged (LPCGUID, LPCGUID) = 0;
    JUCE_COMCALL OnStateChanged (AudioSessionState) = 0;
    JUCE_COMCALL OnSessionDisconnected (AudioSessionDisconnectReason) = 0;
};

JUCE_IUNKNOWNCLASS (IAudioSessionControl, "F4B1A599-7266-4319-A8CA-E70ACB11E8CD")
{
    JUCE_COMCALL GetState (AudioSessionState*) = 0;
    JUCE_COMCALL GetDisplayName (LPWSTR*) = 0;
    JUCE_COMCALL SetDisplayName (LPCWSTR, LPCGUID) = 0;
    JUCE_COMCALL GetIconPath (LPWSTR*) = 0;
    JUCE_COMCALL SetIconPath (LPCWSTR, LPCGUID) = 0;
    JUCE_COMCALL GetGroupingParam (GUID*) = 0;
    JUCE_COMCALL SetGroupingParam (LPCGUID, LPCGUID) = 0;
    JUCE_COMCALL RegisterAudioSessionNotification (IAudioSessionEvents*) = 0;
    JUCE_COMCALL UnregisterAudioSessionNotification (IAudioSessionEvents*) = 0;
};

} // namespace juce

#ifdef __CRT_UUID_DECL
__CRT_UUID_DECL (juce::IPropertyStore,        0x886d8eeb, 0x8cf2, 0x4446, 0x8d, 0x02, 0xcd, 0xba, 0x1d, 0xbd, 0xcf, 0x99)
__CRT_UUID_DECL (juce::IMMDevice,             0xD666063F, 0x1587, 0x4E43, 0x81, 0xF1, 0xB9, 0x48, 0xE8, 0x07, 0x36, 0x3F)
__CRT_UUID_DECL (juce::IMMEndpoint,           0x1BE09788, 0x6894, 0x4089, 0x85, 0x86, 0x9A, 0x2A, 0x6C, 0x26, 0x5A, 0xC5)
__CRT_UUID_DECL (juce::IMMNotificationClient, 0x7991EEC9, 0x7E89, 0x4D85, 0x83, 0x90, 0x6C, 0x70, 0x3C, 0xEC, 0x60, 0xC0)
__CRT_UUID_DECL (juce::IMMDeviceEnumerator,   0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6)
__CRT_UUID_DECL (juce::MMDeviceEnumerator,    0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E)
__CRT_UUID_DECL (juce::IAudioClient,          0x1CB9AD4C, 0xDBFA, 0x4c32, 0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2)
__CRT_UUID_DECL (juce::IAudioClient2,         0x726778CD, 0xF60A, 0x4eda, 0x82, 0xDE, 0xE4, 0x76, 0x10, 0xCD, 0x78, 0xAA)
__CRT_UUID_DECL (juce::IAudioClient3,         0x7ED4EE07, 0x8E67, 0x4CD4, 0x8C, 0x1A, 0x2B, 0x7A, 0x59, 0x87, 0xAD, 0x42) // Vochlea fix, see above
__CRT_UUID_DECL (juce::IAudioCaptureClient,   0xC8ADBD64, 0xE71E, 0x48a0, 0xA4, 0xDE, 0x18, 0x5C, 0x39, 0x5C, 0xD3, 0x17)
__CRT_UUID_DECL (juce::IAudioRenderClient,    0xF294ACFC, 0x3146, 0x4483, 0xA7, 0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2)
__CRT_UUID_DECL (juce::IAudioEndpointVolume,  0x5CDF2C82, 0x841E, 0x4546, 0x97, 0x22, 0x0C, 0xF7, 0x40, 0x78, 0x22, 0x9A)
__CRT_UUID_DECL (juce::IAudioSessionEvents,   0x24918ACC, 0x64B3, 0x37C1, 0x8C, 0xA9, 0x74, 0xA6, 0x6E, 0x99, 0x57, 0xA8)
__CRT_UUID_DECL (juce::IAudioSessionControl,  0xF4B1A599, 0x7266, 0x4319, 0xA8, 0xCA, 0xE7, 0x0A, 0xCB, 0x11, 0xE8, 0xCD)
#endif

//==============================================================================
namespace juce
{
namespace WasapiClasses
{

static String getDeviceID (IMMDevice* device)
{
    String s;
    WCHAR* deviceId = nullptr;

    if (check (device->GetId (&deviceId)))
    {
        s = String (deviceId);
        CoTaskMemFree (deviceId);
    }

    return s;
}

static EDataFlow getDataFlow (const ComSmartPtr<IMMDevice>& device)
{
    EDataFlow flow = eRender;
    if (auto endpoint = device.getInterface<IMMEndpoint>())
        (void) check (endpoint->GetDataFlow (&flow));

    return flow;
}

static int refTimeToSamples (const REFERENCE_TIME& t, double sampleRate) noexcept
{
    return roundToInt (sampleRate * ((double) t) * 0.0000001);
}

static REFERENCE_TIME samplesToRefTime (int numSamples, double sampleRate) noexcept
{
    return (REFERENCE_TIME) ((numSamples * 10000.0 * 1000.0 / sampleRate) + 0.5);
}

static void copyWavFormat (WAVEFORMATEXTENSIBLE& dest, const WAVEFORMATEX* src) noexcept
{
    memcpy (&dest, src, src->wFormatTag == WAVE_FORMAT_EXTENSIBLE ? sizeof (WAVEFORMATEXTENSIBLE)
                                                                  : sizeof (WAVEFORMATEX));
}

static bool isExclusiveMode (WASAPIDeviceMode deviceMode) noexcept
{
    return deviceMode == WASAPIDeviceMode::exclusive;
}

static bool isLowLatencyMode (WASAPIDeviceMode deviceMode) noexcept
{
    return deviceMode == WASAPIDeviceMode::sharedLowLatency;
}

static bool supportsSampleRateConversion (WASAPIDeviceMode deviceMode) noexcept
{
    return deviceMode == WASAPIDeviceMode::shared;
}

//==============================================================================
/*  Vochlea addition: what WASAPI actually negotiated for one endpoint.

    Stock JUCE reports none of this, so a silent fallback - no IAudioClient3, an engine period
    already locked by another process, a driver that only offers 10ms periods - is
    indistinguishable from a device that opened exactly as asked. Captured per opened device and
    reachable from outside this file through the process-wide juce::getLastWASAPIProbeReport()
    snapshot at the end of it - deliberately the only route out, since neither this struct nor
    WASAPIAudioIODevice is declared in any public JUCE header.

    An unset optional means the call was never made, which is a different fact from the call
    having failed.
*/
struct WASAPIProbeInfo
{
    bool hasAudioClient3 = false;
    bool rawProcessingSupported = false;
    bool rawProcessingEnabled = false;
    bool usedLowLatencyClient = false;

    std::optional<HRESULT> sharedModeEnginePeriodResult;
    UINT32 defaultPeriodFrames = 0, fundamentalPeriodFrames = 0, minPeriodFrames = 0, maxPeriodFrames = 0;

    std::optional<HRESULT> initSharedStreamResult; // the final attempt's result
    int initSharedStreamAttempts = 0;              // > 1 means we hit a locked engine and retried
    std::optional<HRESULT> currentEnginePeriodResult;
    UINT32 currentEnginePeriodFrames = 0;
    double currentEngineSampleRate = 0;

    int negotiatedBufferSize = 0;
    double negotiatedSampleRate = 0;

    String toString() const
    {
        auto hr = [] (const std::optional<HRESULT>& r) { return r.has_value() ? String::toHexString ((int) *r) : String ("n/a"); };
        auto u  = [] (UINT32 v)                        { return String ((int) v); };

        return "audioClient3=" + String ((int) hasAudioClient3)
             + " rawSupported=" + String ((int) rawProcessingSupported)
             + " rawEnabled=" + String ((int) rawProcessingEnabled)
             + " lowLatencyClient=" + String ((int) usedLowLatencyClient)
             + " | GetSharedModeEnginePeriod hr=" + hr (sharedModeEnginePeriodResult)
             + " default=" + u (defaultPeriodFrames)
             + " fundamental=" + u (fundamentalPeriodFrames)
             + " min=" + u (minPeriodFrames)
             + " max=" + u (maxPeriodFrames)
             + " | InitializeSharedAudioStream hr=" + hr (initSharedStreamResult)
             + " attempts=" + String (initSharedStreamAttempts)
             + " | GetCurrentSharedModeEnginePeriod hr=" + hr (currentEnginePeriodResult)
             + " frames=" + u (currentEnginePeriodFrames)
             + " rate=" + String (currentEngineSampleRate, 0)
             + " | negotiated buffer=" + String (negotiatedBufferSize)
             + " rate=" + String (negotiatedSampleRate, 0);
    }
};

static CriticalSection& getLastProbeReportLock()
{
    static CriticalSection lock;
    return lock;
}

static String& getLastProbeReportRef()
{
    static String report;
    return report;
}

//==============================================================================
class WASAPIDeviceBase
{
public:
    WASAPIDeviceBase (const ComSmartPtr<IMMDevice>& d, WASAPIDeviceMode mode, bool isCapture)
        : device (d),
          deviceMode (mode),
          isCaptureEndpoint (isCapture)
    {
        clientEvent = CreateEvent (nullptr, false, false, nullptr);

        // Vochlea: an endpoint property, read once here - it gates setRawStreamOptions().
        probe.rawProcessingSupported = isRawProcessingSupported (device);

        // Vochlea: enumeration must NOT run on a raw client. RAW bypasses the capture APO chain,
        // which is precisely what changes the channel count and the sample rate on array mics
        // (4ch raw vs 1ch post-array; some raw streams are 16kHz). Measured on a raw client, the
        // mix format, buffer sizes, rates and channel count below would describe a stream the
        // device may never be opened with - and, worse than a failure to open, it would then open
        // cleanly at the wrong format with nothing downstream able to tell. Stream options are
        // applied only to the client openClient() actually opens.
        ComSmartPtr<IAudioClient> tempClient (createClient (false));

        if (tempClient == nullptr)
            return;

        auto format = getClientMixFormat (tempClient);

        if (! format)
            return;

        defaultNumChannels = maxNumChannels = format->Format.nChannels;
        defaultSampleRate = format->Format.nSamplesPerSec;
        rates.addUsingDefaultSort (defaultSampleRate);
        defaultFormatChannelMask = format->dwChannelMask;

        if (isExclusiveMode (deviceMode))
            if (auto optFormat = findSupportedFormat (tempClient, defaultNumChannels, defaultSampleRate))
                format = optFormat;

        querySupportedBufferSizes (*format, tempClient);
        querySupportedSampleRates (*format, tempClient);
        maxNumChannels = queryMaxNumChannels (tempClient);
    }

    virtual ~WASAPIDeviceBase()
    {
        device = nullptr;
        CloseHandle (clientEvent);
    }

    bool isOk() const noexcept   { return defaultBufferSize > 0 && defaultSampleRate > 0; }

    bool openClient (const double newSampleRate, const BigInteger& newChannels, const int bufferSizeSamples)
    {
        // Vochlea: WASAPIAudioIODevice::open() reuses this object across reopens, and every probe
        // field below is a per-open negotiation result - usedLowLatencyClient is only ever set
        // true, initSharedStreamAttempts only ever accumulates. Without this reset a device that
        // opened low-latency once reports lowLatencyClient=1 forever, including after a later
        // open fell back to the standard client, which is the one thing the probe exists to say.
        // rawProcessingSupported is an endpoint property read in the constructor, not a per-open
        // result, so it survives.
        {
            const auto rawSupported = probe.rawProcessingSupported;
            probe = WASAPIProbeInfo();
            probe.rawProcessingSupported = rawSupported;
        }

        // Vochlea: same reasoning, and these two matter more than the probe fields. They latch on
        // a transient refusal - a DAW holding the engine format for a moment - and are never
        // cleared, so without this one such moment disables the APO bypass this patch exists for
        // on every later reopen of this object, silently.
        matchFormatDisabled = false;
        rawStreamOptionsDisabled = false;

        sampleRate = newSampleRate;
        channels = newChannels;
        channels.setRange (maxNumChannels, channels.getHighestBit() + 1 - maxNumChannels, false);
        numChannels = channels.getHighestBit() + 1;

        if (numChannels == 0)
            return true;

        client = createClient();

        if (client != nullptr
             && tryInitialisingWithBufferSize (bufferSizeSamples))
        {
            sampleRateHasChanged = false;
            shouldShutdown = false;

            // Vochlea: these indices come from maxNumChannels, measured during enumeration, while
            // actualNumChannels comes from the format the open actually negotiated - in shared mode
            // findSupportedFormat() accepts the engine's "nearest" format, which can carry fewer
            // channels. So an index here can exceed the real frame, and convertSamples bounds-checks
            // nothing on the audio thread. Left UNCLAMPED deliberately: the entry means a SOURCE
            // channel to the capture consumer and a DESTINATION channel to the render one, so a
            // single clamp cannot be right for both - clamping the render side silently collapses
            // two sources onto one channel and leaves another unwritten. Each consumer bounds it.
            channelMaps.clear();

            for (int i = 0; i <= channels.getHighestBit(); ++i)
                if (channels[i])
                    channelMaps.add (i);

            REFERENCE_TIME latency;

            if (check (client->GetStreamLatency (&latency)))
                latencySamples = refTimeToSamples (latency, sampleRate);

            (void) check (client->GetBufferSize (&actualBufferSize));

            probe.negotiatedBufferSize = (int) actualBufferSize;
            probe.negotiatedSampleRate = sampleRate;
            queryCurrentEnginePeriod();

            createSessionEventCallback();
            return check (client->SetEventHandle (clientEvent));
        }

        return false;
    }

    void closeClient()
    {
        if (client != nullptr)
            client->Stop();

        // N.B. this is needed to prevent a double-deletion of the IAudioSessionEvents object
        // on older versions of Windows
        Thread::sleep (5);

        deleteSessionEventCallback();
        client = nullptr;
        ResetEvent (clientEvent);
    }

    void deviceSampleRateChanged()
    {
        sampleRateHasChanged = true;
    }

    void deviceSessionBecameInactive()
    {
        isActive = false;
    }

    void deviceSessionExpired()
    {
        shouldShutdown = true;
    }

    void deviceSessionBecameActive()
    {
        isActive = true;
    }

    std::optional<BigInteger> getDefaultLayout() const
    {
        if (countNumberOfBits ((uint64) defaultFormatChannelMask) == defaultNumChannels)
            return BigInteger ((int64) defaultFormatChannelMask);

        BigInteger integer;
        integer.setRange (0, defaultNumChannels, true);
        return integer;
    }

    //==============================================================================
    ComSmartPtr<IMMDevice> device;
    ComSmartPtr<IAudioClient> client;

    WASAPIDeviceMode deviceMode;
    const bool isCaptureEndpoint; // Vochlea addition: RAW is a capture-side decision only

    double sampleRate = 0, defaultSampleRate = 0;
    int numChannels = 0, actualNumChannels = 0, maxNumChannels = 0, defaultNumChannels = 0;
    int minBufferSize = 0, defaultBufferSize = 0, latencySamples = 0;
    int lowLatencyBufferSizeMultiple = 0, lowLatencyMaxBufferSize = 0;
    DWORD defaultFormatChannelMask = 0;
    Array<double> rates;
    HANDLE clientEvent = {};
    BigInteger channels;
    Array<int> channelMaps;
    UINT32 actualBufferSize = 0;
    int bytesPerSample = 0, bytesPerFrame = 0;
    std::atomic<bool> sampleRateHasChanged { false }, shouldShutdown { false }, isActive { true };
    WASAPIProbeInfo probe; // Vochlea addition

    virtual void updateFormat (bool isFloat) = 0;

private:
    //==============================================================================
    struct SessionEventCallback final : public ComBaseClassHelper<IAudioSessionEvents>
    {
        SessionEventCallback (WASAPIDeviceBase& d) : owner (d) {}

        JUCE_COMRESULT OnDisplayNameChanged (LPCWSTR, LPCGUID)                 override { return S_OK; }
        JUCE_COMRESULT OnIconPathChanged (LPCWSTR, LPCGUID)                    override { return S_OK; }
        JUCE_COMRESULT OnSimpleVolumeChanged (float, BOOL, LPCGUID)            override { return S_OK; }
        JUCE_COMRESULT OnChannelVolumeChanged (DWORD, float*, DWORD, LPCGUID)  override { return S_OK; }
        JUCE_COMRESULT OnGroupingParamChanged (LPCGUID, LPCGUID)               override { return S_OK; }

        JUCE_COMRESULT OnStateChanged (AudioSessionState state) override
        {
            switch (state)
            {
            case AudioSessionStateInactive:
                owner.deviceSessionBecameInactive();
                break;
            case AudioSessionStateExpired:
                owner.deviceSessionExpired();
                break;
            case AudioSessionStateActive:
                owner.deviceSessionBecameActive();
                break;
            }

            return S_OK;
        }

        JUCE_COMRESULT OnSessionDisconnected (AudioSessionDisconnectReason reason) override
        {
            if (reason == DisconnectReasonFormatChanged)
                owner.deviceSampleRateChanged();

            return S_OK;
        }

        WASAPIDeviceBase& owner;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SessionEventCallback)
    };

    ComSmartPtr<IAudioSessionControl> audioSessionControl;
    ComSmartPtr<SessionEventCallback> sessionEventCallback;

    // Vochlea: latch once this endpoint has proved it cannot open with the corresponding
    // stream option. MATCH_FORMAT is dropped first (it is the option that provokes
    // AUDCLNT_E_ENGINE_FORMAT_LOCKED); RAW is only ever dropped as a last resort.
    bool matchFormatDisabled = false, rawStreamOptionsDisabled = false;

    // Vochlea: whether the client currently held by `client` was configured with stream options,
    // maintained by setRawStreamOptions() alone. Deliberately NOT read back out of the probe:
    // probe.rawProcessingEnabled records the last SetClientProperties *result* and is left
    // untouched by that function's early returns, so an intermediate failure there would silently
    // disable the raw-drop retry below.
    bool rawStreamOptionsRequested = false;

    void createSessionEventCallback()
    {
        deleteSessionEventCallback();
        client->GetService (__uuidof (IAudioSessionControl),
                            (void**) audioSessionControl.resetAndGetPointerAddress());

        if (audioSessionControl != nullptr)
        {
            sessionEventCallback = becomeComSmartPtrOwner (new SessionEventCallback (*this));
            audioSessionControl->RegisterAudioSessionNotification (sessionEventCallback);
        }
    }

    void deleteSessionEventCallback()
    {
        if (audioSessionControl != nullptr && sessionEventCallback != nullptr)
            audioSessionControl->UnregisterAudioSessionNotification (sessionEventCallback);

        audioSessionControl = nullptr;
        sessionEventCallback = nullptr;
    }

    //==============================================================================
    ComSmartPtr<IAudioClient> createClient (bool applyStreamOptions = true)
    {
        ComSmartPtr<IAudioClient> newClient;

        if (device != nullptr)
            logFailure (device->Activate (__uuidof (IAudioClient), CLSCTX_INPROC_SERVER,
                                          nullptr, (void**) newClient.resetAndGetPointerAddress()));

        // Vochlea: applied here rather than at the call sites so that every client we intend to
        // OPEN - the one openClient() creates, and the ones recreated by the alignment dance, the
        // locked-engine retry and the raw-drop retry - carries the same stream options. The
        // constructor's enumeration client passes false; see the note there. If this fails the
        // stream simply opens processed, as it did before.
        if (applyStreamOptions && newClient != nullptr)
            setRawStreamOptions (newClient);

        return newClient;
    }

    /*  Vochlea: RawProcessingSupported is an endpoint property, not something IAudioClient
        exposes, so it has to be read off the IMMDevice's property store. Absent property or
        any failure => treated as unsupported, i.e. today's behaviour.
    */
    static bool isRawProcessingSupported (const ComSmartPtr<IMMDevice>& d)
    {
        if (d == nullptr)
            return false;

        ComSmartPtr<IPropertyStore> properties;

        if (! check (d->OpenPropertyStore (STGM_READ, properties.resetAndGetPointerAddress())))
            return false;

        // PKEY_Devices_AudioDevice_RawProcessingSupported
        const PROPERTYKEY rawProcessingSupportedKey
            = { { 0x8943b373, 0x388c, 0x4395, { 0xb5, 0x57, 0xbc, 0x6d, 0xba, 0xff, 0xaf, 0xdb } }, 2 };

        PROPVARIANT value;
        zerostruct (value);

        auto result = false;

        if (check (properties->GetValue (rawProcessingSupportedKey, &value)))
        {
            // The device-property type is DEVPROP_TYPE_BOOLEAN, which surfaces through
            // IPropertyStore as one of several variant types depending on the Windows build.
            switch (value.vt)
            {
                case VT_BOOL: result = value.boolVal != 0; break;
                case VT_UI1:  result = value.bVal    != 0; break;
                case VT_UI4:  result = value.ulVal   != 0; break;
                case VT_I4:   result = value.lVal    != 0; break;
                default:      break;
            }
        }

        PropVariantClear (&value);
        return result;
    }

    void setRawStreamOptions (ComSmartPtr<IAudioClient>& clientToUse)
    {
        // Always describes the client passed in, never an older one - the raw-drop retry below
        // reads it back and must not act on a stale value.
        rawStreamOptionsRequested = false;

        // Vochlea: RAW is a capture-side decision. Its entire motive is detector accuracy, which
        // is a property of the input signal; on a render endpoint it would instead bypass speaker
        // EQ, bass management, loudness and speaker protection - user-visible cost, no benefit.
        if (! isCaptureEndpoint
             || ! probe.rawProcessingSupported
             || isExclusiveMode (deviceMode)
             || rawStreamOptionsDisabled)
            return;

        auto audioClient2 = clientToUse.getInterface<IAudioClient2>();

        if (audioClient2 == nullptr)
            return;

        AudioClientProperties properties;
        zerostruct (properties);

        properties.cbSize     = (UINT32) sizeof (properties);
        properties.bIsOffload = FALSE;
        properties.eCategory  = AudioCategory_Other;

        // MATCH_FORMAT asks the engine to adopt the format we are about to request instead of
        // resampling us into its own. It is only requested on the low-latency path, because
        // that is the only path below that knows how to recover from AUDCLNT_E_ENGINE_FORMAT_LOCKED
        // when another process already owns the engine format.
        properties.Options = (isLowLatencyMode (deviceMode) && ! matchFormatDisabled)
                                 ? (AUDCLNT_STREAMOPTIONS) (AUDCLNT_STREAMOPTIONS_RAW | AUDCLNT_STREAMOPTIONS_MATCH_FORMAT)
                                 : AUDCLNT_STREAMOPTIONS_RAW;

        // Only true if the call actually took: Win8.0 rejects this cbSize and some drivers return
        // E_NOTIMPL. Claiming otherwise would send an unrelated open failure down the raw-drop
        // retry, wasting an Activate and latching RAW off on a machine that never applied it.
        probe.rawProcessingEnabled = check (audioClient2->SetClientProperties (&properties));
        rawStreamOptionsRequested = probe.rawProcessingEnabled;
    }

    void queryCurrentEnginePeriod()
    {
        if (client == nullptr || isExclusiveMode (deviceMode))
            return;

        auto audioClient3 = client.getInterface<IAudioClient3>();
        probe.hasAudioClient3 = (audioClient3 != nullptr);

        if (audioClient3 == nullptr)
            return;

        WAVEFORMATEX* currentFormat = nullptr;
        UINT32 currentPeriod = 0;

        auto hr = audioClient3->GetCurrentSharedModeEnginePeriod (&currentFormat, &currentPeriod);
        probe.currentEnginePeriodResult = hr;
        logFailure (hr);

        if (SUCCEEDED (hr))
        {
            probe.currentEnginePeriodFrames = currentPeriod;

            if (currentFormat != nullptr)
                probe.currentEngineSampleRate = (double) currentFormat->nSamplesPerSec;
        }

        CoTaskMemFree (currentFormat);
    }

    static std::optional<WAVEFORMATEXTENSIBLE> getClientMixFormat (ComSmartPtr<IAudioClient>& client)
    {
        WAVEFORMATEX* mixFormat = nullptr;

        if (! check (client->GetMixFormat (&mixFormat)))
            return {};

        WAVEFORMATEXTENSIBLE format{};

        copyWavFormat (format, mixFormat);
        CoTaskMemFree (mixFormat);

        return format;
    }

    //==============================================================================
    void querySupportedBufferSizes (WAVEFORMATEXTENSIBLE format, ComSmartPtr<IAudioClient>& audioClient)
    {
        // Vochlea: the low-latency query is now allowed to fail. Stock JUCE left every buffer
        // size at zero when the IAudioClient3 QI or the period query failed, which makes
        // isOk() false and the device unopenable - a driver without IAudioClient3 has to
        // degrade to the standard period, not disappear.
        if (isLowLatencyMode (deviceMode) && queryLowLatencyBufferSizes (format, audioClient))
            return;

        REFERENCE_TIME defaultPeriod, minPeriod;

        if (! check (audioClient->GetDevicePeriod (&defaultPeriod, &minPeriod)))
            return;

        minBufferSize = refTimeToSamples (minPeriod, defaultSampleRate);
        defaultBufferSize = refTimeToSamples (defaultPeriod, defaultSampleRate);
    }

    bool queryLowLatencyBufferSizes (WAVEFORMATEXTENSIBLE format, ComSmartPtr<IAudioClient>& audioClient)
    {
        auto audioClient3 = audioClient.getInterface<IAudioClient3>();
        probe.hasAudioClient3 = (audioClient3 != nullptr);

        if (audioClient3 == nullptr)
            return false;

        UINT32 defaultPeriod = 0, fundamentalPeriod = 0, minPeriod = 0, maxPeriod = 0;

        auto hr = audioClient3->GetSharedModeEnginePeriod ((WAVEFORMATEX*) &format,
                                                           &defaultPeriod,
                                                           &fundamentalPeriod,
                                                           &minPeriod,
                                                           &maxPeriod);
        probe.sharedModeEnginePeriodResult = hr;
        logFailure (hr);

        if (FAILED (hr))
            return false;

        probe.defaultPeriodFrames     = defaultPeriod;
        probe.fundamentalPeriodFrames = fundamentalPeriod;
        probe.minPeriodFrames         = minPeriod;
        probe.maxPeriodFrames         = maxPeriod;

        // A zero fundamental period would make the buffer-size enumeration in
        // WASAPIAudioIODevice::initialise() spin forever, and zero periods are not usable
        // sizes - treat any of them as "this driver cannot do low latency".
        if (defaultPeriod == 0 || minPeriod == 0 || fundamentalPeriod == 0 || maxPeriod == 0)
            return false;

        minBufferSize = (int) minPeriod;
        defaultBufferSize = (int) defaultPeriod;
        lowLatencyMaxBufferSize = (int) maxPeriod;
        lowLatencyBufferSizeMultiple = (int) fundamentalPeriod;

        return true;
    }

    void querySupportedSampleRates (WAVEFORMATEXTENSIBLE format, ComSmartPtr<IAudioClient>& audioClient)
    {
        for (auto rate : SampleRateHelpers::getAllSampleRates())
        {
            if (rates.contains (rate))
                continue;

            format.Format.nSamplesPerSec  = (DWORD) rate;
            format.Format.nAvgBytesPerSec = (DWORD) (format.Format.nSamplesPerSec * format.Format.nChannels * format.Format.wBitsPerSample / 8);

            WAVEFORMATEX* nearestFormat = nullptr;

            if (SUCCEEDED (audioClient->IsFormatSupported (isExclusiveMode (deviceMode) ? AUDCLNT_SHAREMODE_EXCLUSIVE
                                                                                        : AUDCLNT_SHAREMODE_SHARED,
                                                           (WAVEFORMATEX*) &format,
                                                           isExclusiveMode (deviceMode) ? nullptr
                                                                                        : &nearestFormat)))
            {
                if (nearestFormat != nullptr)
                    rate = (double) nearestFormat->nSamplesPerSec;

                if (! rates.contains (rate))
                    rates.addUsingDefaultSort (rate);
            }

            CoTaskMemFree (nearestFormat);
        }
    }

    struct AudioSampleFormat
    {
        bool useFloat;
        int  bitsPerSampleToTry;
        int  bytesPerSampleContainer;
    };

    static constexpr AudioSampleFormat formatsToTry[] =
    {
        { true,  32, 4 },
        { false, 32, 4 },
        { false, 24, 4 },
        { false, 24, 3 },
        { false, 20, 4 },
        { false, 20, 3 },
        { false, 16, 2 }
    };

    static std::optional<WAVEFORMATEXTENSIBLE> tryFormat (const AudioSampleFormat sampleFormat, IAudioClient* clientToUse,
                                                          WASAPIDeviceMode mode, int newNumChannels, double newSampleRate,
                                                          DWORD newMixFormatChannelMask)
    {
        WAVEFORMATEXTENSIBLE format;
        zerostruct (format);

        if (newNumChannels <= 2 && sampleFormat.bitsPerSampleToTry <= 16)
        {
            format.Format.wFormatTag = WAVE_FORMAT_PCM;
        }
        else
        {
            format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
            format.Format.cbSize = sizeof (WAVEFORMATEXTENSIBLE) - sizeof (WAVEFORMATEX);
        }

        format.Format.nSamplesPerSec       = (DWORD) newSampleRate;
        format.Format.nChannels            = (WORD) newNumChannels;
        format.Format.wBitsPerSample       = (WORD) (8 * sampleFormat.bytesPerSampleContainer);
        format.Samples.wValidBitsPerSample = (WORD) (sampleFormat.bitsPerSampleToTry);
        format.Format.nBlockAlign          = (WORD) (format.Format.nChannels * format.Format.wBitsPerSample / 8);
        format.Format.nAvgBytesPerSec      = (DWORD) (format.Format.nSamplesPerSec * format.Format.nBlockAlign);
        format.SubFormat                   = sampleFormat.useFloat ? KSDATAFORMAT_SUBTYPE_IEEE_FLOAT : KSDATAFORMAT_SUBTYPE_PCM;
        format.dwChannelMask               = newMixFormatChannelMask;

        WAVEFORMATEX* nearestFormat = nullptr;

        HRESULT hr = clientToUse->IsFormatSupported (isExclusiveMode (mode) ? AUDCLNT_SHAREMODE_EXCLUSIVE
                                                                            : AUDCLNT_SHAREMODE_SHARED,
                                                     (WAVEFORMATEX*) &format,
                                                     isExclusiveMode (mode) ? nullptr
                                                                            : &nearestFormat);
        logFailure (hr);

        auto supportsSRC = supportsSampleRateConversion (mode);

        if (hr == S_FALSE
            && nearestFormat != nullptr
            && (format.Format.nSamplesPerSec == nearestFormat->nSamplesPerSec
                || supportsSRC))
        {
            copyWavFormat (format, nearestFormat);

            if (supportsSRC)
            {
                format.Format.nSamplesPerSec  = (DWORD) newSampleRate;
                format.Format.nAvgBytesPerSec = (DWORD) (format.Format.nSamplesPerSec * format.Format.nBlockAlign);
            }

            hr = S_OK;
        }

        CoTaskMemFree (nearestFormat);

        if (hr != S_OK)
            return {};

        return format;
    }

    std::optional<WAVEFORMATEXTENSIBLE> findSupportedFormat (IAudioClient* clientToUse, int newNumChannels, double newSampleRate) const
    {
        for (auto ch = newNumChannels; ch <= maxNumChannels; ++ch)
        {
            auto maskWithLowestNBitsSet = static_cast<DWORD> ((1 << ch) - 1);
            auto mixFormatChannelMask = (ch == defaultNumChannels ? defaultFormatChannelMask : maskWithLowestNBitsSet);

            for (auto const& sampleFormat: formatsToTry)
                if (auto format = tryFormat (sampleFormat, clientToUse, deviceMode, ch, newSampleRate, mixFormatChannelMask))
                    return format;
        }

        return {};
    }

    int queryMaxNumChannels (IAudioClient* clientToUse) const
    {
        static constexpr auto maxNumChannelsToQuery = static_cast<int> (AudioChannelSet::maxChannelsOfNamedLayout);
        const auto fallbackNumChannels = defaultNumChannels;

        if (fallbackNumChannels >= maxNumChannelsToQuery)
            return fallbackNumChannels;

        auto result = fallbackNumChannels;

        for (auto ch = maxNumChannelsToQuery; ch > result; --ch)
        {
            auto channelMask = static_cast<DWORD> ((1 << ch) - 1);

            for (auto rate : rates)
                for (auto const& sampleFormat: formatsToTry)
                    if (auto format = tryFormat (sampleFormat, clientToUse, deviceMode, ch, rate, channelMask))
                        result = jmax (static_cast<int> (format->Format.nChannels), result);
        }

        return result;
    }

    DWORD getStreamFlags() const
    {
        DWORD streamFlags = 0x40000; /*AUDCLNT_STREAMFLAGS_EVENTCALLBACK*/

        if (supportsSampleRateConversion (deviceMode))
            streamFlags |= (0x80000000    /*AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM*/
                            | 0x8000000); /*AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY*/

        return streamFlags;
    }

    /*  Vochlea: the shared engine period and format are global per endpoint. If a DAW, a
        browser or Discord got there first, InitializeSharedAudioStream fails with
        AUDCLNT_E_ENGINE_PERIODICITY_LOCKED / _FORMAT_LOCKED and stock JUCE gives up, which
        (with no fallback at the call site) means the device does not open at all. Retry on
        the incumbent's terms instead, asking the engine what those terms actually are rather
        than guessing.

        Note what this deliberately does NOT do: adopt the engine's format. findSupportedFormat
        guarantees the format has at least numChannels channels and a container updateFormat
        can convert; an engine format satisfies neither, and channelMaps is built from the
        requested channels afterwards, so adopting one can index past the end of a frame. The
        recovery for a locked format is to stop asking for MATCH_FORMAT, not to change format.
    */
    bool initialiseLowLatencyClient (int bufferSizeSamples, WAVEFORMATEXTENSIBLE format)
    {
        auto audioClient3 = client.getInterface<IAudioClient3>();
        probe.hasAudioClient3 = (audioClient3 != nullptr);

        if (audioClient3 == nullptr)
            return false;

        auto periodFrames = (UINT32) bufferSizeSamples;

        // One initial attempt plus at most one period retry and one format retry.
        for (int attempt = 0; attempt < 3; ++attempt)
        {
            auto hr = audioClient3->InitializeSharedAudioStream (getStreamFlags(),
                                                                 periodFrames,
                                                                 (WAVEFORMATEX*) &format,
                                                                 nullptr);
            probe.initSharedStreamResult = hr;
            ++probe.initSharedStreamAttempts;
            logFailure (hr);

            if (SUCCEEDED (hr))
            {
                probe.usedLowLatencyClient = true;
                return true;
            }

            const auto periodicityLocked = (hr == MAKE_HRESULT (1, 0x889, 0x028)); // AUDCLNT_E_ENGINE_PERIODICITY_LOCKED
            const auto formatLocked      = (hr == MAKE_HRESULT (1, 0x889, 0x029)); // AUDCLNT_E_ENGINE_FORMAT_LOCKED

            if (! (periodicityLocked || formatLocked))
                return false;

            WAVEFORMATEX* currentFormat = nullptr;
            UINT32 currentPeriod = 0;

            auto periodHr = audioClient3->GetCurrentSharedModeEnginePeriod (&currentFormat, &currentPeriod);
            probe.currentEnginePeriodResult = periodHr;

            auto changed = false;

            if (check (periodHr))
            {
                probe.currentEnginePeriodFrames = currentPeriod;

                if (currentFormat != nullptr)
                    probe.currentEngineSampleRate = (double) currentFormat->nSamplesPerSec;

                // Only follow the engine's period when it is the period that was refused -
                // a locked format does not stop us asking for a small buffer.
                if (periodicityLocked && currentPeriod != 0 && currentPeriod != periodFrames)
                {
                    periodFrames = currentPeriod;
                    changed = true;
                }
            }

            CoTaskMemFree (currentFormat);

            // MATCH_FORMAT is what provokes FORMAT_LOCKED, so the retry has to drop it or it
            // just earns the same error again. If it was already off, the engine is holding a
            // format we cannot satisfy and the standard-client fallback is the answer.
            if (formatLocked && ! matchFormatDisabled)
            {
                matchFormatDisabled = true;
                changed = true;
            }

            if (! changed)
                return false;

            // A failed Initialize leaves the client unusable, exactly as in the alignment
            // dance below, so it has to be released and recreated before the retry - and
            // audioClient3 holds a reference of its own, so it has to go first.
            audioClient3 = nullptr;
            client = nullptr;
            client = createClient();

            if (client == nullptr)
                return false;

            audioClient3 = client.getInterface<IAudioClient3>();

            if (audioClient3 == nullptr)
                return false;
        }

        return false;
    }

    bool initialiseStandardClient (int bufferSizeSamples, WAVEFORMATEXTENSIBLE format)
    {
        REFERENCE_TIME defaultPeriod = 0, minPeriod = 0;

        check (client->GetDevicePeriod (&defaultPeriod, &minPeriod));

        if (isExclusiveMode (deviceMode) && bufferSizeSamples > 0)
            defaultPeriod = jmax (minPeriod, samplesToRefTime (bufferSizeSamples, format.Format.nSamplesPerSec));

        for (;;)
        {
            GUID session;
            auto hr = client->Initialize (isExclusiveMode (deviceMode) ? AUDCLNT_SHAREMODE_EXCLUSIVE
                                                                       : AUDCLNT_SHAREMODE_SHARED,
                                          getStreamFlags(),
                                          defaultPeriod,
                                          isExclusiveMode (deviceMode) ? defaultPeriod : 0,
                                          (WAVEFORMATEX*) &format,
                                          &session);

            if (check (hr))
                return true;

            // Handle the "alignment dance" : http://msdn.microsoft.com/en-us/library/windows/desktop/dd370875(v=vs.85).aspx (see Remarks)
            if (hr != MAKE_HRESULT (1, 0x889, 0x19)) // AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED
                break;

            UINT32 numFrames = 0;
            if (! check (client->GetBufferSize (&numFrames)))
                break;

            // Recreate client
            client = nullptr;
            client = createClient();

            defaultPeriod = samplesToRefTime ((int) numFrames, format.Format.nSamplesPerSec);
        }

        return false;
    }

    bool tryInitialisingWithBufferSize (int bufferSizeSamples)
    {
        if (initialiseClient (bufferSizeSamples))
            return true;

        // Vochlea: last resort. If we asked for a raw (APO-bypassing) stream and nothing would
        // open, drop that request and try once more exactly as stock JUCE would. Losing the APO
        // bypass costs detection accuracy; losing the device costs everything.
        if (rawStreamOptionsRequested && ! rawStreamOptionsDisabled)
        {
            rawStreamOptionsDisabled = true;
            matchFormatDisabled = true;
            probe.rawProcessingEnabled = false;

            client = nullptr;
            client = createClient();

            return client != nullptr && initialiseClient (bufferSizeSamples);
        }

        return false;
    }

    bool initialiseClient (int bufferSizeSamples)
    {
        if (auto format = findSupportedFormat (client, numChannels, sampleRate))
        {
            auto isInitialised = isLowLatencyMode (deviceMode) && initialiseLowLatencyClient (bufferSizeSamples, *format);

            // Vochlea: degrade to the standard shared client rather than failing to open.
            // This is what catches drivers with no IAudioClient3 at all, and engines whose
            // period or format we could not join above. A device that opens at the standard
            // 10ms period beats a device that does not open.
            if (! isInitialised)
            {
                if (isLowLatencyMode (deviceMode))
                {
                    // MATCH_FORMAT only ever makes sense paired with the FORMAT_LOCKED
                    // recovery above, which we have just exhausted - never carry it into a
                    // plain IAudioClient::Initialize, where a locked engine has no recovery.
                    matchFormatDisabled = true;

                    client = nullptr;
                    client = createClient();
                }

                isInitialised = client != nullptr && initialiseStandardClient (bufferSizeSamples, *format);
            }

            if (isInitialised)
            {
                actualNumChannels  = format->Format.nChannels;
                const bool isFloat = format->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE && format->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
                bytesPerSample     = format->Format.wBitsPerSample / 8;
                bytesPerFrame      = format->Format.nBlockAlign;

                updateFormat (isFloat);

                return true;
            }
        }

        return false;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WASAPIDeviceBase)
};

//==============================================================================
class WASAPIInputDevice final : public WASAPIDeviceBase
{
public:
    WASAPIInputDevice (const ComSmartPtr<IMMDevice>& d, WASAPIDeviceMode mode)
        : WASAPIDeviceBase (d, mode, true)
    {
    }

    ~WASAPIInputDevice() override
    {
        close();
    }

    bool open (double newSampleRate, const BigInteger& newChannels, int bufferSizeSamples)
    {
        return openClient (newSampleRate, newChannels, bufferSizeSamples)
                && (numChannels == 0 || check (client->GetService (__uuidof (IAudioCaptureClient),
                                                                   (void**) captureClient.resetAndGetPointerAddress())));
    }

    void close()
    {
        closeClient();
        captureClient = nullptr;
        reservoir.reset();
        queue = SingleThreadedAbstractFifo();
    }

    template <class SourceType>
    void updateFormatWithType (SourceType*) noexcept
    {
        using NativeType = AudioData::Pointer<AudioData::Float32, AudioData::NativeEndian, AudioData::NonInterleaved, AudioData::NonConst>;
        converter.reset (new AudioData::ConverterInstance<AudioData::Pointer<SourceType, AudioData::LittleEndian, AudioData::Interleaved, AudioData::Const>, NativeType> (actualNumChannels, 1));
    }

    void updateFormat (bool isFloat) override
    {
        if (isFloat)                    updateFormatWithType ((AudioData::Float32*) nullptr);
        else if (bytesPerSample == 4)   updateFormatWithType ((AudioData::Int32*)   nullptr);
        else if (bytesPerSample == 3)   updateFormatWithType ((AudioData::Int24*)   nullptr);
        else                            updateFormatWithType ((AudioData::Int16*)   nullptr);
    }

    bool start (int userBufferSizeIn)
    {
        const auto reservoirSize = nextPowerOfTwo ((int) (actualBufferSize + (UINT32) userBufferSizeIn));

        queue = SingleThreadedAbstractFifo (reservoirSize);
        reservoir.setSize ((size_t) (queue.getSize() * bytesPerFrame), true);
        xruns = 0;

        if (! check (client->Start()))
            return false;

        purgeInputBuffers();
        isActive = true;

        return true;
    }

    void purgeInputBuffers()
    {
        uint8* inputData;
        UINT32 numSamplesAvailable;
        DWORD flags;

        while (captureClient->GetBuffer (&inputData, &numSamplesAvailable, &flags, nullptr, nullptr) != MAKE_HRESULT (0, 0x889, 0x1) /* AUDCLNT_S_BUFFER_EMPTY */)
            captureClient->ReleaseBuffer (numSamplesAvailable);
    }

    int getNumSamplesInReservoir() const noexcept    { return queue.getNumReadable(); }

    void handleDeviceBuffer()
    {
        if (numChannels <= 0)
            return;

        uint8* inputData = nullptr;
        UINT32 numSamplesAvailable = 0;
        DWORD flags = 0;

        while (check (captureClient->GetBuffer (&inputData, &numSamplesAvailable, &flags, nullptr, nullptr)) && numSamplesAvailable > 0)
        {
            if ((flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY) != 0)
                xruns++;

            if (numSamplesAvailable > (UINT32) queue.getRemainingSpace())
            {
                captureClient->ReleaseBuffer (0);
                return;
            }

            auto offset = 0;

            for (const auto& block : queue.write ((int) numSamplesAvailable))
            {
                const auto samplesToDoBytes = block.getLength() * bytesPerFrame;

                auto* reservoirPtr = addBytesToPointer (reservoir.getData(), block.getStart() * bytesPerFrame);

                if ((flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0)
                    zeromem (reservoirPtr, (size_t) samplesToDoBytes);
                else
                    memcpy (reservoirPtr, inputData + offset * bytesPerFrame, (size_t) samplesToDoBytes);

                offset += block.getLength();
            }

            captureClient->ReleaseBuffer (numSamplesAvailable);
        }
    }

    void copyBuffersFromReservoir (float* const* destBuffers, const int numDestBuffers, const int bufferSize)
    {
        if ((numChannels <= 0 && bufferSize == 0) || reservoir.isEmpty())
            return;

        auto offset = jmax (0, bufferSize - queue.getNumReadable());

        if (offset > 0)
            for (int i = 0; i < numDestBuffers; ++i)
                zeromem (destBuffers[i], (size_t) offset * sizeof (float));

        for (const auto& block : queue.read (jmin (queue.getNumReadable(), bufferSize)))
        {
            // Vochlea: a source index past the negotiated frame would read off the end of the
            // reservoir. Clamping duplicates the last real channel, which is the benign outcome
            // here; a no-op whenever the format carries at least the requested channels.
            const auto highestSourceChannel = jmax (0, actualNumChannels - 1);

            for (auto i = 0; i < numDestBuffers; ++i)
                converter->convertSamples (destBuffers[i] + offset,
                                           0,
                                           addBytesToPointer (reservoir.getData(), block.getStart() * bytesPerFrame),
                                           jmin (channelMaps.getUnchecked (i), highestSourceChannel),
                                           block.getLength());

            offset += block.getLength();
        }
    }

    ComSmartPtr<IAudioCaptureClient> captureClient;
    MemoryBlock reservoir;
    SingleThreadedAbstractFifo queue;
    int xruns = 0;

    std::unique_ptr<AudioData::Converter> converter;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WASAPIInputDevice)
};

//==============================================================================
class WASAPIOutputDevice final : public WASAPIDeviceBase
{
public:
    WASAPIOutputDevice (const ComSmartPtr<IMMDevice>& d, WASAPIDeviceMode mode)
        : WASAPIDeviceBase (d, mode, false)
    {
    }

    ~WASAPIOutputDevice() override
    {
        close();
    }

    bool open (double newSampleRate, const BigInteger& newChannels, int bufferSizeSamples)
    {
        return openClient (newSampleRate, newChannels, bufferSizeSamples)
                && (numChannels == 0 || check (client->GetService (__uuidof (IAudioRenderClient),
                                                                   (void**) renderClient.resetAndGetPointerAddress())));
    }

    void close()
    {
        closeClient();
        renderClient = nullptr;
    }

    template <class DestType>
    void updateFormatWithType (DestType*)
    {
        using NativeType = AudioData::Pointer<AudioData::Float32, AudioData::NativeEndian, AudioData::NonInterleaved, AudioData::Const>;
        converter.reset (new AudioData::ConverterInstance<NativeType, AudioData::Pointer<DestType, AudioData::LittleEndian, AudioData::Interleaved, AudioData::NonConst>> (1, actualNumChannels));
    }

    void updateFormat (bool isFloat) override
    {
        if (isFloat)                    updateFormatWithType ((AudioData::Float32*) nullptr);
        else if (bytesPerSample == 4)   updateFormatWithType ((AudioData::Int32*)   nullptr);
        else if (bytesPerSample == 3)   updateFormatWithType ((AudioData::Int24*)   nullptr);
        else                            updateFormatWithType ((AudioData::Int16*)   nullptr);
    }

    bool start()
    {
        auto samplesToDo = getNumSamplesAvailableToCopy();
        uint8* outputData;

        if (check (renderClient->GetBuffer ((UINT32) samplesToDo, &outputData)))
            renderClient->ReleaseBuffer ((UINT32) samplesToDo, AUDCLNT_BUFFERFLAGS_SILENT);

        if (! check (client->Start()))
            return false;

        isActive = true;

        return true;
    }

    int getNumSamplesAvailableToCopy() const
    {
        if (numChannels <= 0)
            return 0;

        if (! isExclusiveMode (deviceMode))
        {
            UINT32 padding = 0;

            if (check (client->GetCurrentPadding (&padding)))
                return (int) actualBufferSize - (int) padding;
        }

        return (int) actualBufferSize;
    }

    void copyBuffers (const float* const* srcBuffers, int numSrcBuffers, int bufferSize,
                      WASAPIInputDevice* inputDevice, Thread& thread)
    {
        if (numChannels <= 0)
            return;

        int offset = 0;

        while (bufferSize > 0)
        {
            // This is needed in order not to drop any input data if the output device endpoint buffer was full
            if ((! isExclusiveMode (deviceMode)) && inputDevice != nullptr
                  && WaitForSingleObject (inputDevice->clientEvent, 0) == WAIT_OBJECT_0)
                inputDevice->handleDeviceBuffer();

            int samplesToDo = jmin (getNumSamplesAvailableToCopy(), bufferSize);

            if (samplesToDo == 0)
            {
                // This can ONLY occur in non-exclusive mode
                if (! thread.threadShouldExit() && WaitForSingleObject (clientEvent, 1000) == WAIT_OBJECT_0)
                    continue;

                break;
            }

            if (isExclusiveMode (deviceMode) && WaitForSingleObject (clientEvent, 1000) == WAIT_TIMEOUT)
                break;

            const auto numChannelsToCopy = jmin (actualNumChannels, numSrcBuffers);
            jassert (numChannelsToCopy <= channelMaps.size());

            uint8* outputData = nullptr;
            if (check (renderClient->GetBuffer ((UINT32) samplesToDo, &outputData)))
            {
                // Vochlea: GetBuffer does not zero, and any destination channel we skip below -
                // or never selected at all - would otherwise emit whatever was last in the slot.
                zeromem (outputData, (size_t) samplesToDo * (size_t) bytesPerFrame);

                for (int i = 0; i < numChannelsToCopy; ++i)
                {
                    // A destination past the negotiated frame is dropped, not folded onto another
                    // channel: writing it would corrupt the render buffer, and clamping it would
                    // overwrite a channel that has its own source.
                    const auto destChannel = channelMaps.getUnchecked (i);

                    if (destChannel < actualNumChannels)
                        converter->convertSamples (outputData, destChannel, srcBuffers[i] + offset, 0, samplesToDo);
                }

                renderClient->ReleaseBuffer ((UINT32) samplesToDo, 0);
            }

            bufferSize -= samplesToDo;
            offset += samplesToDo;
        }
    }

    ComSmartPtr<IAudioRenderClient> renderClient;
    std::unique_ptr<AudioData::Converter> converter;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WASAPIOutputDevice)
};

//==============================================================================
class WASAPIAudioIODevice final : public AudioIODevice,
                                  public Thread,
                                  private AsyncUpdater
{
public:
    WASAPIAudioIODevice (const String& deviceName,
                         const String& typeNameIn,
                         const String& outputDeviceID,
                         const String& inputDeviceID,
                         WASAPIDeviceMode mode)
        : AudioIODevice (deviceName, typeNameIn),
          Thread (SystemStats::getJUCEVersion() + ": WASAPI"),
          outputDeviceId (outputDeviceID),
          inputDeviceId (inputDeviceID),
          deviceMode (mode)
    {
    }

    ~WASAPIAudioIODevice() override
    {
        cancelPendingUpdate();
        close();
    }

    bool initialise()
    {
        latencyIn = latencyOut = 0;
        Array<double> ratesIn, ratesOut;

        if (createDevices())
        {
            jassert (inputDevice != nullptr || outputDevice != nullptr);

            sampleRates.clear();

            if (inputDevice != nullptr && outputDevice != nullptr)
            {
                defaultSampleRate = jmin (inputDevice->defaultSampleRate, outputDevice->defaultSampleRate);
                minBufferSize = jmax (inputDevice->minBufferSize, outputDevice->minBufferSize);
                defaultBufferSize = jmax (inputDevice->defaultBufferSize, outputDevice->defaultBufferSize);

                if (isLowLatencyMode (deviceMode))
                {
                    lowLatencyMaxBufferSize = jmin (inputDevice->lowLatencyMaxBufferSize, outputDevice->lowLatencyMaxBufferSize);
                    lowLatencyBufferSizeMultiple = jmax (inputDevice->lowLatencyBufferSizeMultiple, outputDevice->lowLatencyBufferSizeMultiple);
                }

                sampleRates.addArray (inputDevice->rates);

                if (supportsSampleRateConversion (deviceMode))
                {
                    for (auto r : outputDevice->rates)
                        if (! sampleRates.contains (r))
                            sampleRates.addUsingDefaultSort (r);
                }
                else
                {
                    sampleRates.removeValuesNotIn (outputDevice->rates);
                }
            }
            else
            {
                auto* d = inputDevice != nullptr ? static_cast<WASAPIDeviceBase*> (inputDevice.get())
                                                 : static_cast<WASAPIDeviceBase*> (outputDevice.get());

                defaultSampleRate = d->defaultSampleRate;
                minBufferSize = d->minBufferSize;
                defaultBufferSize = d->defaultBufferSize;

                if (isLowLatencyMode (deviceMode))
                {
                    lowLatencyMaxBufferSize = d->lowLatencyMaxBufferSize;
                    lowLatencyBufferSizeMultiple = d->lowLatencyBufferSizeMultiple;
                }

                sampleRates = d->rates;
            }

            bufferSizes.clear();
            bufferSizes.addUsingDefaultSort (defaultBufferSize);

            if (minBufferSize != defaultBufferSize)
                bufferSizes.addUsingDefaultSort (minBufferSize);

            if (isLowLatencyMode (deviceMode))
            {
                auto size = minBufferSize;

                while (size < lowLatencyMaxBufferSize)
                {
                    size += lowLatencyBufferSizeMultiple;

                    if (! bufferSizes.contains (size))
                        bufferSizes.addUsingDefaultSort (size);
                }
            }
            else
            {
                int n = 64;
                for (int i = 0; i < 40; ++i)
                {
                    if (n >= minBufferSize && n <= 2048 && ! bufferSizes.contains (n))
                        bufferSizes.addUsingDefaultSort (n);

                    n += (n < 512) ? 32 : (n < 1024 ? 64 : 128);
                }
            }

            return true;
        }

        return false;
    }

    StringArray getOutputChannelNames() override
    {
        StringArray outChannels;

        if (outputDevice != nullptr)
            for (int i = 1; i <= outputDevice->maxNumChannels; ++i)
                outChannels.add ("Output channel " + String (i));

        return outChannels;
    }

    StringArray getInputChannelNames() override
    {
        StringArray inChannels;

        if (inputDevice != nullptr)
            for (int i = 1; i <= inputDevice->maxNumChannels; ++i)
                inChannels.add ("Input channel " + String (i));

        return inChannels;
    }

    Array<double> getAvailableSampleRates() override        { return sampleRates; }
    Array<int> getAvailableBufferSizes() override           { return bufferSizes; }
    int getDefaultBufferSize() override                     { return defaultBufferSize; }

    int getCurrentBufferSizeSamples() override              { return currentBufferSizeSamples; }
    double getCurrentSampleRate() override                  { return currentSampleRate; }
    int getCurrentBitDepth() override                       { return 32; }
    int getOutputLatencyInSamples() override                { return latencyOut; }
    int getInputLatencyInSamples() override                 { return latencyIn; }
    BigInteger getActiveOutputChannels() const override     { return outputDevice != nullptr ? outputDevice->channels : BigInteger(); }
    BigInteger getActiveInputChannels() const override      { return inputDevice  != nullptr ? inputDevice->channels  : BigInteger(); }
    String getLastError() override                          { return lastError; }
    int getXRunCount() const noexcept override              { return inputDevice != nullptr ? inputDevice->xruns : -1; }

    std::optional<BigInteger> getDefaultOutputChannels() const override
    {
        return outputDevice != nullptr ? outputDevice->getDefaultLayout() : std::nullopt;
    }

    std::optional<BigInteger> getDefaultInputChannels() const override
    {
        return inputDevice != nullptr ? inputDevice->getDefaultLayout() : std::nullopt;
    }

    String open (const BigInteger& inputChannels, const BigInteger& outputChannels,
                 double sampleRate, int bufferSizeSamples) override
    {
        close();
        lastError.clear();

        if (sampleRates.size() == 0 && inputDevice != nullptr && outputDevice != nullptr)
        {
            lastError = TRANS ("The input and output devices don't share a common sample rate!");
            return lastError;
        }

        currentBufferSizeSamples  = bufferSizeSamples <= 0 ? defaultBufferSize : jmax (bufferSizeSamples, minBufferSize);
        currentSampleRate         = sampleRate > 0 ? sampleRate : defaultSampleRate;
        lastKnownInputChannels    = inputChannels;
        lastKnownOutputChannels   = outputChannels;

        if (inputDevice != nullptr && ! inputDevice->open (currentSampleRate, inputChannels, bufferSizeSamples))
        {
            lastError = TRANS ("Couldn't open the input device!");
            return lastError;
        }

        if (outputDevice != nullptr && ! outputDevice->open (currentSampleRate, outputChannels, bufferSizeSamples))
        {
            close();
            lastError = TRANS ("Couldn't open the output device!");
            return lastError;
        }

        if (isExclusiveMode (deviceMode))
        {
            // This is to make sure that the callback uses actualBufferSize in case of exclusive mode
            if (inputDevice != nullptr && outputDevice != nullptr && inputDevice->actualBufferSize != outputDevice->actualBufferSize)
            {
                close();
                lastError = TRANS ("Couldn't open the output device (buffer size mismatch)");
                return lastError;
            }

            currentBufferSizeSamples = (int) (outputDevice != nullptr ? outputDevice->actualBufferSize
                                                                      : inputDevice->actualBufferSize);
        }
        else if (isLowLatencyMode (deviceMode))
        {
            // Vochlea: both new fallbacks under initialiseLowLatencyClient() can come back with a
            // period we did not ask for - the incumbent engine's period on AUDCLNT_E_ENGINE_
            // PERIODICITY_LOCKED, or the standard client's default period - and in shared mode
            // nothing else corrects for it. Left uncorrected the callback consumes less than one
            // device period per event, so the input reservoir over-fills and the endpoint starts
            // dropping packets, and getCurrentBufferSizeSamples() plus the latencies derived from
            // it below both under-report - the exact figure §10 row 4 exists to measure.
            //
            // Only ever raises the block size, so it cannot fail an open that has already
            // succeeded, and unlike the exclusive branch above a mismatch between the two legs is
            // not an error here: shared mode tolerates a block larger than one period on either
            // leg (WASAPIOutputDevice::copyBuffers chunks; the input reservoir accumulates).
            // Deliberately scoped to low-latency mode - plain shared has the same latent
            // discrepancy but no path here changed its behaviour, so it is left as it was.
            const auto negotiated = jmax (inputDevice  != nullptr ? (int) inputDevice->actualBufferSize  : 0,
                                          outputDevice != nullptr ? (int) outputDevice->actualBufferSize : 0);

            if (negotiated > currentBufferSizeSamples)
                currentBufferSizeSamples = negotiated;
        }

        if (inputDevice != nullptr)   ResetEvent (inputDevice->clientEvent);
        if (outputDevice != nullptr)  ResetEvent (outputDevice->clientEvent);

        shouldShutdown = false;
        deviceSampleRateChanged = false;

        startThread (Priority::high);
        Thread::sleep (5);

        if (inputDevice != nullptr && inputDevice->client != nullptr)
        {
            latencyIn = (int) (inputDevice->latencySamples + currentBufferSizeSamples);

            if (! inputDevice->start (currentBufferSizeSamples))
            {
                close();
                lastError = TRANS ("Couldn't start the input device!");
                return lastError;
            }
        }

        if (outputDevice != nullptr && outputDevice->client != nullptr)
        {
            latencyOut = (int) (outputDevice->latencySamples + currentBufferSizeSamples);

            if (! outputDevice->start())
            {
                close();
                lastError = TRANS ("Couldn't start the output device!");
                return lastError;
            }
        }

        isOpen_ = true;
        logProbeInfo();
        return lastError;
    }

    /*  Vochlea addition: records what this open actually negotiated, once per device open, so
        nothing here is on the audio thread.

        Two sinks, and only the first is durable. The snapshot behind getLastWASAPIProbeReport()
        (end of this file) is always written and is what the application layer is expected to
        read. Logger::writeToLog() is best-effort: it is not behind JUCE_WASAPI_LOGGING, but with
        no juce::Logger installed it falls through to OutputDebugString, i.e. visible under a
        debugger or DebugView and nowhere else. Installing a Logger at startup would light it up.
    */
    void logProbeInfo() const
    {
        String report;
        report << "WASAPI probe: type=\"" << getTypeName() << "\" device=\"" << getName() << "\"";

        if (inputDevice != nullptr)
            report << newLine << "  input:  " << inputDevice->probe.toString();

        if (outputDevice != nullptr)
            report << newLine << "  output: " << outputDevice->probe.toString();

        {
            const ScopedLock sl (getLastProbeReportLock());
            getLastProbeReportRef() = report;
        }

        Logger::writeToLog (report);
    }

    void close() override
    {
        stop();
        signalThreadShouldExit();

        if (inputDevice != nullptr)   SetEvent (inputDevice->clientEvent);
        if (outputDevice != nullptr)  SetEvent (outputDevice->clientEvent);

        stopThread (5000);

        if (inputDevice != nullptr)   inputDevice->close();
        if (outputDevice != nullptr)  outputDevice->close();

        isOpen_ = false;
    }

    bool isOpen() override       { return isOpen_ && isThreadRunning(); }
    bool isPlaying() override    { return isStarted && isOpen_ && isThreadRunning(); }

    void start (AudioIODeviceCallback* call) override
    {
        if (isOpen_ && call != nullptr && ! isStarted)
        {
            if (! isThreadRunning())
            {
                // something's gone wrong and the thread's stopped..
                isOpen_ = false;
                return;
            }

            call->audioDeviceAboutToStart (this);

            const ScopedLock sl (startStopLock);
            callback = call;
            isStarted = true;
        }
    }

    void stop() override
    {
        if (isStarted)
        {
            auto* callbackLocal = callback;

            {
                const ScopedLock sl (startStopLock);
                isStarted = false;
            }

            if (callbackLocal != nullptr)
                callbackLocal->audioDeviceStopped();
        }
    }

    void setMMThreadPriority()
    {
        DynamicLibrary dll ("avrt.dll");
        JUCE_LOAD_WINAPI_FUNCTION (dll, AvSetMmThreadCharacteristicsW, avSetMmThreadCharacteristics, HANDLE, (LPCWSTR, LPDWORD))
        JUCE_LOAD_WINAPI_FUNCTION (dll, AvSetMmThreadPriority, avSetMmThreadPriority, HANDLE, (HANDLE, AVRT_PRIORITY))

        if (avSetMmThreadCharacteristics != nullptr && avSetMmThreadPriority != nullptr)
        {
            DWORD dummy = 0;

            if (auto h = avSetMmThreadCharacteristics (L"Pro Audio", &dummy))
                avSetMmThreadPriority (h, AVRT_PRIORITY_NORMAL);
        }
    }

    void run() override
    {
        setMMThreadPriority();

        auto bufferSize        = currentBufferSizeSamples;
        auto numInputBuffers   = getActiveInputChannels().countNumberOfSetBits();
        auto numOutputBuffers  = getActiveOutputChannels().countNumberOfSetBits();

        AudioBuffer<float> ins  (jmax (1, numInputBuffers),  bufferSize + 32);
        AudioBuffer<float> outs (jmax (1, numOutputBuffers), bufferSize + 32);
        auto inputBuffers  = ins.getArrayOfWritePointers();
        auto outputBuffers = outs.getArrayOfWritePointers();
        ins.clear();
        outs.clear();

        while (! threadShouldExit())
        {
            if ((outputDevice != nullptr && outputDevice->shouldShutdown)
                || (inputDevice != nullptr && inputDevice->shouldShutdown))
            {
                shouldShutdown = true;
                triggerAsyncUpdate();

                break;
            }

            auto inputDeviceActive = (inputDevice != nullptr && inputDevice->isActive);
            auto outputDeviceActive = (outputDevice != nullptr && outputDevice->isActive);

            if (! inputDeviceActive && ! outputDeviceActive)
                continue;

            if (inputDeviceActive)
            {
                if (outputDevice == nullptr)
                {
                    if (WaitForSingleObject (inputDevice->clientEvent, 1000) == WAIT_TIMEOUT)
                        break;

                    inputDevice->handleDeviceBuffer();

                    if (inputDevice->getNumSamplesInReservoir() < bufferSize)
                        continue;
                }
                else
                {
                    if (isExclusiveMode (deviceMode) && WaitForSingleObject (inputDevice->clientEvent, 0) == WAIT_OBJECT_0)
                        inputDevice->handleDeviceBuffer();
                }

                inputDevice->copyBuffersFromReservoir (inputBuffers, numInputBuffers, bufferSize);

                if (inputDevice->sampleRateHasChanged)
                {
                    deviceSampleRateChanged = true;
                    triggerAsyncUpdate();

                    break;
                }
            }

            {
                const ScopedTryLock sl (startStopLock);

                if (sl.isLocked() && isStarted)
                    callback->audioDeviceIOCallbackWithContext (inputBuffers,
                                                                numInputBuffers,
                                                                outputBuffers,
                                                                numOutputBuffers,
                                                                bufferSize,
                                                                {});
                else
                    outs.clear();
            }

            if (outputDeviceActive)
            {
                // Note that this function is handed the input device so it can check for the event and make sure
                // the input reservoir is filled up correctly even when bufferSize > device actualBufferSize
                outputDevice->copyBuffers (outputBuffers, numOutputBuffers, bufferSize, inputDevice.get(), *this);

                if (outputDevice->sampleRateHasChanged)
                {
                    deviceSampleRateChanged = true;
                    triggerAsyncUpdate();

                    break;
                }
            }
        }
    }

    //==============================================================================
    String outputDeviceId, inputDeviceId;
    String lastError;

private:
    // Device stats...
    std::unique_ptr<WASAPIInputDevice> inputDevice;
    std::unique_ptr<WASAPIOutputDevice> outputDevice;
    WASAPIDeviceMode deviceMode;
    double defaultSampleRate = 0;
    int minBufferSize = 0, defaultBufferSize = 0;
    int lowLatencyMaxBufferSize = 0, lowLatencyBufferSizeMultiple = 0;
    int latencyIn = 0, latencyOut = 0;
    Array<double> sampleRates;
    Array<int> bufferSizes;

    // Active state...
    bool isOpen_ = false, isStarted = false;
    int currentBufferSizeSamples = 0;
    double currentSampleRate = 0;

    AudioIODeviceCallback* callback = {};
    CriticalSection startStopLock;

    std::atomic<bool> shouldShutdown { false }, deviceSampleRateChanged { false };

    BigInteger lastKnownInputChannels, lastKnownOutputChannels;

    //==============================================================================
    bool createDevices()
    {
        ComSmartPtr<IMMDeviceEnumerator> enumerator;

        if (! check (enumerator.CoCreateInstance (__uuidof (MMDeviceEnumerator))))
            return false;

        ComSmartPtr<IMMDeviceCollection> deviceCollection;

        if (! check (enumerator->EnumAudioEndpoints (eAll, DEVICE_STATE_ACTIVE, deviceCollection.resetAndGetPointerAddress())))
            return false;

        UINT32 numDevices = 0;

        if (! check (deviceCollection->GetCount (&numDevices)))
            return false;

        for (UINT32 i = 0; i < numDevices; ++i)
        {
            ComSmartPtr<IMMDevice> device;

            if (! check (deviceCollection->Item (i, device.resetAndGetPointerAddress())))
                continue;

            auto deviceId = getDeviceID (device);

            if (deviceId.isEmpty())
                continue;

            auto flow = getDataFlow (device);

            if (deviceId == inputDeviceId && flow == eCapture)
                inputDevice.reset (new WASAPIInputDevice (device, deviceMode));
            else if (deviceId == outputDeviceId && flow == eRender)
                outputDevice.reset (new WASAPIOutputDevice (device, deviceMode));
        }

        return (outputDeviceId.isEmpty() || (outputDevice != nullptr && outputDevice->isOk()))
             && (inputDeviceId.isEmpty() || (inputDevice != nullptr && inputDevice->isOk()));
    }

    //==============================================================================
    void handleAsyncUpdate() override
    {
        auto closeDevices = [this]
        {
            close();

            outputDevice = nullptr;
            inputDevice = nullptr;
        };

        if (shouldShutdown)
        {
            closeDevices();
        }
        else if (deviceSampleRateChanged)
        {
            auto sampleRateChangedByInput = (inputDevice != nullptr && inputDevice->sampleRateHasChanged);

            closeDevices();
            initialise();

            auto changedSampleRate = [this, sampleRateChangedByInput]()
            {
                if (inputDevice != nullptr && sampleRateChangedByInput)
                    return inputDevice->defaultSampleRate;

                if (outputDevice != nullptr && ! sampleRateChangedByInput)
                    return outputDevice->defaultSampleRate;

                return 0.0;
            }();

            open (lastKnownInputChannels, lastKnownOutputChannels,
                  changedSampleRate, currentBufferSizeSamples);

            start (callback);
        }
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WASAPIAudioIODevice)
};


//==============================================================================
class WASAPIAudioIODeviceType final : public AudioIODeviceType
{
public:
    explicit WASAPIAudioIODeviceType (WASAPIDeviceMode mode)
        : AudioIODeviceType (getDeviceTypename (mode)),
          deviceMode (mode)
    {
    }

    ~WASAPIAudioIODeviceType() override
    {
        if (notifyClient != nullptr)
            enumerator->UnregisterEndpointNotificationCallback (notifyClient);
    }

    //==============================================================================
    void scanForDevices() override
    {
        hasScanned = true;
        devices = scan();
    }

    StringArray getDeviceNames (bool wantInputNames) const override
    {
        jassert (hasScanned); // need to call scanForDevices() before doing this

        return wantInputNames ? devices.inputDeviceNames
                              : devices.outputDeviceNames;
    }

    int getDefaultDeviceIndex (bool /*forInput*/) const override
    {
        jassert (hasScanned); // need to call scanForDevices() before doing this
        return 0;
    }

    int getIndexOfDevice (AudioIODevice* device, bool asInput) const override
    {
        jassert (hasScanned); // need to call scanForDevices() before doing this

        if (auto d = dynamic_cast<WASAPIAudioIODevice*> (device))
            return asInput ? devices.inputDeviceIds .indexOf (d->inputDeviceId)
                           : devices.outputDeviceIds.indexOf (d->outputDeviceId);

        return -1;
    }

    bool hasSeparateInputsAndOutputs() const override    { return true; }

    AudioIODevice* createDevice (const String& outputDeviceName,
                                 const String& inputDeviceName) override
    {
        jassert (hasScanned); // need to call scanForDevices() before doing this

        std::unique_ptr<WASAPIAudioIODevice> device;

        auto outputIndex = devices.outputDeviceNames.indexOf (outputDeviceName);
        auto inputIndex  = devices.inputDeviceNames .indexOf (inputDeviceName);

        if (outputIndex >= 0 || inputIndex >= 0)
        {
            device.reset (new WASAPIAudioIODevice (outputDeviceName.isNotEmpty() ? outputDeviceName
                                                                                 : inputDeviceName,
                                                   getTypeName(),
                                                   devices.outputDeviceIds[outputIndex],
                                                   devices.inputDeviceIds [inputIndex],
                                                   deviceMode));

            if (! device->initialise())
                device = nullptr;
        }

        return device.release();
    }

    //==============================================================================
    struct Devices
    {
        StringArray outputDeviceNames, outputDeviceIds;
        StringArray inputDeviceNames, inputDeviceIds;

        auto tie() const
        {
            return std::tie (outputDeviceNames, outputDeviceIds, inputDeviceNames, inputDeviceIds);
        }

        bool operator== (const Devices& other) const { return tie() == other.tie(); }
        bool operator!= (const Devices& other) const { return tie() != other.tie(); }
    };

    Devices devices;

private:
    DeviceChangeDetector deviceChangeDetector { L"Windows Audio", [this] { systemDeviceChanged(); } };
    WASAPIDeviceMode deviceMode;
    bool hasScanned = false;
    ComSmartPtr<IMMDeviceEnumerator> enumerator;

    //==============================================================================
    class ChangeNotificationClient final : public ComBaseClassHelper<IMMNotificationClient>
    {
    public:
        explicit ChangeNotificationClient (WASAPIAudioIODeviceType* d)
            : device (d) {}

        JUCE_COMRESULT OnDeviceAdded (LPCWSTR)                             override { return notify(); }
        JUCE_COMRESULT OnDeviceRemoved (LPCWSTR)                           override { return notify(); }
        JUCE_COMRESULT OnDeviceStateChanged (LPCWSTR, DWORD)               override { return notify(); }
        JUCE_COMRESULT OnDefaultDeviceChanged (EDataFlow, ERole, LPCWSTR)  override { return notify(); }
        JUCE_COMRESULT OnPropertyValueChanged (LPCWSTR, const PROPERTYKEY) override { return notify(); }

    private:
        WeakReference<WASAPIAudioIODeviceType> device;

        HRESULT notify()
        {
            if (device != nullptr)
                device->deviceChangeDetector.triggerAsyncDeviceChangeCallback();

            return S_OK;
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChangeNotificationClient)
    };

    ComSmartPtr<ChangeNotificationClient> notifyClient;

    //==============================================================================
    static String getDefaultEndpoint (IMMDeviceEnumerator* enumerator, bool forCapture)
    {
        String s;
        ComSmartPtr<IMMDevice> dev;

        if (check (enumerator->GetDefaultAudioEndpoint (forCapture ? eCapture : eRender,
                                                        eMultimedia, dev.resetAndGetPointerAddress())))
        {
            WCHAR* deviceId = nullptr;

            if (check (dev->GetId (&deviceId)))
            {
                s = deviceId;
                CoTaskMemFree (deviceId);
            }
        }

        return s;
    }

    //==============================================================================
    Devices scan()
    {
        if (enumerator == nullptr)
        {
            if (! check (enumerator.CoCreateInstance (__uuidof (MMDeviceEnumerator))))
                return {};

            notifyClient = becomeComSmartPtrOwner (new ChangeNotificationClient (this));
            enumerator->RegisterEndpointNotificationCallback (notifyClient);
        }

        auto defaultRenderer = getDefaultEndpoint (enumerator, false);
        auto defaultCapture  = getDefaultEndpoint (enumerator, true);

        ComSmartPtr<IMMDeviceCollection> deviceCollection;
        UINT32 numDevices = 0;

        if (! (check (enumerator->EnumAudioEndpoints (eAll, DEVICE_STATE_ACTIVE, deviceCollection.resetAndGetPointerAddress()))
                && check (deviceCollection->GetCount (&numDevices))))
            return {};

        Devices result;

        for (UINT32 i = 0; i < numDevices; ++i)
        {
            ComSmartPtr<IMMDevice> device;

            if (! check (deviceCollection->Item (i, device.resetAndGetPointerAddress())))
                continue;

            DWORD state = 0;

            if (! (check (device->GetState (&state)) && state == DEVICE_STATE_ACTIVE))
                continue;

            auto deviceId = getDeviceID (device);
            String name;

            {
                ComSmartPtr<IPropertyStore> properties;

                if (! check (device->OpenPropertyStore (STGM_READ, properties.resetAndGetPointerAddress())))
                    continue;

                PROPVARIANT value;
                zerostruct (value);

                const PROPERTYKEY PKEY_Device_FriendlyName
                    = { { 0xa45c254e, 0xdf1c, 0x4efd, { 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0 } }, 14 };

                if (check (properties->GetValue (PKEY_Device_FriendlyName, &value)))
                    name = value.pwszVal;

                PropVariantClear (&value);
            }

            auto flow = getDataFlow (device);

            if (flow == eRender)
            {
                const int index = (deviceId == defaultRenderer) ? 0 : -1;
                result.outputDeviceIds.insert (index, deviceId);
                result.outputDeviceNames.insert (index, name);
            }
            else if (flow == eCapture)
            {
                const int index = (deviceId == defaultCapture) ? 0 : -1;
                result.inputDeviceIds.insert (index, deviceId);
                result.inputDeviceNames.insert (index, name);
            }
        }

        result.inputDeviceNames .appendNumbersToDuplicates (false, false);
        result.outputDeviceNames.appendNumbersToDuplicates (false, false);

        return result;
    }

    //==============================================================================
    void systemDeviceChanged()
    {
        const auto newDevices = scan();

        if (std::exchange (devices, newDevices) != newDevices)
        {
            hasScanned = true;
            callDeviceChangeListeners();
        }
    }

    //==============================================================================
    static String getDeviceTypename (WASAPIDeviceMode mode)
    {
        if (mode == WASAPIDeviceMode::shared)            return "Windows Audio";
        if (mode == WASAPIDeviceMode::sharedLowLatency)  return "Windows Audio (Low Latency Mode)";
        if (mode == WASAPIDeviceMode::exclusive)         return "Windows Audio (Exclusive Mode)";

        jassertfalse;
        return {};
    }

    //==============================================================================
    JUCE_DECLARE_WEAK_REFERENCEABLE (WASAPIAudioIODeviceType)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WASAPIAudioIODeviceType)
};

//==============================================================================
struct MMDeviceMasterVolume
{
    MMDeviceMasterVolume()
    {
        ComSmartPtr<IMMDeviceEnumerator> enumerator;

        if (check (enumerator.CoCreateInstance (__uuidof (MMDeviceEnumerator))))
        {
            ComSmartPtr<IMMDevice> device;

            if (check (enumerator->GetDefaultAudioEndpoint (eRender, eConsole, device.resetAndGetPointerAddress())))
                check (device->Activate (__uuidof (IAudioEndpointVolume), CLSCTX_INPROC_SERVER, nullptr,
                                         (void**) endpointVolume.resetAndGetPointerAddress()));
        }
    }

    float getGain() const
    {
        float vol = 0.0f;

        if (endpointVolume != nullptr)
            check (endpointVolume->GetMasterVolumeLevelScalar (&vol));

        return vol;
    }

    bool setGain (float newGain) const
    {
        return endpointVolume != nullptr
                && check (endpointVolume->SetMasterVolumeLevelScalar (jlimit (0.0f, 1.0f, newGain), nullptr));
    }

    bool isMuted() const
    {
        BOOL mute = 0;
        return endpointVolume != nullptr
                 && check (endpointVolume->GetMute (&mute)) && mute != 0;
    }

    bool setMuted (bool shouldMute) const
    {
        return endpointVolume != nullptr
                && check (endpointVolume->SetMute (shouldMute, nullptr));
    }

    ComSmartPtr<IAudioEndpointVolume> endpointVolume;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MMDeviceMasterVolume)
};

} // namespace WasapiClasses

//==============================================================================
/*  Vochlea addition: the probe values for the most recently opened WASAPI device, as a
    single preformatted string.

    Deliberately not a struct: the app-side analytics wiring lives in another repo layer and
    only needs `namespace juce { extern String getLastWASAPIProbeReport(); }` to consume this,
    with no shared type and no new public JUCE header. Per-field accessors were removed because
    they cannot be called - WASAPIProbeInfo and WASAPIAudioIODevice both live in an anonymous-by-
    convention implementation namespace that no header exposes, so this string is the only route
    off the machine. Keep it parseable.
*/
String getLastWASAPIProbeReport()
{
    const ScopedLock sl (WasapiClasses::getLastProbeReportLock());
    return WasapiClasses::getLastProbeReportRef();
}

//==============================================================================
#define JUCE_SYSTEMAUDIOVOL_IMPLEMENTED 1
float JUCE_CALLTYPE SystemAudioVolume::getGain()              { return WasapiClasses::MMDeviceMasterVolume().getGain(); }
bool  JUCE_CALLTYPE SystemAudioVolume::setGain (float gain)   { return WasapiClasses::MMDeviceMasterVolume().setGain (gain); }
bool  JUCE_CALLTYPE SystemAudioVolume::isMuted()              { return WasapiClasses::MMDeviceMasterVolume().isMuted(); }
bool  JUCE_CALLTYPE SystemAudioVolume::setMuted (bool mute)   { return WasapiClasses::MMDeviceMasterVolume().setMuted (mute); }

JUCE_END_IGNORE_WARNINGS_GCC_LIKE

} // namespace juce
