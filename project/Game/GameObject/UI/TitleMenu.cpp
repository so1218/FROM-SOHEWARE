#include "pch.h"
#include "TitleMenu.h"
#include "TimeManager.h"
#include "AudioPlayer.h"
#include "GameInput.h"
#include "StageCatalog.h"
#include "UISound.h"

using namespace FS;

TitleMenu::TitleMenu(Engine* engine)
    : engine_(engine)
{
    binder_ = std::make_unique<PropertyBinder>(engine_, "TitleMenu");

    titleText_ = std::make_unique<Text>(engine_);

    // 保存した値が無いときの初期値
    subtitleText_ = std::make_unique<Text>(engine_);

    startText_ = std::make_unique<Text>(engine_);
    // ステージ名は START GAME のすぐ下に、少し小さく出す
    stageText_ = std::make_unique<Text>(engine_, "Main");
    stageText_->SetScreenAnchor({ 0.5f, 0.5f });
    stageText_->SetPivot({ 0.5f, 0.5f });
    stageText_->SetPosition({ 0.0f, 250.0f });
    stageText_->SetAlign(TextAlign::Center);
    stageText_->SetFontSize(24.0f);
    stageText_->SetCharacterSpacing(4.0f);
    stageText_->SetShadow({ 2.5f, 2.5f }, { 0.0f, 0.0f, 0.0f, 0.6f }, 3.5f, 1.0f);
    stageText_->SetLayerOrder(31);

    blackScreen_ = std::make_unique<Sprite>(engine_);
    blackScreen_->SetScreenAnchor({ 0.0f, 0.0f });
    blackScreen_->SetAnchorPoint({ 0.0f, 0.0f });
    blackScreen_->SetLayerOrder(30);
}

void TitleMenu::Initialize()
{
    binder_->BindTextElement("TitleText", titleText_.get());
    binder_->BindTextElement("SubtitleText", subtitleText_.get());
    binder_->BindTextElement("StartText", startText_.get());
    // 中身は選んでいるステージから毎回作るので、保存しない
    binder_->BindTextElement("StageText", stageText_.get(), false);

    binder_->Bind("FadeInTime", &fadeInTime_, 2.0f, 0.05f, 0.0f, 10.0f);
    binder_->Bind("TextFadeInDelay", &textFadeInDelay_, 1.0f, 0.05f, 0.0f, 10.0f);
    binder_->Bind("TextFadeTime", &textFadeTime_, 1.0f, 0.05f, 0.01f, 10.0f);
    binder_->Bind("FadeOutTime", &fadeOutTime_, 0.6f, 0.05f, 0.01f, 10.0f);
    binder_->Bind("BlinkSpeed", &blinkSpeed_, 2.5f, 0.05f, 0.0f, 20.0f);
    binder_->Bind("BlinkMinOpacity", &blinkMinOpacity_, 0.35f, 0.01f, 0.0f, 1.0f);
}

void TitleMenu::Update()
{
    phaseTimer_ += TimeManager::GetInstance()->GetDeltaTime();

    switch (phase_)
    {
    case Phase::FadeIn:
        if (phaseTimer_ >= textFadeInDelay_ + textFadeTime_ || IsConfirmTriggered())
        {
            phase_ = Phase::Waiting;
            phaseTimer_ = 0.0f;
        }
        break;

    case Phase::Waiting:
        if (IsConfirmTriggered())
        {
            UISound::PlayConfirm();
            phase_ = Phase::Started;
            phaseTimer_ = 0.0f;
            break;
        }
        UpdateStageSelect(TimeManager::GetInstance()->GetDeltaTime());
        break;

    case Phase::Started:
        break;
    }
}

bool TitleMenu::IsConfirmTriggered() const
{
    if (!isInputEnabled_)
    {
        return false;
    }

    return GameInput::IsTriggered(GameAction::Confirm);
}

