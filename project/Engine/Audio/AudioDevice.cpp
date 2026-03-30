#include "pch.h"
#include "AudioDevice.h"
#include "AudioPlayer.h"

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "shlwapi.lib")

namespace FE
{

/// @brief AudioDeviceの初期化
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
}

void AudioDevice::Finalize()
{
    // 音声データ開放
    AudioPlayer::GetInstance().StopAll();
    MFShutdown();
}

}