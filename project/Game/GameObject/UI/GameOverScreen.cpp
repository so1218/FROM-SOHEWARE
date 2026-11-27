#include "pch.h"
#include "GameOverScreen.h"
#include "UITextStyle.h"
#include "GameInput.h"
#include "TimeManager.h"
#include "AudioPlayer.h"
#include "PostEffectManager.h"
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

GameOverScreen::GameOverScreen(Engine* engine)
    : engine_(engine)
{
    binder_ = std::make_unique<PropertyBinder>(engine_, "GameOverScreen");

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

    deathText_ = std::make_unique<Text>(engine_, "Title");
    echoText_ = std::make_unique<Text>(engine_, "Title");
    for (Text* text : { deathText_.get(), echoText_.get() })
    {
        text->SetText("Death");
        text->SetAnchor(UIAnchor::Center);
        text->SetAlign(TextAlign::Center);
    }
    echoText_->SetLayerOrder(46);
    deathText_->SetLayerOrder(47);

    keyIcon_ = std::make_unique<Sprite>(engine_);
    keyIcon_->SetTexture("keyboard_space");
    buttonIcon_ = std::make_unique<Sprite>(engine_);
    buttonIcon_->SetTexture("xbox_button_a");
    for (Sprite* icon : { keyIcon_.get(), buttonIcon_.get() })
    {
        icon->SetScreenAnchor({ 0.5f, 0.5f });
        icon->SetAnchorPoint({ 0.0f, 0.5f });
        icon->SetLayerOrder(48);
    }

    separatorText_ = std::make_unique<Text>(engine_, "Maru");
    UITextStyle::ApplyMaruShadow(*separatorText_);
    separatorText_->SetText("/");
    promptText_ = std::make_unique<Text>(engine_, "Maru");
    UITextStyle::ApplyMaruShadow(*promptText_);
    promptText_->SetText("再開");
    for (Text* text : { separatorText_.get(), promptText_.get() })
    {
        text->SetScreenAnchor({ 0.5f, 0.5f });
        text->SetPivot({ 0.0f, 0.5f });
        text->SetLayerOrder(48);
    }
}

