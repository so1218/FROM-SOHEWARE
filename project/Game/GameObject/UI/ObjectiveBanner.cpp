#include "pch.h"
#include "ObjectiveBanner.h"
#include "UITextStyle.h"
#include "GameInput.h"
#include "TimeManager.h"
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

ObjectiveBanner::ObjectiveBanner(Engine* engine)
    : engine_(engine)
{
    binder_ = std::make_unique<PropertyBinder>(engine_, "ObjectiveBanner");

    blackout_ = std::make_unique<Sprite>(engine_);
    blackout_->SetScreenAnchor({ 0.0f, 0.0f });
    blackout_->SetAnchorPoint({ 0.0f, 0.0f });
    blackout_->SetLayerOrder(49);

    // 画面の中央を横切る帯と、見出しの下の線
    band_ = std::make_unique<Sprite>(engine_);
    band_->SetScreenAnchor({ 0.0f, 0.5f });
    band_->SetAnchorPoint({ 0.0f, 0.5f });
    band_->SetLayerOrder(44);
    line_ = std::make_unique<Sprite>(engine_);
    line_->SetScreenAnchor({ 0.5f, 0.5f });
    line_->SetAnchorPoint({ 0.5f, 0.5f });
    line_->SetLayerOrder(45);

    titleText_ = std::make_unique<Text>(engine_, "Maru");
    UITextStyle::ApplyMaruShadow(*titleText_);
    subText_ = std::make_unique<Text>(engine_, "Maru");
    UITextStyle::ApplyMaruShadow(*subText_);
    subText_->SetText("敵を倒して金を手に入れろ");
    timeText_ = std::make_unique<Text>(engine_, "Maru");
    UITextStyle::ApplyMaruShadow(*timeText_);
    for (Text* text : { titleText_.get(), subText_.get(), timeText_.get() })
    {
        text->SetAnchor(UIAnchor::Center);
        text->SetAlign(TextAlign::Center);
        text->SetLayerOrder(46);
    }
}

