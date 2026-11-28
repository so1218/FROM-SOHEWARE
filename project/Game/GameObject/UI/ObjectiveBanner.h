#pragma once
#include "GameObject.h"
#include "PropertyBinder.h"
#include "Sprite.h"
#include "Text.h"
#include "RadioSubtitle.h"

// 無線の声を字幕で出して目的を伝える
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
    void StartSubtitle(float startDelay);
    void DrawReplyPrompt(float opacity);
    std::string ApplyPlaceholders(const std::string& text) const;

    FS::Engine* engine_ = nullptr;
    std::unique_ptr<FS::PropertyBinder> binder_;

    std::unique_ptr<FS::Sprite> blackout_;
    RadioSubtitle subtitle_;
    std::unique_ptr<FS::Sprite> replyIcon_;
    std::unique_ptr<FS::Text> replyText_;

    Phase phase_ = Phase::Hidden;
    float phaseTimer_ = 0.0f;
    bool isResetRequested_ = false;
    bool isFinished_ = false;

    // Begin で決まる中身
    int endHour_ = 20;
    float playSeconds_ = 0.0f;

    // 暗転
    float fadeOutTime_ = 0.8f;
    float holdTime_ = 0.5f;
    float fadeInTime_ = 0.8f;

    // 文章
    std::string speakerName_ = "無線";
    std::string lineSources_[RadioSubtitle::kMaxLineCount] =
    {
        "…こちら救助隊。{hour}時にヘリが着く",
        "それまで生き延びろ。奴らを倒して金を集めておけ",
        "集めた分だけ、報酬は弾むぞ",
    };
    std::string replyLabel_ = "了解！";

    // 時間
    float replyDelay_ = 0.3f;
    float leaveTime_ = 0.5f;
    float skipDelay_ = 0.5f;

    // 見た目
    float offsetY_ = 30.0f;
    float replyGap_ = 60.0f;
    float replyFontSize_ = 26.0f;
    float replyIconSize_ = 40.0f;
    float replyIconGap_ = 10.0f;
    float replyPulseSpeed_ = 3.0f;
    FS::Vector4 replyColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
};
