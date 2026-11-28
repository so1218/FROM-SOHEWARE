#pragma once
#include <optional>
#include "GameObject.h"
#include "PropertyBinder.h"
#include "Sprite.h"
#include "Text.h"
#include "RadioSubtitle.h"

class ResultScreen : public FS::GameObject
{
public:
    // 結果画面で選べるもの
    enum class Choice
    {
        Retry,
        Title,
    };

    explicit ResultScreen(FS::Engine* engine);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugGui() override;

    // 時間切れになった瞬間に呼ぶ
    void Begin(int money);
    // 途中でやめて何も出していない状態に戻す
    void Cancel();

    // 画面が真っ暗になった瞬間に一度だけ、選ばれたものを返す
    std::optional<Choice> TakeRequest();
    bool IsActive() const { return phase_ != Phase::Hidden; }

private:
    enum class Phase
    {
        Hidden,
        Reveal,
        Select,
        FadeOut,
        Hold,
        FadeIn,
    };

    void ChangePhase(Phase phase);
    void UpdateSelection();
    void DrawContent(float opacity);
    void DrawChoices(float opacity);
    void PlayIfLoaded(const std::string& name, float volume);
    // 金額から段階を決める
    int GetRank() const;
    // 金額から、隊長の一言を選ぶ
    const std::string& SelectComment() const { return comments_[GetRank()]; }
    const std::string& SelectReward() const { return rewards_[GetRank()]; }
    // 報酬が出てからの時間
    float GetRewardTime() const;
    void DrawReward(float opacity);
    // 隊長の一言を言い終えたか
    bool IsCommentFinished() const;

    FS::Engine* engine_ = nullptr;
    std::unique_ptr<FS::PropertyBinder> binder_;

    std::unique_ptr<FS::Sprite> dimOverlay_;
    std::unique_ptr<FS::Sprite> blackout_;
    std::unique_ptr<FS::Text> titleText_;
    std::unique_ptr<FS::Text> moneyLabelText_;
    std::unique_ptr<FS::Text> moneyText_;
    std::unique_ptr<FS::Text> retryText_;
    std::unique_ptr<FS::Text> titleChoiceText_;

    Phase phase_ = Phase::Hidden;
    float phaseTimer_ = 0.0f;
    float revealTimer_ = 0.0f;
    int money_ = 0;
    Choice selectedChoice_ = Choice::Retry;
    std::optional<Choice> request_;
    bool isHorizontalHeld_ = false;

    // 背景
    FS::Vector4 dimColor_ = { 0.0f, 0.0f, 0.0f, 0.88f };
    float dimTime_ = 1.2f;

    // 見出し
    float titleDelay_ = 0.6f;
    // 見出しが出るときの音。ファイルがなければ鳴らさない
    std::string clearSoundName_ = "resultClear";
    float clearSoundVolume_ = 0.8f;
    float titleFadeTime_ = 0.8f;
    float titleFontSize_ = 96.0f;
    float titleOffsetY_ = -200.0f;
    FS::Vector4 titleColor_ = { 0.86f, 0.72f, 0.38f, 1.0f };
    FS::Vector4 titleGlowColor_ = { 0.45f, 0.3f, 0.05f, 0.7f };
    float titleGlowSoftness_ = 12.0f;
    float titleCharSpacing_ = 8.0f;
    // 出てからゆっくり大きくなる量
    float titleDrift_ = 0.04f;

    // 所持金
    float moneyDelay_ = 1.4f;
    float moneyCountTime_ = 1.2f;
    float moneyLabelFontSize_ = 28.0f;
    float moneyLabelOffsetY_ = -85.0f;
    float moneyFontSize_ = 80.0f;
    float moneyOffsetY_ = -15.0f;
    FS::Vector4 moneyLabelColor_ = { 1.0f, 1.0f, 1.0f, 0.7f };
    FS::Vector4 moneyColor_ = { 1.0f, 0.85f, 0.4f, 1.0f };
    float moneyCountSoundVolume_ = 0.6f;

    // 救助隊の隊長からの無線の一言
    static constexpr int kCommentRankCount = 5;
    RadioSubtitle comment_;
    bool hasStartedComment_ = false;
    std::string commentSpeaker_ = "ホーリー隊長";
    // この金額に届いたら次の段階
    int commentThresholds_[kCommentRankCount - 1] = { 150, 500, 1000, 2000 };
    std::string comments_[kCommentRankCount] =
    {
        "…生きて戻っただけ上出来だ。次は稼いでこい",
        "悪くない。だが、奴らはまだ山ほどいたぞ",
        "いい働きだ。上にもそう伝えておく",
        "大した奴だ。うちの部隊に欲しいくらいだな",
        "…化け物か、お前は。今夜の酒はおごらせてもらう",
    };
    float commentDelay_ = 0.5f;
    float commentOffsetY_ = 50.0f;

    // 一言を言い終えたら、段階ごとの報酬を判子を押すように出す
    std::unique_ptr<FS::Text> rewardLabelText_;
    std::unique_ptr<FS::Text> rewardText_;
    bool hasShownReward_ = false;
    std::string rewardLabel_ = "報酬";
    std::string rewards_[kCommentRankCount] =
    {
        "パン一切れ",
        "冷めたスープ",
        "弾薬一箱",
        "部隊の勲章",
        "隊長秘蔵のウイスキー",
    };
    float rewardDelay_ = 0.4f;
    float rewardGap_ = 52.0f;
    float rewardStampTime_ = 0.25f;
    float rewardStampScale_ = 1.6f;
    float rewardFontSize_ = 34.0f;
    float rewardLabelFontSize_ = 22.0f;
    float rewardLabelGap_ = 16.0f;
    FS::Vector4 rewardColor_ = { 1.0f, 0.85f, 0.4f, 1.0f };
    FS::Vector4 rewardLabelColor_ = { 1.0f, 1.0f, 1.0f, 0.7f };
    std::string rewardSoundName_ = "rewardStamp";
    float rewardSoundVolume_ = 0.8f;

    // 選択肢
    float choiceDelay_ = 0.6f;
    float choiceFadeTime_ = 0.4f;
    float choiceFontSize_ = 30.0f;
    float choiceOffsetY_ = 230.0f;
    float choiceSpacing_ = 260.0f;
    FS::Vector4 choiceSelectedColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
    FS::Vector4 choiceUnselectedColor_ = { 1.0f, 1.0f, 1.0f, 0.35f };

    // 画面の切り替え
    float fadeOutTime_ = 0.9f;
    float holdTime_ = 0.4f;
    float fadeInTime_ = 1.0f;
};
