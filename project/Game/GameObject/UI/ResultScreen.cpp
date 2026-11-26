#include "pch.h"
#include "ResultScreen.h"
#include "GameInput.h"
#include "TimeManager.h"
#include "AudioPlayer.h"
#include "UISound.h"

using namespace FS;

namespace
{
float EaseOutCubic(float t)
{
    const float inverse = 1.0f - std::clamp(t, 0.0f, 1.0f);
    return 1.0f - inverse * inverse * inverse;
}

float SmoothStep01(float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

float Ratio(float time, float duration)
{
    return std::clamp(time / std::max(duration, 0.001f), 0.0f, 1.0f);
}
}

ResultScreen::ResultScreen(Engine* engine)
    : engine_(engine)
{
    binder_ = std::make_unique<PropertyBinder>(engine_, "ResultScreen");

    // 画面全体を覆う
    dimOverlay_ = std::make_unique<Sprite>(engine_);
    blackout_ = std::make_unique<Sprite>(engine_);
    for (Sprite* sprite : { dimOverlay_.get(), blackout_.get() })
    {
        sprite->SetScreenAnchor({ 0.0f, 0.0f });
        sprite->SetAnchorPoint({ 0.0f, 0.0f });
    }
    dimOverlay_->SetLayerOrder(44);
    blackout_->SetLayerOrder(49);

    titleText_ = std::make_unique<Text>(engine_, "Title");
    titleText_->SetText("YOU SURVIVED");
    moneyLabelText_ = std::make_unique<Text>(engine_, "Shippori");
    moneyLabelText_->SetText("所持金");
    moneyText_ = std::make_unique<Text>(engine_, "Title");
    retryText_ = std::make_unique<Text>(engine_, "Shippori");
    retryText_->SetText("もう一度");
    titleChoiceText_ = std::make_unique<Text>(engine_, "Shippori");
    titleChoiceText_->SetText("タイトルへ");
    for (Text* text : { titleText_.get(), moneyLabelText_.get(), moneyText_.get(), retryText_.get(), titleChoiceText_.get() })
    {
        text->SetAnchor(UIAnchor::Center);
        text->SetAlign(TextAlign::Center);
        text->SetLayerOrder(46);
    }
}

void ResultScreen::Initialize()
{
    binder_->BindColor("DimColor", &dimColor_, { 0.0f, 0.0f, 0.0f, 0.7f });
    binder_->Bind("DimTime", &dimTime_, 1.2f, 0.01f, 0.01f, 5.0f);

    binder_->BindFontName("Font", "Shippori", [this](const std::string& fontName)
        {
            for (Text* text : { moneyLabelText_.get(), retryText_.get(), titleChoiceText_.get() })
            {
                text->SetFont(fontName);
            }
        });
    binder_->BindFontName("TitleFont", "Title", [this](const std::string& fontName) { titleText_->SetFont(fontName); });
    binder_->BindFontName("MoneyFont", "Title", [this](const std::string& fontName) { moneyText_->SetFont(fontName); });
    binder_->BindString("TitleContent", "YOU SURVIVED", [this](const std::string& content) { titleText_->SetText(content); });

    binder_->Bind("TitleDelay", &titleDelay_, 0.6f, 0.01f, 0.0f, 10.0f);
    binder_->Bind("TitleFadeTime", &titleFadeTime_, 0.8f, 0.01f, 0.01f, 5.0f);
    binder_->Bind("TitleFontSize", &titleFontSize_, 96.0f, 1.0f, 8.0f, 1000.0f);
    binder_->Bind("TitleOffsetY", &titleOffsetY_, -170.0f, 1.0f);
    binder_->BindColor("TitleColor", &titleColor_, { 0.86f, 0.72f, 0.38f, 1.0f });
    binder_->BindColor("TitleGlowColor", &titleGlowColor_, { 0.45f, 0.3f, 0.05f, 0.7f });
    binder_->Bind("TitleGlowSoftness", &titleGlowSoftness_, 12.0f, 0.1f, 0.0f, 50.0f);
    binder_->Bind("TitleCharSpacing", &titleCharSpacing_, 8.0f, 0.1f, -50.0f, 100.0f);
    binder_->Bind("TitleDrift", &titleDrift_, 0.04f, 0.001f, 0.0f, 1.0f);

    binder_->Bind("MoneyDelay", &moneyDelay_, 1.4f, 0.01f, 0.0f, 10.0f);
    binder_->Bind("MoneyCountTime", &moneyCountTime_, 1.2f, 0.01f, 0.01f, 10.0f);
    binder_->Bind("MoneyLabelFontSize", &moneyLabelFontSize_, 28.0f, 0.5f, 4.0f, 200.0f);
    binder_->Bind("MoneyLabelOffsetY", &moneyLabelOffsetY_, -40.0f, 1.0f);
    binder_->Bind("MoneyFontSize", &moneyFontSize_, 80.0f, 0.5f, 4.0f, 400.0f);
    binder_->Bind("MoneyOffsetY", &moneyOffsetY_, 30.0f, 1.0f);
    binder_->BindColor("MoneyLabelColor", &moneyLabelColor_, { 1.0f, 1.0f, 1.0f, 0.7f });
    binder_->BindColor("MoneyColor", &moneyColor_, { 1.0f, 0.85f, 0.4f, 1.0f });
    binder_->Bind("MoneyCountSoundVolume", &moneyCountSoundVolume_, 0.6f, 0.01f, 0.0f, 2.0f);

    binder_->Bind("ChoiceDelay", &choiceDelay_, 0.6f, 0.01f, 0.0f, 10.0f);
    binder_->Bind("ChoiceFadeTime", &choiceFadeTime_, 0.4f, 0.01f, 0.01f, 5.0f);
    binder_->Bind("ChoiceFontSize", &choiceFontSize_, 30.0f, 0.5f, 4.0f, 200.0f);
    binder_->Bind("ChoiceOffsetY", &choiceOffsetY_, 200.0f, 1.0f);
    binder_->Bind("ChoiceSpacing", &choiceSpacing_, 260.0f, 1.0f, 0.0f, 1000.0f);
    binder_->BindColor("ChoiceSelectedColor", &choiceSelectedColor_, { 1.0f, 1.0f, 1.0f, 1.0f });
    binder_->BindColor("ChoiceUnselectedColor", &choiceUnselectedColor_, { 1.0f, 1.0f, 1.0f, 0.35f });

    binder_->Bind("FadeOutTime", &fadeOutTime_, 0.9f, 0.01f, 0.01f, 5.0f);
    binder_->Bind("HoldTime", &holdTime_, 0.4f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("FadeInTime", &fadeInTime_, 1.0f, 0.01f, 0.01f, 5.0f);

    phase_ = Phase::Hidden;
    request_.reset();
}

void ResultScreen::Begin(int money)
{
    if (phase_ != Phase::Hidden)
    {
        return;
    }

    money_ = money;
    revealTimer_ = 0.0f;
    selectedChoice_ = Choice::Retry;
    request_.reset();
    // 遊んでいる最中に倒していた方向キーで、すぐに選択が動かないようにする
    isHorizontalHeld_ = true;
    ChangePhase(Phase::Reveal);
}

void ResultScreen::Cancel()
{
    request_.reset();
    ChangePhase(Phase::Hidden);
}

std::optional<ResultScreen::Choice> ResultScreen::TakeRequest()
{
    std::optional<Choice> request = request_;
    request_.reset();
    return request;
}

void ResultScreen::ChangePhase(Phase phase)
{
    phase_ = phase;
    phaseTimer_ = 0.0f;
}

void ResultScreen::Update()
{
    if (phase_ == Phase::Hidden)
    {
        return;
    }

    const float deltaTime = TimeManager::GetInstance()->GetUnscaledDeltaTime();
    phaseTimer_ += deltaTime;

    switch (phase_)
    {
    case Phase::Reveal:
    {
        const float previousTimer = revealTimer_;
        revealTimer_ += deltaTime;
        if (previousTimer < moneyDelay_ && revealTimer_ >= moneyDelay_ && money_ > 0)
        {
            PlayIfLoaded("coinBurst", moneyCountSoundVolume_);
        }
        if (revealTimer_ >= moneyDelay_ + moneyCountTime_ + choiceDelay_)
        {
            ChangePhase(Phase::Select);
        }
        break;
    }

    case Phase::Select:
        revealTimer_ += deltaTime;
        UpdateSelection();
        if (GameInput::IsTriggered(GameAction::Confirm))
        {
            UISound::PlayConfirm();
            ChangePhase(Phase::FadeOut);
        }
        break;

    case Phase::FadeOut:
        revealTimer_ += deltaTime;
        if (phaseTimer_ >= fadeOutTime_)
        {
            // 真っ暗になったので、ここで遊び直しかタイトルへ戻してもらう
            request_ = selectedChoice_;
            ChangePhase(Phase::Hold);
        }
        break;

    case Phase::Hold:
        if (phaseTimer_ >= holdTime_)
        {
            ChangePhase(Phase::FadeIn);
        }
        break;

    case Phase::FadeIn:
        if (phaseTimer_ >= fadeInTime_)
        {
            ChangePhase(Phase::Hidden);
        }
        break;

    case Phase::Hidden:
        break;
    }
}

void ResultScreen::UpdateSelection()
{
    const float horizontal = GameInput::GetMenuHorizontal();
    const bool isHeld = std::abs(horizontal) >= 0.5f;
    const bool isTriggered = isHeld && !isHorizontalHeld_;
    isHorizontalHeld_ = isHeld;
    if (!isTriggered)
    {
        return;
    }

    const Choice nextChoice = horizontal < 0.0f ? Choice::Retry : Choice::Title;
    if (nextChoice != selectedChoice_)
    {
        selectedChoice_ = nextChoice;
        UISound::PlayCursor();
    }
}

void ResultScreen::Draw()
{
    if (phase_ == Phase::Hidden)
    {
        return;
    }

    const Vector2 screenSize = { Engine::GetUIWidth(), Engine::GetUIHeight() };

    float dimRatio = SmoothStep01(Ratio(revealTimer_, dimTime_));
    float blackoutRatio = 0.0f;
    float contentOpacity = 1.0f;
    switch (phase_)
    {
    case Phase::FadeOut:
        blackoutRatio = SmoothStep01(Ratio(phaseTimer_, fadeOutTime_));
        contentOpacity = 1.0f - blackoutRatio;
        break;
    case Phase::Hold:
        blackoutRatio = 1.0f;
        contentOpacity = 0.0f;
        break;
    case Phase::FadeIn:
        blackoutRatio = 1.0f - SmoothStep01(Ratio(phaseTimer_, fadeInTime_));
        contentOpacity = 0.0f;
        dimRatio = 0.0f;
        break;
    default:
        break;
    }

    if (dimRatio > 0.0f)
    {
        Vector4 dimColor = dimColor_;
        dimColor.w *= dimRatio;
        dimOverlay_->SetSize(screenSize);
        dimOverlay_->SetColor(dimColor);
        dimOverlay_->Draw();
    }

    if (contentOpacity > 0.0f)
    {
        DrawContent(contentOpacity);
    }

    if (blackoutRatio > 0.0f)
    {
        blackout_->SetSize(screenSize);
        blackout_->SetColor({ 0.0f, 0.0f, 0.0f, blackoutRatio });
        blackout_->Draw();
    }
}

void ResultScreen::DrawContent(float opacity)
{
    // 見出しはゆっくり浮かび上がり、表示している間もわずかに迫ってくる
    const float titleTime = revealTimer_ - titleDelay_;
    const float titleRatio = Ratio(titleTime, titleFadeTime_);
    if (titleRatio > 0.0f)
    {
        const float driftScale = 1.0f + titleDrift_ * Ratio(titleTime, 6.0f);
        titleText_->SetFontSize(titleFontSize_ * driftScale);
        titleText_->SetCharacterSpacing(titleCharSpacing_);
        titleText_->SetPosition({ 0.0f, titleOffsetY_ + 20.0f * (1.0f - EaseOutCubic(titleRatio)) });
        titleText_->SetColor(titleColor_);
        titleText_->SetShadow({ 0.0f, 0.0f }, titleGlowColor_, titleGlowSoftness_, 0.0f);
        titleText_->SetOpacity(opacity * SmoothStep01(titleRatio));
        titleText_->Draw();
    }

    // 所持金は 0 から数え上げる
    const float moneyTime = revealTimer_ - moneyDelay_;
    if (moneyTime > 0.0f)
    {
        const float labelOpacity = opacity * SmoothStep01(Ratio(moneyTime, 0.3f));
        moneyLabelText_->SetFontSize(moneyLabelFontSize_);
        moneyLabelText_->SetPosition({ 0.0f, moneyLabelOffsetY_ });
        moneyLabelText_->SetColor(moneyLabelColor_);
        moneyLabelText_->SetOpacity(labelOpacity);
        moneyLabelText_->Draw();

        const float countRatio = EaseOutCubic(Ratio(moneyTime, moneyCountTime_));
        const int shownMoney = static_cast<int>(std::round(static_cast<float>(money_) * countRatio));
        moneyText_->SetText(std::to_string(shownMoney));
        moneyText_->SetFontSize(moneyFontSize_);
        moneyText_->SetPosition({ 0.0f, moneyOffsetY_ });
        moneyText_->SetColor(moneyColor_);
        moneyText_->SetOpacity(labelOpacity);
        moneyText_->Draw();
    }

    const float choiceRatio = phase_ == Phase::Reveal ? 0.0f : Ratio(phase_ == Phase::Select ? phaseTimer_ : choiceFadeTime_, choiceFadeTime_);
    if (choiceRatio > 0.0f)
    {
        DrawChoices(opacity * SmoothStep01(choiceRatio));
    }
}

void ResultScreen::DrawChoices(float opacity)
{
    auto drawChoice = [&](Text& text, Choice choice, float positionX)
        {
            const bool isSelected = selectedChoice_ == choice;
            text.SetFontSize(choiceFontSize_ * (isSelected ? 1.1f : 1.0f));
            text.SetPosition({ positionX, choiceOffsetY_ });
            text.SetColor(isSelected ? choiceSelectedColor_ : choiceUnselectedColor_);
            text.SetOpacity(opacity);
            text.Draw();
        };

    drawChoice(*retryText_, Choice::Retry, -choiceSpacing_ * 0.5f);
    drawChoice(*titleChoiceText_, Choice::Title, choiceSpacing_ * 0.5f);
}

void ResultScreen::PlayIfLoaded(const std::string& name, float volume)
{
    AudioPlayer& audioPlayer = AudioPlayer::GetInstance();
    if (audioPlayer.HasSound(name))
    {
        audioPlayer.Play(name, volume);
    }
}

void ResultScreen::DebugGui()
{
    ImGui::Begin("リザルト");

    if (ImGui::Button("表示してみる (所持金 1234)"))
    {
        Begin(1234);
    }

    if (ImGui::TreeNode("背景"))
    {
        binder_->Draw("DimColor", "画面を暗くする色");
        binder_->Draw("DimTime", "暗くなりきるまで (秒)");
        binder_->Draw("Font", "フォント (見出しと金額以外)");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("見出し"))
    {
        binder_->Draw("TitleContent", "文字");
        binder_->Draw("TitleFont", "フォント");
        binder_->Draw("TitleDelay", "出るまで (秒)");
        binder_->Draw("TitleFadeTime", "濃くなるまで (秒)");
        binder_->Draw("TitleFontSize", "大きさ");
        binder_->Draw("TitleOffsetY", "位置 (画面中央から)");
        binder_->Draw("TitleColor", "色");
        binder_->Draw("TitleGlowColor", "にじみの色");
        binder_->Draw("TitleGlowSoftness", "にじみの広さ");
        binder_->Draw("TitleCharSpacing", "字の間隔");
        binder_->Draw("TitleDrift", "表示中に大きくなる量 (倍)");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("所持金"))
    {
        binder_->Draw("MoneyDelay", "出るまで (秒)");
        binder_->Draw("MoneyCountTime", "数え終わるまで (秒)");
        binder_->Draw("MoneyLabelFontSize", "「所持金」の大きさ");
        binder_->Draw("MoneyLabelOffsetY", "「所持金」の位置 (画面中央から)");
        binder_->Draw("MoneyLabelColor", "「所持金」の色");
        binder_->Draw("MoneyFont", "金額のフォント");
        binder_->Draw("MoneyFontSize", "金額の大きさ");
        binder_->Draw("MoneyOffsetY", "金額の位置 (画面中央から)");
        binder_->Draw("MoneyColor", "金額の色");
        binder_->Draw("MoneyCountSoundVolume", "数え始めの音 (coinBurst) の大きさ");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("選択肢"))
    {
        binder_->Draw("ChoiceDelay", "数え終わってから出るまで (秒)");
        binder_->Draw("ChoiceFadeTime", "濃くなるまで (秒)");
        binder_->Draw("ChoiceFontSize", "大きさ");
        binder_->Draw("ChoiceOffsetY", "位置 (画面中央から)");
        binder_->Draw("ChoiceSpacing", "左右の間隔");
        binder_->Draw("ChoiceSelectedColor", "選んでいるときの色");
        binder_->Draw("ChoiceUnselectedColor", "選んでいないときの色");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("画面の切り替え"))
    {
        binder_->Draw("FadeOutTime", "暗くなるまで (秒)");
        binder_->Draw("HoldTime", "真っ暗なまま待つ時間 (秒)");
        binder_->Draw("FadeInTime", "明けるまで (秒)");
        ImGui::TreePop();
    }

    ImGui::End();
}
