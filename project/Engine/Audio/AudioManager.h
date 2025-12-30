#pragma once
#include <xaudio2.h>           
#include <wrl.h>     

// XAudio2の生成・管理を行うオーディオマネージャ
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

    // ゲッター
    IXAudio2* GetXAudio2() const { return xAudio2_.Get(); }

private:
    AudioManager() = default;
    ~AudioManager() = default;

    // シングルトンのためコピー・ムーブ禁止
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;

    // マスターボイス
    IXAudio2MasteringVoice* masterVoice_ = nullptr;
};