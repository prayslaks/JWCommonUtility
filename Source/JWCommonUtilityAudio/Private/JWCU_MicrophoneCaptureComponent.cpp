// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWCU_MicrophoneCaptureComponent.h"
#include "JWCU_PCMConversion.h"
#include "JWCU_AudioPCM.h"
#include "Engine/World.h"
#include "Misc/ScopeLock.h"
#include "Modules/ModuleManager.h"
#if !UE_SERVER
#include "AudioCaptureCore.h"

struct FJWCU_MicrophoneCapture
{
    Audio::FAudioCapture Device;
    TSharedPtr<JWCU::AudioIO::FCaptureBuffer, ESPMode::ThreadSafe> Buffer = MakeShared<JWCU::AudioIO::FCaptureBuffer, ESPMode::ThreadSafe>();
    int32 Rate = 24000;
    ~FJWCU_MicrophoneCapture() { Device.AbortStream(); }
};
#else
struct FJWCU_MicrophoneCapture {};
#endif

UJWCU_MicrophoneCaptureComponent::UJWCU_MicrophoneCaptureComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}
bool UJWCU_MicrophoneCaptureComponent::StartCapture(int32 SampleRate, int32 DeviceIndex)
{
    check(IsInGameThread());
    if (Capture) { return false; }
#if !UE_SERVER
    if (!IsRegistered() || !GetWorld() || GetWorld()->GetNetMode() == NM_DedicatedServer || !JWCU::AudioIO::IsSupportedSampleRate(SampleRate))
    { Report(TEXT("Capture requires a registered client component and a supported PCM sample rate.")); return false; }
    if (!FModuleManager::Get().LoadModule(TEXT("AudioCaptureWasapi")))
    { Report(TEXT("The Windows capture backend is unavailable.")); return false; }
    auto NewCapture = MakeShared<FJWCU_MicrophoneCapture>();
    NewCapture->Rate = SampleRate;
    NewCapture->Buffer->Converter.TargetRate = SampleRate;
    Audio::FAudioCaptureDeviceParams Params;
    Params.DeviceIndex = DeviceIndex;
    Params.PCMAudioEncoding = Audio::EPCMAudioEncoding::FLOATING_POINT_32;
    // 콜백은 UObject를 만지지 않는다. 종료 후에도 공유 버퍼 수명은 콜백이 보장한다.
    auto Buffer = NewCapture->Buffer;
    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    const bool bOpened = NewCapture->Device.OpenCaptureStream(Params,
        [Buffer](const float* Input, int32 Frames, int32 Channels, int32 Rate, double Time, bool bOverflow)
        {
            Buffer->Push(Input, Frames, Channels, Rate, bOverflow);
        }, 1024);
    PRAGMA_ENABLE_DEPRECATION_WARNINGS
    // UE 5.7의 새 OpenAudioCaptureStream은 DLL export가 없어 내보낸 float32 진입점을 사용한다.
    if (!bOpened || !NewCapture->Device.StartStream())
    { Report(TEXT("Cannot open/start the microphone. Check the device and Windows microphone permission.")); return false; }
    Capture = NewCapture;
    ++CaptureGeneration;
    InputLevel = 0;
    SetComponentTickEnabled(true);
    return true;
#else
    Report(TEXT("Microphone capture is unavailable on server targets."));
    return false;
#endif
}
bool UJWCU_MicrophoneCaptureComponent::IsCapturing() const { return Capture.IsValid(); }
TArray<FJWCU_AudioCaptureDevice> UJWCU_MicrophoneCaptureComponent::GetCaptureDevices()
{
    check(IsInGameThread());
    TArray<FJWCU_AudioCaptureDevice> Result;
#if !UE_SERVER
    if (!FModuleManager::Get().LoadModule(TEXT("AudioCaptureWasapi"))) { return Result; }
    Audio::FAudioCapture Device;
    TArray<Audio::FCaptureDeviceInfo> Devices;
    Device.GetCaptureDevicesAvailable(Devices);
    for (int32 Index = 0; Index < Devices.Num(); ++Index)
    {
        FJWCU_AudioCaptureDevice Info;
        Info.DeviceIndex = Index; Info.Name = Devices[Index].DeviceName;
        Info.Channels = Devices[Index].InputChannels; Info.PreferredSampleRate = Devices[Index].PreferredSampleRate;
        Result.Add(MoveTemp(Info));
    }
#endif
    return Result;
}
void UJWCU_MicrophoneCaptureComponent::StopCapture()
{
    check(IsInGameThread());
    SetComponentTickEnabled(false);
    Capture.Reset();
    ++CaptureGeneration;
    InputLevel = 0;
}
void UJWCU_MicrophoneCaptureComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
#if !UE_SERVER
    if (!Capture) { return; }
    const uint64 Generation = CaptureGeneration;
    TArray<uint8> Bytes;
    bool bOverflow = false;
    const int32 ChunkBytes = Capture->Rate / 50 * 2; // 20ms
    {
        FScopeLock Lock(&Capture->Buffer->Mutex);
        bOverflow = Capture->Buffer->bOverflow || FPlatformTime::Seconds() - Capture->Buffer->LastCallbackSeconds > 5.;
        const int32 Count = Capture->Buffer->Bytes.Num() / ChunkBytes * ChunkBytes;
        Bytes.Append(Capture->Buffer->Bytes.GetData(), Count);
        Capture->Buffer->Bytes.RemoveAt(0, Count, EAllowShrinking::No);
    }
    if (bOverflow) { StopCapture(); Report(TEXT("Microphone buffer overflow, device discontinuity or no samples for five seconds.")); return; }
    for (int32 Offset = 0; Offset < Bytes.Num() && Capture && Generation == CaptureGeneration; Offset += ChunkBytes)
    {
        TArray<uint8> Chunk; Chunk.Append(Bytes.GetData() + Offset, ChunkBytes);
        InputLevel = JWCU::AudioIO::RMS(Chunk);
        OnPCMNative.Broadcast(Chunk);
        if (Capture && Generation == CaptureGeneration) { OnPCM.Broadcast(Chunk); }
    }
#endif
}
void UJWCU_MicrophoneCaptureComponent::Report(const FString& Message)
{
    FJWCU_AudioError Error;
    Error.Code = TEXT("microphone"); Error.Message = Message; Error.bFatal = true;
    OnErrorNative.Broadcast(Error); OnError.Broadcast(Error);
}
void UJWCU_MicrophoneCaptureComponent::EndPlay(const EEndPlayReason::Type Reason) { StopCapture(); Super::EndPlay(Reason); }
void UJWCU_MicrophoneCaptureComponent::BeginDestroy() { Capture.Reset(); Super::BeginDestroy(); }
void UJWCU_MicrophoneCaptureComponent::OnComponentDestroyed(bool bDestroyingHierarchy) { StopCapture(); Super::OnComponentDestroyed(bDestroyingHierarchy); }
