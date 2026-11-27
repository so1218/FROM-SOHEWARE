#pragma once         

namespace FS
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

    const X3DAUDIO_HANDLE& GetX3DInstance() const { return x3dInstance_; }
    uint32_t GetMasterChannels() const { return masterChannels_; }
    uint32_t GetMasterSampleRate() const { return masterSampleRate_; }
    // ゲームの世界の音をまとめて通す
    IXAudio2SubmixVoice* GetWorldSubmix() const { return worldSubmix_; }

private:
    AudioDevice() = default;
    ~AudioDevice() = default;

    AudioDevice(const AudioDevice&) = delete;
    AudioDevice& operator=(const AudioDevice&) = delete;

    Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
    IXAudio2MasteringVoice* masterVoice_ = nullptr;

    IXAudio2SubmixVoice* worldSubmix_ = nullptr;

    X3DAUDIO_HANDLE x3dInstance_ = {};
    uint32_t masterChannels_ = 0;
    uint32_t masterSampleRate_ = 48000;
};

}