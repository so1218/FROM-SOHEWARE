#include "AudioManager.h"
#include "AudioPlayer.h"

#include <cassert>
#include <mfapi.h>
#include <mfplay.h>
#include <mfreadwrite.h>
#include <mfobjects.h>
#include <mfidl.h>
#include <shlwapi.h> 
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "shlwapi.lib")

void AudioManager::Initialize()
{
    // Media Foundation の初期化
    HRESULT hr = MFStartup(MF_VERSION);
    assert(SUCCEEDED(hr));

    // XAudio2 初期化
    hr = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
    assert(SUCCEEDED(hr));
    hr = xAudio2_->CreateMasteringVoice(&masterVoice_);
    assert(SUCCEEDED(hr));
}

void AudioManager::Finalize()
{
    // 音声データ開放
    AudioPlayer::GetInstance().StopAll();
    MFShutdown();
}