#include "AudioPlayer.h"
#include "AudioManager.h"

#include <algorithm>

int AudioPlayer::Load(const std::wstring& filePath)
{
    AudioData audioData = MediaAudioDecoder::DecodeAudioFile(filePath);
    loadedAudios_.push_back(std::move(audioData));
    return static_cast<int>(loadedAudios_.size()) - 1;
}

// 音声再生
int AudioPlayer::Play(int audioID, bool loop, uint32_t volume)
{
    if (audioID < 0 || audioID >= (int)loadedAudios_.size()) return -1;
    if (!AudioManager::GetInstance().GetXAudio2()) return -1;

    const auto& audioData = loadedAudios_[audioID];

    IXAudio2SourceVoice* sourceVoice = nullptr;
    HRESULT hr = AudioManager::GetInstance().GetXAudio2()->CreateSourceVoice(&sourceVoice, &audioData.wfex);
    if (FAILED(hr)) return -1;

    XAUDIO2_BUFFER buffer = { 0 };
    buffer.AudioBytes = audioData.bufferSize;
    buffer.pAudioData = audioData.pBuffer;
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    buffer.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;

    hr = sourceVoice->SubmitSourceBuffer(&buffer);
    if (FAILED(hr)) {
        sourceVoice->DestroyVoice();
        return -1;
    }

    float fVolume = std::clamp(volume / 100.0f, 0.0f, 1.0f);
    sourceVoice->SetVolume(fVolume);

    hr = sourceVoice->Start(0);
    if (FAILED(hr)) {
        sourceVoice->DestroyVoice();
        return -1;
    }

    // 再生中の音声を管理
    activeVoices_.push_back({ sourceVoice, audioID });

    // 再生中の音声のIDとしてインデックスを返す
    return static_cast<int>(activeVoices_.size()) - 1;
}

void AudioPlayer::Stop(int instanceID)
{
    if (instanceID < 0 || instanceID >= (int)activeVoices_.size()) return;

    auto& voiceInfo = activeVoices_[instanceID];
    if (voiceInfo.voice)
    {
        voiceInfo.voice->Stop(0);
        voiceInfo.voice->FlushSourceBuffers();
        voiceInfo.voice->DestroyVoice();
        voiceInfo.voice = nullptr;
    }

    // 無効化or削除
    voiceInfo.audioIndex = -1;
}

void AudioPlayer::StopAll()
{
    for (auto& voiceInfo : activeVoices_)
    {
        if (voiceInfo.voice) {
            voiceInfo.voice->Stop(0);
            voiceInfo.voice->FlushSourceBuffers();
            voiceInfo.voice->DestroyVoice();
            voiceInfo.voice = nullptr;
        }
        voiceInfo.audioIndex = -1;
    }
    activeVoices_.clear();
}

int AudioPlayer::PlayUnique(int audioID, bool loop, uint32_t volume)
{
    // すでに再生中なら何もしない
    if (currentBGMInstanceID_ >= 0 && currentBGMInstanceID_ < (int)activeVoices_.size())
    {
        auto& voiceInfo = activeVoices_[currentBGMInstanceID_];
        if (voiceInfo.voice) {
            XAUDIO2_VOICE_STATE state = {};
            voiceInfo.voice->GetState(&state);
            if (state.BuffersQueued > 0) 
            {
                // 再生中なので再度再生しない
                return currentBGMInstanceID_;
            }
        }
    }

    // 新しく再生
    int newInstanceID = Play(audioID, loop, volume);
    currentBGMInstanceID_ = newInstanceID;
    return newInstanceID;
}

void AudioPlayer::StopUnique()
{
    if (currentBGMInstanceID_ >= 0) {
        Stop(currentBGMInstanceID_);
        currentBGMInstanceID_ = -1;
    }
}
