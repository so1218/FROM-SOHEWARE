#pragma once

#include "MediaAudioDecoder.h"

#include <xaudio2.h>       
#include <cassert>         
#include <cstdint>  
#include <vector>
#include <functional>

namespace FromEngine
{
	/// @brief 音声データ情報
    struct AudioInstance
    {
        IXAudio2SourceVoice* voice = nullptr;
        int audioIndex = -1;
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

        // コピー禁止
        AudioPlayer(const AudioPlayer&) = delete;
        AudioPlayer& operator=(const AudioPlayer&) = delete;

        int Load(const std::wstring& filePath);
        int Play(int audioID, bool loop = false, uint32_t volume = 100);
        void Stop(int instanceID);
        void StopAll();
        int PlayUnique(int audioID, bool loop = true, uint32_t volume = 100);
        void StopUnique(int audioID);
        bool IsPlaying(int instanceID);

    private:
        AudioPlayer() {}
        ~AudioPlayer() {}

        std::vector<AudioData> loadedAudios_;
        std::vector<AudioInstance> activeVoices_;
        IXAudio2* xAudio2_ = nullptr;

        std::unordered_map<int, int> uniqueInstances_;
    };

    class VoiceCallback : public IXAudio2VoiceCallback
    {
    public:
        std::function<void()> onBufferEnd_;

        VoiceCallback(std::function<void()> onBufferEnd = nullptr) : onBufferEnd_(onBufferEnd) {}

        void STDMETHODCALLTYPE OnBufferEnd(void* pBufferContext) override {
            if (onBufferEnd_) onBufferEnd_();
        }

        // 他のメソッドは空
        void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32) override {}
        void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override {}
        void STDMETHODCALLTYPE OnStreamEnd() override {}
        void STDMETHODCALLTYPE OnBufferStart(void*) override {}
        void STDMETHODCALLTYPE OnLoopEnd(void*) override {}
        void STDMETHODCALLTYPE OnVoiceError(void*, HRESULT) override {}
    };
}