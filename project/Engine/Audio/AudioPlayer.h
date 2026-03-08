#pragma once
#include "MediaAudioDecoder.h"

/// @brief 再生中のインスタンス情報
struct AudioInstance
{
    IXAudio2SourceVoice* voice = nullptr;
    std::string audioName; // どの音を再生しているか（デバッグや管理用）
};

/// @brief 音声再生管理クラス
class AudioPlayer
{
public:
    static AudioPlayer& GetInstance()
    {
        static AudioPlayer instance;
        return instance;
    }

    AudioPlayer(const AudioPlayer&) = delete;
    AudioPlayer& operator=(const AudioPlayer&) = delete;

    // 名前を紐付けてロードする
    void Load(const std::string& name, const std::wstring& filePath);

    // 名前で再生し、ユニークなハンドルIDを返す
    int Play(const std::string& name, bool loop = false, uint32_t volume = 100);

    // ハンドルIDを指定して停止
    void Stop(int handleID);

    // 全停止
    void StopAll();

    // BGMなどの重複防止再生
    int PlayUnique(const std::string& name, bool loop = true, uint32_t volume = 100);

    // ユニーク再生の停止
    void StopUnique(const std::string& name);

    // 再生中か確認
    bool IsPlaying(int handleID);

private:
    AudioPlayer() {}
    ~AudioPlayer() { StopAll(); }

    IXAudio2* xAudio2_ = nullptr; // Initializeで取得想定

    // ロード済みデータ（名前検索用）
    std::unordered_map<std::string, AudioData> audioDataMap_;

    // 再生中のボイス
    std::map<int, AudioInstance> activeVoices_;

    // ユニーク再生管理
    std::unordered_map<std::string, int> uniqueHandles_;

    // 次に発行するハンドルID（連番）
    int nextHandleID_ = 0;
};

// コールバックは変更なしでOK
class VoiceCallback : public IXAudio2VoiceCallback
{
public:
    std::function<void()> onBufferEnd_;
    VoiceCallback(std::function<void()> onBufferEnd = nullptr) : onBufferEnd_(onBufferEnd) {}
    void STDMETHODCALLTYPE OnBufferEnd(void* pBufferContext) override {
        if (onBufferEnd_) onBufferEnd_();
    }
    void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32) override {}
    void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override {}
    void STDMETHODCALLTYPE OnStreamEnd() override {}
    void STDMETHODCALLTYPE OnBufferStart(void*) override {}
    void STDMETHODCALLTYPE OnLoopEnd(void*) override {}
    void STDMETHODCALLTYPE OnVoiceError(void*, HRESULT) override {}
};
