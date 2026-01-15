#pragma once
#include <xaudio2.h>           
#include <wrl.h>     

namespace FromEngine
{
    /// @brief オーディオ管理クラス
    class AudioManager
    {
    public:
        /// @brief シングルトンインスタンス取得
        /// @return AudioManagerのインスタンス
        static AudioManager& GetInstance()
        {
            static AudioManager instance;
            return instance;
        }

        /// @brief 初期化
        void Initialize();
        /// @brief 終了処理
        void Finalize();

        /// @brief  XAudio2インスタンス取得
        /// @return IXAudio2インスタンス
        IXAudio2* GetXAudio2() const { return xAudio2_.Get(); }

    private:
        /// @brief コンストラクタ・デストラクタ
        AudioManager() = default;
        ~AudioManager() = default;

        /// @brief コピー禁止
        /// @param  
        AudioManager(const AudioManager&) = delete;
        AudioManager& operator=(const AudioManager&) = delete;

        /// @brief XAudio2インスタンス
        Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;

        /// @brief マスターボイス
        IXAudio2MasteringVoice* masterVoice_ = nullptr;
    };
}