void GameOverScreen::Initialize()
{
    binder_->Bind("SlowMotionScale", &slowMotionScale_, 0.3f, 0.01f, 0.05f, 1.0f);
    binder_->Bind("SlowMotionTime", &slowMotionTime_, 1.2f, 0.01f, 0.0f, 5.0f);
    binder_->BindColor("DimColor", &dimColor_, { 0.0f, 0.0f, 0.0f, 0.55f });
    binder_->Bind("DimTime", &dimTime_, 1.5f, 0.01f, 0.01f, 5.0f);
    binder_->Bind("VignetteAmount", &vignetteAmount_, 0.8f, 0.01f, 0.0f, 1.0f);
    binder_->BindColor("VignetteColor", &vignetteColor_, { 0.12f, 0.0f, 0.0f });

    binder_->Bind("KanjiDelay", &deathTextDelay_, 1.8f, 0.01f, 0.0f, 10.0f);
    binder_->BindFontName("KanjiFont", "Title", [this](const std::string& fontName)
        {
            deathText_->SetFont(fontName);
            echoText_->SetFont(fontName);
        });
    binder_->Bind("KanjiFontSize", &deathTextFontSize_, 150.0f, 1.0f, 8.0f, 1000.0f);
    binder_->Bind("KanjiOffsetY", &deathTextOffsetY_, -20.0f, 1.0f);
    binder_->BindColor("KanjiColor", &deathTextColor_, { 0.72f, 0.04f, 0.03f, 1.0f });
    binder_->BindColor("KanjiGlowColor", &deathTextGlowColor_, { 0.45f, 0.0f, 0.0f, 0.8f });
    binder_->Bind("KanjiGlowSoftness", &deathTextGlowSoftness_, 12.0f, 0.1f, 0.0f, 50.0f);
    binder_->Bind("KanjiStartScale", &deathTextStartScale_, 1.35f, 0.01f, 0.1f, 5.0f);
    binder_->Bind("KanjiSettleTime", &deathTextSettleTime_, 0.6f, 0.01f, 0.01f, 5.0f);
    binder_->Bind("KanjiFadeTime", &deathTextFadeTime_, 0.3f, 0.01f, 0.01f, 5.0f);
    binder_->Bind("KanjiDrift", &deathTextDrift_, 0.04f, 0.001f, 0.0f, 1.0f);
    binder_->Bind("KanjiDriftTime", &deathTextDriftTime_, 6.0f, 0.1f, 0.1f, 60.0f);
    binder_->Bind("EchoScale", &echoScale_, 1.9f, 0.01f, 1.0f, 5.0f);
    binder_->Bind("EchoTime", &echoTime_, 1.1f, 0.01f, 0.01f, 5.0f);
    binder_->Bind("EchoOpacity", &echoOpacity_, 0.45f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("KanjiSoundVolume", &deathTextSoundVolume_, 0.7f, 0.01f, 0.0f, 2.0f);


    binder_->Bind("PromptDelay", &promptDelay_, 1.4f, 0.01f, 0.0f, 10.0f);
    binder_->Bind("PromptFadeTime", &promptFadeTime_, 0.5f, 0.01f, 0.01f, 5.0f);
    binder_->Bind("PromptOffsetY", &promptOffsetY_, 210.0f, 1.0f);
    binder_->Bind("PromptFontSize", &promptFontSize_, 24.0f, 0.5f, 4.0f, 200.0f);
    binder_->Bind("PromptIconSize", &promptIconSize_, 34.0f, 0.5f, 4.0f, 200.0f);
    binder_->Bind("PromptSpacing", &promptSpacing_, 8.0f, 0.5f, 0.0f, 100.0f);
    binder_->BindColor("PromptColor", &promptColor_, { 1.0f, 1.0f, 1.0f, 0.85f });

    binder_->Bind("FadeOutTime", &fadeOutTime_, 0.9f, 0.01f, 0.01f, 5.0f);
    binder_->Bind("HoldTime", &holdTime_, 0.5f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("FadeInTime", &fadeInTime_, 1.0f, 0.01f, 0.01f, 5.0f);

    phase_ = Phase::Hidden;
    isRestartRequested_ = false;
}

void GameOverScreen::Begin()
{
    if (phase_ != Phase::Hidden)
    {
        return;
    }

    deathTimer_ = 0.0f;
    revealTimer_ = 0.0f;
    isRestartRequested_ = false;

    TimeManager* timeManager = TimeManager::GetInstance();
    timeScaleBeforeDeath_ = timeManager->GetTimeScale();
    timeManager->SetTimeScale(timeScaleBeforeDeath_ * slowMotionScale_);
    isSlowMotionActive_ = true;

    ChangePhase(Phase::Dying);
}

void GameOverScreen::Cancel()
{
    if (isSlowMotionActive_)
    {
        TimeManager::GetInstance()->SetTimeScale(timeScaleBeforeDeath_);
        isSlowMotionActive_ = false;
    }
    isRestartRequested_ = false;
    ChangePhase(Phase::Hidden);
}

bool GameOverScreen::TakeRestartRequest()
{
    const bool isRequested = isRestartRequested_;
    isRestartRequested_ = false;
    return isRequested;
}

void GameOverScreen::ChangePhase(Phase phase)
{
    phase_ = phase;
    phaseTimer_ = 0.0f;
}

void GameOverScreen::Update()
{
    if (phase_ == Phase::Hidden)
    {
        return;
    }

    const float deltaTime = TimeManager::GetInstance()->GetUnscaledDeltaTime();
    phaseTimer_ += deltaTime;
    deathTimer_ += deltaTime;
    UpdateTimeScale();

    switch (phase_)
    {
    case Phase::Dying:
        if (phaseTimer_ >= deathTextDelay_)
        {
            revealTimer_ = 0.0f;
            PlayIfLoaded("gameOver", deathTextSoundVolume_);
            ChangePhase(Phase::Revealed);
        }
        break;

    case Phase::Revealed:
        revealTimer_ += deltaTime;
        // 案内が出る前の決定は、倒れた直後の連打で飛ばさないよう受け付けない
        if (phaseTimer_ >= promptDelay_ && GameInput::IsTriggered(GameAction::Confirm))
        {
            UISound::PlayConfirm();
            ChangePhase(Phase::FadeOut);
        }
        break;

    case Phase::FadeOut:
        revealTimer_ += deltaTime;
        if (phaseTimer_ >= fadeOutTime_)
        {
            // 真っ暗になったので、ここで世界を戻してもらう
            isRestartRequested_ = true;
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

    ApplyVignette();
}

void GameOverScreen::UpdateTimeScale()
{
    if (!isSlowMotionActive_)
    {
        return;
    }

    // 倒れた瞬間を一番遅くし、だんだん普通の速さに戻す
    const float ratio = Ratio(deathTimer_, slowMotionTime_);
    const float scale = slowMotionScale_ + (1.0f - slowMotionScale_) * ratio * ratio;
    TimeManager::GetInstance()->SetTimeScale(timeScaleBeforeDeath_ * scale);
    if (ratio >= 1.0f)
    {
        isSlowMotionActive_ = false;
    }
}

void GameOverScreen::ApplyVignette()
{
    // 暗転してからはプレイヤー側の画面の縁に任せる
    if (phase_ != Phase::Dying && phase_ != Phase::Revealed && phase_ != Phase::FadeOut)
    {
        return;
    }

    PostEffectManager* postEffectManager = engine_->GetPostEffectManager();
    if (postEffectManager)
    {
        postEffectManager->SetOverlayVignette(vignetteAmount_ * SmoothStep01(deathTimer_ / std::max(dimTime_, 0.001f)), vignetteColor_, 0.2f, 0.9f);
    }
}

void GameOverScreen::Draw()
{
    if (phase_ == Phase::Hidden)
    {
        return;
    }

    const Vector2 screenSize = { Engine::GetUIWidth(), Engine::GetUIHeight() };

    // 倒れてからだんだん暗くする
    float dimRatio = SmoothStep01(deathTimer_ / std::max(dimTime_, 0.001f));
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

    const bool isDeathTextShown = phase_ == Phase::Revealed || phase_ == Phase::FadeOut;
    if (isDeathTextShown && contentOpacity > 0.0f)
    {
        DrawDeathText(contentOpacity);

        const float promptRatio = phase_ == Phase::Revealed
            ? Ratio(phaseTimer_ - promptDelay_, promptFadeTime_)
            : 1.0f;
        DrawPrompt(promptRatio * contentOpacity);
    }

    if (blackoutRatio > 0.0f)
    {
        blackout_->SetSize(screenSize);
        blackout_->SetColor({ 0.0f, 0.0f, 0.0f, blackoutRatio });
        blackout_->Draw();
    }
}

void GameOverScreen::DrawDeathText(float opacity)
{
    // 大きく現れて沈み込み、そのあとはゆっくり迫ってくる
    const float settleRatio = EaseOutCubic(Ratio(revealTimer_, deathTextSettleTime_));
    const float settleScale = deathTextStartScale_ + (1.0f - deathTextStartScale_) * settleRatio;
    const float driftScale = 1.0f + deathTextDrift_ * Ratio(revealTimer_, deathTextDriftTime_);
    const float deathTextOpacity = opacity * SmoothStep01(Ratio(revealTimer_, deathTextFadeTime_));

    deathText_->SetFontSize(deathTextFontSize_ * settleScale * driftScale);
    deathText_->SetPosition({ 0.0f, deathTextOffsetY_ });
    deathText_->SetColor(deathTextColor_);
    deathText_->SetShadow({ 0.0f, 0.0f }, deathTextGlowColor_, deathTextGlowSoftness_, 0.0f);
    deathText_->SetOpacity(deathTextOpacity);
    deathText_->Draw();

    // 出た瞬間に、同じ字が広がりながら消えて衝撃を見せる
    const float echoRatio = Ratio(revealTimer_, echoTime_);
    if (echoRatio < 1.0f)
    {
        echoText_->SetFontSize(deathTextFontSize_ * (1.0f + (echoScale_ - 1.0f) * EaseOutCubic(echoRatio)));
        echoText_->SetPosition({ 0.0f, deathTextOffsetY_ });
        echoText_->SetColor(deathTextColor_);
        echoText_->SetOpacity(opacity * echoOpacity_ * (1.0f - echoRatio));
        echoText_->Draw();
    }
}

void GameOverScreen::DrawPrompt(float opacity)
{
    if (opacity <= 0.0f)
    {
        return;
    }

    for (Text* text : { separatorText_.get(), promptText_.get() })
    {
        text->SetFontSize(promptFontSize_);
    }

    const float separatorWidth = separatorText_->MeasureSize().x;
    const float labelWidth = promptText_->MeasureSize().x;
    const float totalWidth = promptIconSize_ * 2.0f + separatorWidth + labelWidth + promptSpacing_ * 3.0f;
    float cursorX = -totalWidth * 0.5f;

    const Vector4 iconColor = { 1.0f, 1.0f, 1.0f, promptColor_.w * opacity };
    auto drawIcon = [&](Sprite& icon)
        {
            icon.SetPosition({ cursorX, promptOffsetY_ });
            icon.SetSize({ promptIconSize_, promptIconSize_ });
            icon.SetColor(iconColor);
            icon.Draw();
            cursorX += promptIconSize_ + promptSpacing_;
        };
    auto drawText = [&](Text& text, float width)
        {
            text.SetPosition({ cursorX, promptOffsetY_ });
            text.SetColor(promptColor_);
            text.SetOpacity(opacity);
            text.Draw();
            cursorX += width + promptSpacing_;
        };

    drawIcon(*keyIcon_);
    drawText(*separatorText_, separatorWidth);
    drawIcon(*buttonIcon_);
    drawText(*promptText_, labelWidth);
}

void GameOverScreen::PlayIfLoaded(const std::string& name, float volume)
{
    AudioPlayer& audioPlayer = AudioPlayer::GetInstance();
    if (audioPlayer.HasSound(name))
    {
        // 画面の演出の音なので、隠れているときもこもらせない
        audioPlayer.PlayUI(name, volume);
    }
}

void GameOverScreen::DebugGui()
{
    ImGui::Begin("ゲームオーバー");

    if (ImGui::TreeNode("倒れた直後"))
    {
        binder_->Draw("SlowMotionScale", "スローの速さ (倒れた瞬間)");
        binder_->Draw("SlowMotionTime", "普通の速さに戻るまで (秒)");
        binder_->Draw("DimColor", "画面を暗くする色");
        binder_->Draw("DimTime", "暗くなりきるまで (秒)");
        binder_->Draw("VignetteAmount", "画面の縁の暗さ");
        binder_->Draw("VignetteColor", "画面の縁の色");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Death"))
    {
        binder_->Draw("KanjiDelay", "倒れてから出るまで (秒)");
        binder_->Draw("KanjiFont", "フォント");
        binder_->Draw("KanjiFontSize", "大きさ");
        binder_->Draw("KanjiOffsetY", "位置 (画面中央から)");
        binder_->Draw("KanjiColor", "色");
        binder_->Draw("KanjiGlowColor", "にじみの色");
        binder_->Draw("KanjiGlowSoftness", "にじみの広さ");
        binder_->Draw("KanjiStartScale", "出始めの大きさ (倍)");
        binder_->Draw("KanjiSettleTime", "元の大きさに沈むまで (秒)");
        binder_->Draw("KanjiFadeTime", "濃くなるまで (秒)");
        binder_->Draw("KanjiDrift", "表示中に大きくなる量 (倍)");
        binder_->Draw("KanjiDriftTime", "大きくなりきるまで (秒)");
        binder_->Draw("EchoScale", "広がる残像の大きさ (倍)");
        binder_->Draw("EchoTime", "残像が消えるまで (秒)");
        binder_->Draw("EchoOpacity", "残像の濃さ");
        binder_->Draw("KanjiSoundVolume", "出たときの音 (gameOver) の大きさ");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("再開の案内"))
    {
        binder_->Draw("PromptDelay", "Death が出てから案内が出るまで (秒)");
        binder_->Draw("PromptFadeTime", "濃くなるまで (秒)");
        binder_->Draw("PromptOffsetY", "位置 (画面中央から)");
        binder_->Draw("PromptFontSize", "文字の大きさ");
        binder_->Draw("PromptIconSize", "ボタンの絵の大きさ");
        binder_->Draw("PromptSpacing", "間隔");
        binder_->Draw("PromptColor", "色");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("リスタート"))
    {
        binder_->Draw("FadeOutTime", "暗くなるまで (秒)");
        binder_->Draw("HoldTime", "真っ暗なまま待つ時間 (秒)");
        binder_->Draw("FadeInTime", "明けるまで (秒)");
        ImGui::TreePop();
    }

    ImGui::End();
}
