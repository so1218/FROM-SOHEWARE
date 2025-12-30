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

    float fVolume = volume / 100.0f;
    fVolume = std::clamp(fVolume, 0.0f, 10.0f);
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

bool AudioPlayer::IsPlaying(int instanceID)
{
    // IDが無効、またはリストの範囲外
    if (instanceID < 0 || instanceID >= (int)activeVoices_.size()) {
        return false;
    }

    auto& voiceInfo = activeVoices_[instanceID];

    // Stop() で voiceInfo.voice が nullptr にされている
    if (!voiceInfo.voice) {
        return false;
    }

    // ボイスの状態を確認
    XAUDIO2_VOICE_STATE state = {};
    voiceInfo.voice->GetState(&state);

    // バッファがキューに残っていれば再生中
    return (state.BuffersQueued > 0);
}

int AudioPlayer::PlayUnique(int audioID, bool loop, uint32_t volume)
{
    // すでに再生中なら何もしない
    if (uniqueInstances_.count(audioID))
    {
        int instanceID = uniqueInstances_[audioID];

        // 追跡中のインスタンスがまだ再生中か確認
        if (IsPlaying(instanceID))
        {
            // まだ再生中なので、新しい音は再生せず、既存のIDを返す
            return instanceID;
        }
        else
        {
            // 再生は終わっていたので、マップから削除（再度再生できるようにする）
            uniqueInstances_.erase(audioID);
        }
    }

    // 新しく再生する
    int newInstanceID = Play(audioID, loop, volume);
    if (newInstanceID >= 0) {
        // 再生に成功したら、新しいInstanceIDをマップに登録
        uniqueInstances_[audioID] = newInstanceID;
    }
    return newInstanceID;
}

void AudioPlayer::StopUnique(int audioID)
{
    // この audioID がユニーク再生として追跡されているか確認
    if (uniqueInstances_.count(audioID))
    {
        int instanceID = uniqueInstances_[audioID];

        // 通常の Stop() を使って停止
        Stop(instanceID);

        // 停止したのでマップから削除
        uniqueInstances_.erase(audioID);
    }
}
