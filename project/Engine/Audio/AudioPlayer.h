#pragma once
#include "Vector3.h"
#include "AudioDefinition.h"

namespace FS
{

class MediaAudioDecoder;

/// @brief 再生中のインスタンス情報
struct AudioInstance
{
    IXAudio2SourceVoice* voice = nullptr;
    std::string name;

    uint32_t channels; // モノラル(1)かステレオ(2)か
    bool is3D;         // 3D音響として鳴らすか
    Vector3 position;  // 音源の3D座標
    float maxDistance; // 音が聞こえる最大距離
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

    // エンジンのメインループで毎フレーム呼ぶ更新処理
    void Update();

    // リスナーの情報をセット
    void SetListener(const Vector3& pos, const Vector3& front, const Vector3& up);
    // シーンを切り替えたときに呼ぶ。新しいカメラの位置が届くまで 3D の音を鳴らさない
    void InvalidateListener() { hasListener_ = false; }

    // 3D座標を指定して再生する関数
    int Play3D(const std::string& name, const Vector3& position, bool loop = false, uint32_t volume = 100, float maxDistance = 100.0f);

    // 動く音源のために座標を後から更新する関数
    void SetEmitterPosition(int handleID, const Vector3& position);

private:
    AudioPlayer() {}
    ~AudioPlayer() { StopAll(); }

    // ボイスを作ってバッファを積み、鳴らさずに登録する。失敗したら -1
    int CreateInstance(const std::string& name, bool loop, uint32_t volume);
    // 鳴らし始める。失敗したら登録ごと消す
    int StartInstance(int handleID);
    // リスナーとの位置関係から、左右の振り分けと距離による減衰を掛ける
    void Apply3D(AudioInstance& instance);

    IXAudio2* xAudio2_ = nullptr; // Initializeで取得想定

    // ロード済みデータ（名前検索用）
    std::unordered_map<std::string, AudioData> audioDataMap_;

    // 再生中のボイス
    std::map<int, AudioInstance> activeVoices_;

    // ユニーク再生管理
    std::unordered_map<std::string, int> uniqueHandles_;

    // 次に発行するハンドルID（連番）
    int nextHandleID_ = 0;

    // リスナーの情報
    X3DAUDIO_LISTENER listener_ = {};
    // まだカメラから位置を受け取っていないとき、前のシーンの位置や向きのない状態で計算しないようにする
    bool hasListener_ = false;
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
    void STDMETHODCALLTYPE OnVoiceProcessingPassStart(uint32_t) override {}
    void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override {}
    void STDMETHODCALLTYPE OnStreamEnd() override {}
    void STDMETHODCALLTYPE OnBufferStart(void*) override {}
    void STDMETHODCALLTYPE OnLoopEnd(void*) override {}
    void STDMETHODCALLTYPE OnVoiceError(void*, HRESULT) override {}
};

}