void ObjectiveBanner::Initialize()
{
    binder_->Bind("FadeOutTime", &fadeOutTime_, 0.8f, 0.01f, 0.01f, 5.0f);
    binder_->Bind("HoldTime", &holdTime_, 0.5f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("FadeInTime", &fadeInTime_, 0.8f, 0.01f, 0.01f, 5.0f);

    binder_->Bind("ShowTime", &showTime_, 3.5f, 0.01f, 0.1f, 20.0f);
    binder_->Bind("RepeatShowTime", &repeatShowTime_, 1.8f, 0.01f, 0.1f, 20.0f);
    binder_->Bind("EnterTime", &enterTime_, 0.6f, 0.01f, 0.01f, 5.0f);
    binder_->Bind("LeaveTime", &leaveTime_, 0.5f, 0.01f, 0.01f, 5.0f);
    binder_->Bind("SkipDelay", &skipDelay_, 0.5f, 0.01f, 0.0f, 5.0f);

    binder_->BindFontName("Font", "Maru", [this](const std::string& fontName)
        {
            for (Text* text : { titleText_.get(), subText_.get(), timeText_.get() })
            {
                text->SetFont(fontName);
            }
        });
    binder_->BindColor("BandColor", &bandColor_, { 0.0f, 0.0f, 0.0f, 0.55f });
    binder_->Bind("BandHeight", &bandHeight_, 230.0f, 1.0f, 0.0f, 720.0f);
    binder_->Bind("LineWidth", &lineWidth_, 520.0f, 1.0f, 0.0f, 1280.0f);
    binder_->Bind("LineThickness", &lineThickness_, 2.0f, 0.1f, 0.0f, 20.0f);
    binder_->BindColor("LineColor", &lineColor_, { 0.85f, 0.15f, 0.1f, 0.9f });
    binder_->Bind("TitleFontSize", &titleFontSize_, 64.0f, 1.0f, 8.0f, 400.0f);
    binder_->Bind("TitleOffsetY", &titleOffsetY_, -40.0f, 1.0f);
    binder_->Bind("TitleStartScale", &titleStartScale_, 1.3f, 0.01f, 0.1f, 5.0f);
    binder_->BindColor("TitleColor", &titleColor_, { 0.95f, 0.9f, 0.8f, 1.0f });
    binder_->Bind("SubFontSize", &subFontSize_, 30.0f, 0.5f, 4.0f, 200.0f);
    binder_->Bind("SubOffsetY", &subOffsetY_, 35.0f, 1.0f);
    binder_->BindColor("SubColor", &subColor_, { 1.0f, 1.0f, 1.0f, 0.9f });
    binder_->Bind("TimeFontSize", &timeFontSize_, 22.0f, 0.5f, 4.0f, 200.0f);
    binder_->Bind("TimeOffsetY", &timeOffsetY_, 80.0f, 1.0f);
    binder_->BindColor("TimeColor", &timeColor_, { 1.0f, 1.0f, 1.0f, 0.6f });
    binder_->Bind("SubDelay", &subDelay_, 0.25f, 0.01f, 0.0f, 5.0f);

    phase_ = Phase::Hidden;
}

void ObjectiveBanner::Begin(bool withFade, bool isFirstTime, int endHour, float playSeconds)
{
    titleText_->SetText(std::to_string(endHour) + "時まで生き残れ");
    // ゲームの中の時刻は見出しで言っているので、ここでは現実の長さを出す
    const int totalSeconds = static_cast<int>(std::round(playSeconds));
    const int minutes = totalSeconds / 60;
    const int seconds = totalSeconds % 60;
    std::string limitText = "制限時間 ";
    if (minutes > 0)
    {
        limitText += std::to_string(minutes) + "分";
    }
    if (seconds > 0 || minutes == 0)
    {
        limitText += std::to_string(seconds) + "秒";
    }
    timeText_->SetText(limitText);
    showDuration_ = isFirstTime ? showTime_ : repeatShowTime_;
    isResetRequested_ = false;
    isFinished_ = false;
    ChangePhase(withFade ? Phase::FadeOut : Phase::Show);
}

void ObjectiveBanner::Cancel()
{
    isResetRequested_ = false;
    isFinished_ = false;
    ChangePhase(Phase::Hidden);
}

bool ObjectiveBanner::TakeResetRequest()
{
    const bool isRequested = isResetRequested_;
    isResetRequested_ = false;
    return isRequested;
}

bool ObjectiveBanner::TakeFinished()
{
    const bool isFinished = isFinished_;
    isFinished_ = false;
    return isFinished;
}

void ObjectiveBanner::ChangePhase(Phase phase)
{
    phase_ = phase;
    phaseTimer_ = 0.0f;
}

void ObjectiveBanner::Update()
{
    if (phase_ == Phase::Hidden)
    {
        return;
    }

    phaseTimer_ += TimeManager::GetInstance()->GetUnscaledDeltaTime();

    switch (phase_)
    {
    case Phase::FadeOut:
        if (phaseTimer_ >= fadeOutTime_)
        {
            // 真っ暗になったので、ここで最初の位置に戻してもらう
            isResetRequested_ = true;
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
        // 明けきる前から目的を出し始める
        if (phaseTimer_ >= fadeInTime_ * 0.5f)
        {
            const float remainingFade = fadeInTime_ * 0.5f;
            ChangePhase(Phase::Show);
            phaseTimer_ = -remainingFade;
        }
        break;

    case Phase::Show:
        // 読み終えた人は決定で飛ばせる
        if (phaseTimer_ >= showDuration_
            || (phaseTimer_ >= skipDelay_ && GameInput::IsTriggered(GameAction::Confirm)))
        {
            ChangePhase(Phase::Leave);
        }
        break;

    case Phase::Leave:
        if (phaseTimer_ >= leaveTime_)
        {
            isFinished_ = true;
            ChangePhase(Phase::Hidden);
        }
        break;

    case Phase::Hidden:
        break;
    }
}

void ObjectiveBanner::Draw()
{
    if (phase_ == Phase::Hidden)
    {
        return;
    }

    const Vector2 screenSize = { Engine::GetUIWidth(), Engine::GetUIHeight() };

    float blackoutRatio = 0.0f;
    switch (phase_)
    {
    case Phase::FadeOut:
        blackoutRatio = SmoothStep01(Ratio(phaseTimer_, fadeOutTime_));
        break;
    case Phase::Hold:
        blackoutRatio = 1.0f;
        break;
    case Phase::FadeIn:
        blackoutRatio = 1.0f - SmoothStep01(Ratio(phaseTimer_, fadeInTime_));
        break;
    case Phase::Show:
        // FadeIn の後半は Show の中で明ける
        if (phaseTimer_ < 0.0f)
        {
            blackoutRatio = 1.0f - SmoothStep01(Ratio(fadeInTime_ + phaseTimer_, fadeInTime_));
        }
        break;
    default:
        break;
    }

    if (phase_ == Phase::Show)
    {
        DrawContent(1.0f);
    }
    else if (phase_ == Phase::Leave)
    {
        DrawContent(1.0f - SmoothStep01(Ratio(phaseTimer_, leaveTime_)));
    }

    if (blackoutRatio > 0.0f)
    {
        blackout_->SetSize(screenSize);
        blackout_->SetColor({ 0.0f, 0.0f, 0.0f, blackoutRatio });
        blackout_->Draw();
    }
}

void ObjectiveBanner::DrawContent(float opacity)
{
    // Leave の間は出しきった姿のまま薄くする
    const float time = phase_ == Phase::Leave ? showDuration_ : std::max(phaseTimer_, 0.0f);
    const float enterRatio = EaseOutCubic(Ratio(time, enterTime_));
    const Vector2 screenSize = { Engine::GetUIWidth(), Engine::GetUIHeight() };

    // 帯は縦に開く
    Vector4 bandColor = bandColor_;
    bandColor.w *= opacity * SmoothStep01(Ratio(time, enterTime_));
    band_->SetSize({ screenSize.x, bandHeight_ * enterRatio });
    band_->SetColor(bandColor);
    band_->Draw();

    // 見出しは大きく現れて沈み込む
    const float titleScale = titleStartScale_ + (1.0f - titleStartScale_) * enterRatio;
    titleText_->SetFontSize(titleFontSize_ * titleScale);
    titleText_->SetPosition({ 0.0f, titleOffsetY_ });
    titleText_->SetColor(titleColor_);
    titleText_->SetOpacity(opacity * SmoothStep01(Ratio(time, enterTime_ * 0.6f)));
    titleText_->Draw();

    // 線は中央から左右に伸びる
    const float subRatio = Ratio(time - subDelay_, enterTime_);
    Vector4 lineColor = lineColor_;
    lineColor.w *= opacity;
    line_->SetPosition({ 0.0f, (titleOffsetY_ + subOffsetY_) * 0.5f });
    line_->SetSize({ lineWidth_ * EaseOutCubic(subRatio), lineThickness_ });
    line_->SetColor(lineColor);
    line_->Draw();

    const float subOpacity = opacity * SmoothStep01(subRatio);
    const float subRise = 12.0f * (1.0f - EaseOutCubic(subRatio));
    subText_->SetFontSize(subFontSize_);
    subText_->SetPosition({ 0.0f, subOffsetY_ + subRise });
    subText_->SetColor(subColor_);
    subText_->SetOpacity(subOpacity);
    subText_->Draw();

    timeText_->SetFontSize(timeFontSize_);
    timeText_->SetPosition({ 0.0f, timeOffsetY_ + subRise });
    timeText_->SetColor(timeColor_);
    timeText_->SetOpacity(subOpacity);
    timeText_->Draw();
}

void ObjectiveBanner::DebugGui()
{
    ImGui::Begin("目的の表示");

    if (ImGui::Button("表示してみる"))
    {
        Begin(false, true, 20, 180.0f);
    }

    if (ImGui::TreeNode("暗転"))
    {
        binder_->Draw("FadeOutTime", "暗くなるまで (秒)");
        binder_->Draw("HoldTime", "真っ暗なまま待つ時間 (秒)");
        binder_->Draw("FadeInTime", "明けるまで (秒)");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("時間"))
    {
        binder_->Draw("ShowTime", "出しておく時間 (秒)");
        binder_->Draw("RepeatShowTime", "2回目からの出しておく時間 (秒)");
        binder_->Draw("EnterTime", "出きるまで (秒)");
        binder_->Draw("SubDelay", "見出しから下の文字が出るまでの遅れ (秒)");
        binder_->Draw("LeaveTime", "消えるまで (秒)");
        binder_->Draw("SkipDelay", "決定で飛ばせるようになるまで (秒)");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("見た目"))
    {
        binder_->Draw("Font", "フォント");
        binder_->Draw("BandColor", "帯の色");
        binder_->Draw("BandHeight", "帯の高さ");
        binder_->Draw("LineWidth", "線の長さ");
        binder_->Draw("LineThickness", "線の太さ");
        binder_->Draw("LineColor", "線の色");
        binder_->Draw("TitleFontSize", "見出しの大きさ");
        binder_->Draw("TitleOffsetY", "見出しの位置 (画面中央から)");
        binder_->Draw("TitleStartScale", "見出しの出始めの大きさ (倍)");
        binder_->Draw("TitleColor", "見出しの色");
        binder_->Draw("SubFontSize", "目的の大きさ");
        binder_->Draw("SubOffsetY", "目的の位置 (画面中央から)");
        binder_->Draw("SubColor", "目的の色");
        binder_->Draw("TimeFontSize", "時刻の大きさ");
        binder_->Draw("TimeOffsetY", "時刻の位置 (画面中央から)");
        binder_->Draw("TimeColor", "時刻の色");
        ImGui::TreePop();
    }

    ImGui::End();
}
