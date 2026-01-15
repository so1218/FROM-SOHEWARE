#pragma once
#include <xaudio2.h>           
#include <wrl.h>     

namespace FromEngine
{
    class AudioManager
    {
    public:

        static AudioManager& GetInstance()
        {
            static AudioManager instance;
            return instance;
        }

        void Initialize();
        void Finalize();

        IXAudio2* GetXAudio2() const { return xAudio2_.Get(); }

    private:
        AudioManager() = default;
        ~AudioManager() = default;

        AudioManager(const AudioManager&) = delete;
        AudioManager& operator=(const AudioManager&) = delete;

        Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
        IXAudio2MasteringVoice* masterVoice_ = nullptr;
    };
}