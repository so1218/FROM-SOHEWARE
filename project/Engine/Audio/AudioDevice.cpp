#include "pch.h"
#include "AudioDevice.h"
#include "AudioPlayer.h"

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "shlwapi.lib")

namespace FS
{

// AudioDeviceの初期化
void AudioDevice::Initialize()
{
    // MediaFoundationの初期化
    HRESULT hr = MFStartup(MF_VERSION);
    assert(SUCCEEDED(hr));

    // XAudio2 初期化
    hr = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
    assert(SUCCEEDED(hr));
    hr = xAudio2_->CreateMasteringVoice(&masterVoice_);
    assert(SUCCEEDED(hr));

    // X3DAudio の初期化
    XAUDIO2_VOICE_DETAILS details;
    masterVoice_->GetVoiceDetails(&details);
    masterChannels_ = details.InputChannels;
    masterSampleRate_ = details.InputSampleRate;

    // 世界の音の通り道。こもらせるためにフィルターを使えるようにして作る
    hr = xAudio2_->CreateSubmixVoice(&worldSubmix_, masterChannels_, masterSampleRate_, XAUDIO2_VOICE_USEFILTER);
    if (FAILED(hr))
    {
        worldSubmix_ = nullptr;
    }
    else
    {
        // 作った直後は何も削らない状態にしておく
        XAUDIO2_FILTER_PARAMETERS filter = { LowPassOnePoleFilter, 1.0f, 1.0f };
        worldSubmix_->SetFilterParameters(&filter);
    }

    DWORD channelMask;
    hr = masterVoice_->GetChannelMask(&channelMask);
    if (FAILED(hr)) channelMask = SPEAKER_STEREO; 

    // 初期化
    X3DAudioInitialize(channelMask, X3DAUDIO_SPEED_OF_SOUND, x3dInstance_);
}

void AudioDevice::Finalize()
{
    // 音声データ開放。通り道は、そこへ送っている音を全部消してから消す
    AudioPlayer::GetInstance().StopAll();
    if (worldSubmix_)
    {
        worldSubmix_->DestroyVoice();
        worldSubmix_ = nullptr;
    }
    MFShutdown();
}

}