void TitleMenu::UpdateStageSelect(float deltaTime)
{
    // 一度選び直したら、シーンを作り直すまで受け付けない
    if (!isInputEnabled_ || isStageChangeRequested_ || StageCatalog::GetStages().size() < 2)
    {
        return;
    }

    const float horizontal = GameInput::GetMenuHorizontal();
    const bool isLeft = leftRepeat_.Update((std::max)(-horizontal, 0.0f), deltaTime);
    const bool isRight = rightRepeat_.Update((std::max)(horizontal, 0.0f), deltaTime);
    const int move = (isRight ? 1 : 0) - (isLeft ? 1 : 0);
    if (move == 0)
    {
        return;
    }

    UISound::PlayCursor();
    StageCatalog::Select(StageCatalog::GetSelectedIndex() + move);
    isStageChangeRequested_ = true;
}

bool TitleMenu::TakeStageChangeRequest()
{
    const bool isRequested = isStageChangeRequested_;
    isStageChangeRequested_ = false;
    return isRequested;
}

void TitleMenu::SkipFadeIn()
{
    phase_ = Phase::Waiting;
    phaseTimer_ = 0.0f;
}

void TitleMenu::Skip()
{
    phase_ = Phase::Started;
    phaseTimer_ = fadeOutTime_;
}

void TitleMenu::Restart()
{
    phase_ = Phase::FadeIn;
    phaseTimer_ = 0.0f;
}

void TitleMenu::Draw()
{
    float blackOpacity = 0.0f;
    float textOpacity = 1.0f;
    float startOpacity = 1.0f;

    switch (phase_)
    {
    case Phase::FadeIn:
        blackOpacity = fadeInTime_ > 0.0f ? 1.0f - std::min(phaseTimer_ / fadeInTime_, 1.0f) : 0.0f;
        textOpacity = std::clamp((phaseTimer_ - textFadeInDelay_) / textFadeTime_, 0.0f, 1.0f);
        startOpacity = textOpacity;
        break;

    case Phase::Waiting:
    {
        const float blink = std::cos(phaseTimer_ * blinkSpeed_) * 0.5f + 0.5f;
        startOpacity = blinkMinOpacity_ + (1.0f - blinkMinOpacity_) * blink;
        break;
    }

    case Phase::Started:
        textOpacity = 1.0f - std::min(phaseTimer_ / fadeOutTime_, 1.0f);
        startOpacity = textOpacity;
        break;
    }

    if (blackOpacity > 0.0f)
    {
        blackScreen_->SetSize({ Engine::GetUIWidth(), Engine::GetUIHeight() });
        blackScreen_->SetColor(Vector4(0.0f, 0.0f, 0.0f, blackOpacity));
        blackScreen_->Draw();
    }

    titleText_->SetOpacity(textOpacity);
    titleText_->Draw();
    subtitleText_->SetOpacity(textOpacity);
    subtitleText_->Draw();
    startText_->SetOpacity(startOpacity);
    startText_->Draw();

    // ステージが 2 つ以上あるときだけ、左右で選べることを見せる
    if (StageCatalog::GetStages().size() >= 2)
    {
        stageText_->SetText("<  " + StageCatalog::GetSelectedStage().displayName + "  >");
        stageText_->SetOpacity(textOpacity);
        stageText_->Draw();
    }
}

void TitleMenu::DebugGui()
{
    ImGui::Begin("タイトル");
    binder_->DrawTextElement("TitleText", "タイトル");
    binder_->DrawTextElement("SubtitleText", "サブタイトル");
    binder_->DrawTextElement("StartText", "START GAME");
    binder_->DrawTextElement("StageText", "ステージ (中身は選んでいるステージから作る)");
    binder_->Draw("FadeInTime", "黒い画面が明けるまで (秒)");
    binder_->Draw("TextFadeInDelay", "文字が出始めるまで (秒)");
    binder_->Draw("TextFadeTime", "文字が出きるまで (秒)");
    binder_->Draw("FadeOutTime", "決定後に文字が消えるまで (秒)");
    binder_->Draw("BlinkSpeed", "点滅の速さ");
    binder_->Draw("BlinkMinOpacity", "点滅で一番薄いとき");
    ImGui::End();
}
