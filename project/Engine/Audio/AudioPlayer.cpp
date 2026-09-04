#include "pch.h"
#include "AudioPlayer.h"
#include "AudioDevice.h"
#include "MediaAudioDecoder.h"

namespace FE
{

// ロード処理
void AudioPlayer::Load(const std::string& name, const std::wstring& filePath)
{
    // 重複チェック
    if (audioDataMap_.find(name) != audioDataMap_.end())
    {
        return;
    }

    AudioData audioData = MediaAudioDecoder::DecodeAudioFile(filePath);

    // ムーブで格納
    audioDataMap_[name] = std::move(audioData);
}

// 再生処理
int AudioPlayer::Play(const std::string& name, bool loop, uint32_t volume)
{
    // 名前でデータを検索
    auto it = audioDataMap_.find(name);
    if (it == audioDataMap_.end())
    {
        // データが見つからない
        return -1;
    }

    const auto& audioData = it->second;

    if (!AudioDevice::GetInstance().GetXAudio2()) return -1;

    // ソースボイス作成
    IXAudio2SourceVoice* sourceVoice = nullptr;
    HRESULT hr = AudioDevice::GetInstance().GetXAudio2()->CreateSourceVoice(&sourceVoice, &audioData.wfex);
    if (FAILED(hr)) return -1;

    // バッファ設定
    XAUDIO2_BUFFER buffer = { 0 };
    buffer.AudioBytes = static_cast<uint32_t>(audioData.buffer.size());
    buffer.pAudioData = audioData.buffer.data();
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    buffer.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;

    hr = sourceVoice->SubmitSourceBuffer(&buffer);
    if (FAILED(hr)) {
        sourceVoice->DestroyVoice();
        return -1;
    }

    // 音量設定
    float fVolume = volume / 100.0f;
    fVolume = std::clamp(fVolume, 0.0f, 10.0f);
    sourceVoice->SetVolume(fVolume);

    // 再生開始
    hr = sourceVoice->Start(0);
    if (FAILED(hr)) {
        sourceVoice->DestroyVoice();
        return -1;
    }

    // ハンドルIDの発行と管理
    int handleID = nextHandleID_;
    nextHandleID_++; // 次回用にカウントアップ

    // mapに登録
    activeVoices_[handleID] = { sourceVoice, name };

    return handleID;
}

// 停止処理
void AudioPlayer::Stop(int handleID)
{
    // mapから検索
    auto it = activeVoices_.find(handleID);
    if (it == activeVoices_.end()) 
    {
        return; // 存在しない、または既に停止済み
    }

    // ボイスの破棄
    if (it->second.voice)
    {
        it->second.voice->Stop(0);
        it->second.voice->FlushSourceBuffers();
        it->second.voice->DestroyVoice();
    }

    // mapから削除
    activeVoices_.erase(it);
}

// 全停止
void AudioPlayer::StopAll()
{
    for (auto& pair : activeVoices_)
    {
        if (pair.second.voice)
        {
            pair.second.voice->Stop(0);
            pair.second.voice->FlushSourceBuffers();
            pair.second.voice->DestroyVoice();
        }
    }
    activeVoices_.clear();
    uniqueHandles_.clear();
}

// 再生中か確認
bool AudioPlayer::IsPlaying(int handleID)
{
    auto it = activeVoices_.find(handleID);
    if (it == activeVoices_.end()) return false;

    if (!it->second.voice) return false;

    XAUDIO2_VOICE_STATE state = {};
    it->second.voice->GetState(&state);

    return (state.BuffersQueued > 0);
}

// ユニーク再生
int AudioPlayer::PlayUnique(const std::string& name, bool loop, uint32_t volume)
{
    // その名前の音がすでに管理されているか確認
    if (uniqueHandles_.count(name))
    {
        int existingHandle = uniqueHandles_[name];

        // まだ再生中か
        if (IsPlaying(existingHandle))
        {
            // 再生中なら何もしない
            return existingHandle;
        }
        else
        {
            // 終わっているなら情報を消して、再作成に進む
            uniqueHandles_.erase(name);
        }
    }

    // 新規再生
    int newHandle = Play(name, loop, volume);

    // 成功したらユニーク管理に登録
    if (newHandle >= 0) {
        uniqueHandles_[name] = newHandle;
    }

    return newHandle;
}

// ユニーク停止
void AudioPlayer::StopUnique(const std::string& name)
{
    if (uniqueHandles_.count(name))
    {
        int handle = uniqueHandles_[name];
        Stop(handle); // 実体を停止
        uniqueHandles_.erase(name); // 管理情報削除
    }
}

}