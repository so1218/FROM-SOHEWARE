#pragma once
#include "PropertyBinder.h"
#include "Text.h"

// 無線の字幕
class RadioSubtitle
{
public:
    static constexpr int kMaxLineCount = 3;

    explicit RadioSubtitle(FS::Engine* engine);
    ~RadioSubtitle();

    // 設定を prefix を付けたキーで保存する
    void BindProperties(FS::PropertyBinder& binder, const std::string& prefix);
    void DrawTimeProperties(FS::PropertyBinder& binder, const std::string& prefix);
    void DrawLookProperties(FS::PropertyBinder& binder, const std::string& prefix);
    void DrawSoundProperties(FS::PropertyBinder& binder, const std::string& prefix);

    // startDelay 秒たってから無線が入り、打ち始める
    void Start(const std::string& speaker, const std::vector<std::string>& lines, float startDelay = 0.0f);
    // 雑音を止めて、何も出していない状態に戻す
    void Stop();
    void Update(float deltaTime);
    // 打っている途中なら、残りを一気に出す
    void SkipTyping();

    bool IsStarted() const { return isActive_ && timer_ >= 0.0f; }
    bool IsTyped() const;
    // 打ち終えてからの時間
    float GetTypedTime() const { return typedTimer_; }

    // 無線が入る音と同じ音を鳴らす
    bool PlayClick() const;

    // 話している人の名前を topY に置き、その下に行を並べる
    void Draw(float topY, float opacity);
    // 最後の行の上端の位置
    float GetLastLineY(float topY) const;

    std::vector<FS::Text*> GetTexts();
    const std::string& GetFontName() const { return speakerText_->GetFontName(); }

private:
    void BuildRevealTimes();
    void UpdateTyping();
    void StartNoise();
    void StopNoise();

    FS::Engine* engine_ = nullptr;
    std::unique_ptr<FS::Text> speakerText_;
    std::unique_ptr<FS::Text> lineTexts_[kMaxLineCount];

    bool isActive_ = false;
    float timer_ = 0.0f;
    float typedTimer_ = 0.0f;
    bool isTypingSkipped_ = false;
    bool hasStartedNoise_ = false;
    int noiseHandle_ = -1;

    std::string lines_[kMaxLineCount];
    size_t lineLengths_[kMaxLineCount] = {};
    size_t shownLengths_[kMaxLineCount] = {};
    std::vector<float> revealTimes_[kMaxLineCount];
    float typingDuration_ = 0.0f;

    // 時間
    float charactersPerSecond_ = 16.0f;
    float linePause_ = 0.4f;
    float commaPause_ = 0.12f;
    float periodPause_ = 0.25f;
    float flickerTime_ = 0.35f;

    // 見た目
    float fontSize_ = 30.0f;
    float speakerFontSize_ = 20.0f;
    float speakerGap_ = 34.0f;
    float lineGap_ = 42.0f;
    FS::Vector4 textColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
    FS::Vector4 speakerColor_ = { 0.7f, 0.75f, 0.7f, 0.9f };

    // 音
    std::string noiseSoundName_ = "radioNoise";
    float noiseVolume_ = 0.35f;
    std::string clickSoundName_ = "radioClick";
    float clickVolume_ = 0.6f;
    std::string typeSoundName_ = "radioType";
    float typeSoundVolume_ = 0.25f;
};
