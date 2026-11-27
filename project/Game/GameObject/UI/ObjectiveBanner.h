#pragma once
#include "GameObject.h"
#include "PropertyBinder.h"
#include "Sprite.h"
#include "Text.h"

class ObjectiveBanner : public FS::GameObject
{
public:
    explicit ObjectiveBanner(FS::Engine* engine);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugGui() override;

    // playSeconds は本番の長さ
    void Begin(bool withFade, bool isFirstTime, int endHour, float playSeconds);
    void Cancel();

    // 画面が真っ暗になった瞬間に一度だけ true を返す
    bool TakeResetRequest();
    // 目的を出し終えた瞬間に一度だけ true を返す
    bool TakeFinished();
    bool IsActive() const { return phase_ != Phase::Hidden; }

private:
    enum class Phase
    {
        Hidden,
        FadeOut,
        Hold,
        FadeIn,
        Show,
        Leave,
    };

    void ChangePhase(Phase phase);
    void DrawContent(float opacity);
    // 各行を、今出ている文字数まで表示する
    void UpdateTyping();
    void DrawReplyPrompt(float opacity);
    // 全部の文字を打ち終える時間
    float GetTypingDuration() const;
    // 1 文字ずつの出る時刻を決める
    void BuildRevealTimes();
    void PlayTypeSound();
    std::string ApplyPlaceholders(const std::string& text) const;
    void StartNoise();
    void StopNoise();
    std::vector<FS::Text*> GetTexts() const;

    FS::Engine* engine_ = nullptr;
    std::unique_ptr<FS::PropertyBinder> binder_;

    std::unique_ptr<FS::Sprite> blackout_;
    static constexpr int kLineCount = 3;

    std::unique_ptr<FS::Text> speakerText_;
    std::unique_ptr<FS::Text> lineTexts_[kLineCount];
    std::unique_ptr<FS::Sprite> replyIcon_;
    std::unique_ptr<FS::Text> replyText_;

    Phase phase_ = Phase::Hidden;
    float phaseTimer_ = 0.0f;
    bool isResetRequested_ = false;
    bool isFinished_ = false;
    bool isTypingSkipped_ = false;
    bool hasStartedNoise_ = false;
    int noiseHandle_ = -1;

    // Begin で決まる中身
    int endHour_ = 20;
    float playSeconds_ = 0.0f;
    std::string lines_[kLineCount];
    size_t lineLengths_[kLineCount] = {};
    size_t shownLengths_[kLineCount] = {};
    // 打ち終えてからの時間。答えるボタンを出すのに使う
    float typedTimer_ = 0.0f;
    std::vector<float> revealTimes_[kLineCount];
    float typingDuration_ = 0.0f;

    // 暗転
    float fadeOutTime_ = 0.8f;
    float holdTime_ = 0.5f;
    float fadeInTime_ = 0.8f;

    // 文章
    std::string speakerName_ = "無線";
    std::string lineSources_[kLineCount] =
    {
        "…こちら救助隊。{hour}時にヘリが着く",
        "それまで生き延びろ。奴らを倒して金を集めておけ",
        "集めた分だけ、報酬は弾むぞ",
    };
    std::string replyLabel_ = "了解！";

    // 時間
    float charactersPerSecond_ = 16.0f;
    float linePause_ = 0.4f;
    float commaPause_ = 0.12f;
    float periodPause_ = 0.25f;
    float replyDelay_ = 0.3f;
    float leaveTime_ = 0.5f;
    float skipDelay_ = 0.5f;
    float flickerTime_ = 0.35f;

    // 見た目
    float fontSize_ = 30.0f;
    float speakerFontSize_ = 20.0f;
    float offsetY_ = 30.0f;
    float speakerGap_ = 34.0f;
    float lineGap_ = 42.0f;
    float replyGap_ = 60.0f;
    float replyFontSize_ = 26.0f;
    float replyIconSize_ = 40.0f;
    float replyIconGap_ = 10.0f;
    float replyPulseSpeed_ = 3.0f;
    FS::Vector4 replyColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
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
