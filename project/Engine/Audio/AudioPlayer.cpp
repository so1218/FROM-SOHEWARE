#include "pch.h"
#include "AudioPlayer.h"
#include "DevError.h"
#include "AudioDevice.h"
#include "MediaAudioDecoder.h"

namespace FS
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
int AudioPlayer::Play(const std::string& name, float volume, std::source_location location)
{
    return StartInstance(CreateInstance(name, false, volume, location));
}

int AudioPlayer::PlayLoop(const std::string& name, float volume, std::source_location location)
{
    return StartInstance(CreateInstance(name, true, volume, location));
}

int AudioPlayer::CreateInstance(const std::string& name, bool isLoop, float volume, const std::source_location& location)
{
    auto it = audioDataMap_.find(name);
    if (it == audioDataMap_.end())
    {
        DevError::Report(std::format("音 \"{}\" が読み込まれていない\nAssets/Audio にその名前のファイルがあるか確認する", name), location);
        return -1;
    }

    const auto& audioData = it->second;

    if (!AudioDevice::GetInstance().GetXAudio2()) return -1;

    IXAudio2SourceVoice* sourceVoice = nullptr;
    HRESULT hr = AudioDevice::GetInstance().GetXAudio2()->CreateSourceVoice(&sourceVoice, &audioData.wfex);
    if (FAILED(hr)) return -1;

    XAUDIO2_BUFFER buffer = { 0 };
    buffer.AudioBytes = static_cast<uint32_t>(audioData.buffer.size());
    buffer.pAudioData = audioData.buffer.data();
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    buffer.LoopCount = isLoop ? XAUDIO2_LOOP_INFINITE : 0;

    hr = sourceVoice->SubmitSourceBuffer(&buffer);
    if (FAILED(hr)) {
        sourceVoice->DestroyVoice();
        return -1;
    }

    sourceVoice->SetVolume(std::clamp(volume, 0.0f, 10.0f));

    int handleID = nextHandleID_;
    nextHandleID_++;

    activeVoices_[handleID] = {
        sourceVoice,
        name,
        audioData.wfex.nChannels,
        false,
        {0, 0, 0},
        100.0f
    };

    return handleID;
}

int AudioPlayer::StartInstance(int handleID)
{
    auto it = activeVoices_.find(handleID);
    if (it == activeVoices_.end()) return -1;

    if (FAILED(it->second.voice->Start(0)))
    {
        it->second.voice->DestroyVoice();
        activeVoices_.erase(it);
        return -1;
    }
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

int AudioPlayer::PlayBgm(const std::string& name, float volume, std::source_location location)
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
    int newHandle = PlayLoop(name, volume, location);

    // 成功したらユニーク管理に登録
    if (newHandle >= 0) {
        uniqueHandles_[name] = newHandle;
    }

    return newHandle;
}

void AudioPlayer::StopBgm(const std::string& name)
{
    if (uniqueHandles_.count(name))
    {
        int handle = uniqueHandles_[name];
        Stop(handle); // 実体を停止
        uniqueHandles_.erase(name); // 管理情報削除
    }
}

void AudioPlayer::SetListener(const Vector3& pos, const Vector3& front, const Vector3& up)
{
    listener_.Position = { pos.x, pos.y, pos.z };
    listener_.OrientFront = { front.x, front.y, front.z };
    listener_.OrientTop = { up.x, up.y, up.z };
    hasListener_ = true;
}

int AudioPlayer::Play3D(const std::string& name, const Vector3& position, const Sound3DSettings& settings,
    std::source_location location)
{
    int handleID = CreateInstance(name, settings.isLoop, settings.volume, location);
    if (handleID < 0) return -1;

    // 鳴らし始める前に減衰を掛けておかないと、最初の 1 フレームは距離に関係なく最大音量で鳴る
    AudioInstance& instance = activeVoices_[handleID];
    instance.is3D = true;
    instance.position = position;
    instance.maxDistance = settings.maxDistance;
    Apply3D(instance);

    return StartInstance(handleID);
}

void AudioPlayer::SetEmitterPosition(int handleID, const Vector3& position)
{
    auto it = activeVoices_.find(handleID);
    if (it != activeVoices_.end()) {
        it->second.position = position;
    }
}

void AudioPlayer::Update()
{
    for (auto it = activeVoices_.begin(); it != activeVoices_.end(); )
    {
        // 再生が終わったボイスは破棄してから外す
        if (!IsPlaying(it->first))
        {
            if (it->second.voice)
            {
                it->second.voice->DestroyVoice();
            }
            it = activeVoices_.erase(it);
            continue;
        }

        if (it->second.is3D && it->second.voice)
        {
            Apply3D(it->second);
        }

        ++it;
    }
}

void AudioPlayer::Apply3D(AudioInstance& instance)
{
    const X3DAUDIO_HANDLE& x3dInstance = AudioDevice::GetInstance().GetX3DInstance();
    uint32_t dstChannels = AudioDevice::GetInstance().GetMasterChannels();

    float dx = instance.position.x - listener_.Position.x;
    float dy = instance.position.y - listener_.Position.y;
    float dz = instance.position.z - listener_.Position.z;
    float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

    // 聞く位置が決まっていないときと maxDistance 以上離れているときは音量を 0 にする
    if (!hasListener_ || dist >= instance.maxDistance)
    {
        float zeroMatrix[8] = { 0 };
        instance.voice->SetOutputMatrix(NULL, instance.channels, dstChannels, zeroMatrix);
        return;
    }

    X3DAUDIO_EMITTER emitter = {};
    emitter.ChannelCount = instance.channels;
    emitter.CurveDistanceScaler = instance.maxDistance;
    emitter.Position = { instance.position.x, instance.position.y, instance.position.z };
    emitter.OrientFront = { 0, 0, 1 };
    emitter.OrientTop = { 0, 1, 0 };

    X3DAUDIO_DSP_SETTINGS dspSettings = {};
    float matrix[8] = { 0 };
    dspSettings.SrcChannelCount = instance.channels;
    dspSettings.DstChannelCount = dstChannels;
    dspSettings.pMatrixCoefficients = matrix;

    X3DAudioCalculate(x3dInstance, &listener_, &emitter, X3DAUDIO_CALCULATE_MATRIX, &dspSettings);

    // maxDistance に近づくにつれてスムーズに 0 に落とす
    float distanceFactor = std::clamp(1.0f - (dist / instance.maxDistance), 0.0f, 1.0f);
    for (uint32_t i = 0; i < instance.channels * dstChannels; ++i)
    {
        matrix[i] *= distanceFactor;
    }

    instance.voice->SetOutputMatrix(NULL, dspSettings.SrcChannelCount, dspSettings.DstChannelCount, matrix);
}

}