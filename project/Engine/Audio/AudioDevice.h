#pragma once         

namespace FE
{

class AudioDevice
{
public:

    static AudioDevice& GetInstance()
    {
        static AudioDevice instance;
        return instance;
    }

    void Initialize();
    void Finalize();

    IXAudio2* GetXAudio2() const { return xAudio2_.Get(); }

private:
    AudioDevice() = default;
    ~AudioDevice() = default;

    AudioDevice(const AudioDevice&) = delete;
    AudioDevice& operator=(const AudioDevice&) = delete;

    Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
    IXAudio2MasteringVoice* masterVoice_ = nullptr;
};

}