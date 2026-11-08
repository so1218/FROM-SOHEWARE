#pragma once
#include "Vector3.h"
#include "AudioDefinition.h"

namespace FS
{

class MediaAudioDecoder;

// 再生中のインスタンス情報
struct AudioInstance
{
    IXAudio2SourceVoice* voice = nullptr;
    std::string name;

    uint32_t channels; // モノラル(1)かステレオ(2)か
    bool is3D;         // 3D音響として鳴らすか
    Vector3 position;  // 音源の3D座標
    float maxDistance; // 音が聞こえる最大距離
};

// 3D で鳴らすときの設定
struct Sound3DSettings
{
    float volume = 1.0f;         // 0 ~ 1
    float maxDistance = 100.0f;  // これより離れると聞こえない
    bool isLoop = false;
};

// 音声再生管理クラス
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

    // 名前を紐付けてロード
    void Load(const std::string& name, const std::wstring& filePath);

    // volume は 0 ~ 1
    // 1 回だけ鳴らす
    int Play(const std::string& name, float volume = 1.0f, std::source_location location = std::source_location::current());
    // 止めるまで繰り返し鳴らす
    int PlayLoop(const std::string& name, float volume = 1.0f, std::source_location location = std::source_location::current());

    // ハンドルIDを指定して停止
    void Stop(int handleID);

    // 全停止
    void StopAll();

    // BGM を繰り返し鳴らす。同じ名前が鳴っていれば何もしない
    int PlayBgm(const std::string& name, float volume = 1.0f, std::source_location location = std::source_location::current());
    void StopBgm(const std::string& name);

    // 再生中か確認
    bool IsPlaying(int handleID);

    // エンジンのメインループで毎フレーム呼ぶ更新処理
    void Update();

    // リスナーの情報をセット
    void SetListener(const Vector3& pos, const Vector3& front, const Vector3& up);
    // シーンを切り替えたときに呼ぶ。新しいカメラの位置が届くまで 3D の音を鳴らさない
    void InvalidateListener() { hasListener_ = false; }

    // position から聞こえるように鳴らす。聞く位置はカメラ
    int Play3D(const std::string& name, const Vector3& position, const Sound3DSettings& settings = {},
        std::source_location location = std::source_location::current());

    // 動く音源のために座標を後から更新する関数
    void SetEmitterPosition(int handleID, const Vector3& position);

private:
    AudioPlayer() {}
    ~AudioPlayer() { StopAll(); }

    // ボイスを作ってバッファを積み、鳴らさずに登録する。失敗したら -1
    int CreateInstance(const std::string& name, bool isLoop, float volume, const std::source_location& location);
    // 鳴らし始める。失敗したら登録ごと消す
    int StartInstance(int handleID);
    // リスナーとの位置関係から、左右の振り分けと距離による減衰を掛ける
    void Apply3D(AudioInstance& instance);

    IXAudio2* xAudio2_ = nullptr; 

    // ロード済みデータ（名前検索用）
    std::unordered_map<std::string, AudioData> audioDataMap_;

    // 再生中のボイス
    std::map<int, AudioInstance> activeVoices_;

    // ユニーク再生管理
    std::unordered_map<std::string, int> uniqueHandles_;

    // 次に発行するハンドルID
    int nextHandleID_ = 0;

    // リスナーの情報
    X3DAUDIO_LISTENER listener_ = {};
    // まだカメラから位置を受け取っていないとき、前のシーンの位置や向きのない状態で計算しないように
    bool hasListener_ = false;
};

// コールバック
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