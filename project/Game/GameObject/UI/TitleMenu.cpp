#include "pch.h"
#include "TitleMenu.h"
#include "UITextStyle.h"
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

    titleText_ = std::make_unique<Text>(engine_, "Title");

    // 保存した値が無いときの初期値
    subtitleText_ = std::make_unique<Text>(engine_, "Title");

    startText_ = std::make_unique<Text>(engine_, "Maru");
    UITextStyle::ApplyMaruShadow(*startText_);
    stageText_ = std::make_unique<Text>(engine_, "Maru");
    stageText_->SetScreenAnchor({ 0.5f, 0.5f });
    stageText_->SetPivot({ 0.5f, 0.5f });
    stageText_->SetPosition({ 0.0f, 112.0f });
    stageText_->SetAlign(TextAlign::Center);
    stageText_->SetFontSize(32.0f);
    stageText_->SetCharacterSpacing(4.0f);
    UITextStyle::ApplyMaruShadow(*stageText_);
    stageText_->SetLayerOrder(31);

    stageLabelText_ = std::make_unique<Text>(engine_, "Maru");
    stageLabelText_->SetText("STAGE");
    stageLabelText_->SetScreenAnchor({ 0.5f, 0.5f });
    stageLabelText_->SetPivot({ 0.5f, 0.5f });
    stageLabelText_->SetPosition({ 0.0f, 80.0f });
    stageLabelText_->SetAlign(TextAlign::Center);
    stageLabelText_->SetFontSize(16.0f);
    stageLabelText_->SetCharacterSpacing(6.0f);
    UITextStyle::ApplyMaruShadow(*stageLabelText_);
    stageLabelText_->SetLayerOrder(31);

    for (std::unique_ptr<Sprite>* icon : { &stageLeftIcon_, &stageRightIcon_ })
    {
        *icon = std::make_unique<Sprite>(engine_);
        (*icon)->SetScreenAnchor({ 0.5f, 0.5f });
        (*icon)->SetAnchorPoint({ 0.5f, 0.5f });
        (*icon)->SetLayerOrder(31);
    }

    padHintIcon_ = std::make_unique<Sprite>(engine_);
    padHintIcon_->SetTexture("controller_xboxseries");
    padHintIcon_->SetScreenAnchor({ 1.0f, 0.0f });
    padHintIcon_->SetAnchorPoint({ 1.0f, 0.5f });
    padHintIcon_->SetLayerOrder(31);

    padHintText_ = std::make_unique<Text>(engine_, "Maru");
    padHintText_->SetText("パッド推奨");
    padHintText_->SetScreenAnchor({ 1.0f, 0.0f });
    padHintText_->SetPivot({ 1.0f, 0.5f });
    padHintText_->SetPosition({ -100.0f, 50.0f });
    padHintText_->SetAlign(TextAlign::Right);
    padHintText_->SetFontSize(22.0f);
    padHintText_->SetCharacterSpacing(3.0f);
    UITextStyle::ApplyMaruShadow(*padHintText_);
    padHintText_->SetLayerOrder(31);

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
    binder_->BindTextElement("StageText", stageText_.get(), false);
    binder_->BindTextElement("StageLabelText", stageLabelText_.get());
    binder_->Bind("StageLabelOpacity", &stageLabelOpacity_, 0.6f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("StageArrowSize", &stageArrowSize_, 36.0f, 0.5f, 1.0f, 300.0f);
    binder_->Bind("StageArrowGap", &stageArrowGap_, 28.0f, 0.5f, -100.0f, 300.0f);
    binder_->Bind("StageArrowBobDistance", &stageArrowBobDistance_, 5.0f, 0.1f, 0.0f, 100.0f);
    binder_->Bind("StageArrowBobSpeed", &stageArrowBobSpeed_, 3.0f, 0.05f, 0.0f, 20.0f);

    // ステージごとの名前と色は ImGui で決める
    const std::vector<StageDefinition>& stages = StageCatalog::GetStages();
    for (int stageIndex = 0; stageIndex < static_cast<int>(stages.size()); ++stageIndex)
    {
        const std::string& settingsName = stages[stageIndex].settingsName;
        StageAppearance& appearance = StageCatalog::GetAppearance(stageIndex);
        binder_->BindString("StageName_" + settingsName, stages[stageIndex].displayName,
            [&appearance](const std::string& name) { appearance.name = name; });
        binder_->BindColor("StageColor_" + settingsName, &appearance.color, { 1.0f, 1.0f, 1.0f, 1.0f });
    }
    binder_->BindTextElement("PadHintText", padHintText_.get());
    binder_->Bind("PadHintIconPos", &padHintIcon_->GetPosition(), { -40.0f, 50.0f }, 1.0f);
    binder_->Bind("PadHintIconSize", &padHintIcon_->GetSize(), { 52.0f, 52.0f }, 1.0f);

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
    arrowTimer_ += TimeManager::GetInstance()->GetDeltaTime();

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
        DrawStageSelect(textOpacity);
    }

    padHintIcon_->SetColor(Vector4(1.0f, 1.0f, 1.0f, textOpacity));
    padHintIcon_->Draw();
    padHintText_->SetOpacity(textOpacity);
    padHintText_->Draw();
}

void TitleMenu::DrawStageSelect(float opacity)
{

    stageLabelText_->SetOpacity(opacity * stageLabelOpacity_);
    stageLabelText_->Draw();

    const StageAppearance& appearance = StageCatalog::GetSelectedAppearance();
    stageText_->SetText(appearance.name);
    stageText_->SetColor(appearance.color);
    stageText_->SetOpacity(opacity);
    stageText_->Draw();

    // 最後に触った方のボタンを出し、外へ向かってゆっくり揺らす
    const bool isUsingController = GameInput::IsUsingController();
    stageLeftIcon_->SetTexture(isUsingController ? "dpad_left" : "keyboard_a");
    stageRightIcon_->SetTexture(isUsingController ? "dpad_right" : "keyboard_d");

    const float bob = (std::sin(arrowTimer_ * stageArrowBobSpeed_) * 0.5f + 0.5f) * stageArrowBobDistance_;
    const float halfNameWidth = stageText_->MeasureSize().x * 0.5f;
    const float iconCenterX = halfNameWidth + stageArrowGap_ + stageArrowSize_ * 0.5f + bob;
    const Vector2& namePosition = *stageText_->GetPositionPtr();

    stageLeftIcon_->SetPosition({ namePosition.x - iconCenterX, namePosition.y });
    stageRightIcon_->SetPosition({ namePosition.x + iconCenterX, namePosition.y });
    for (Sprite* icon : { stageLeftIcon_.get(), stageRightIcon_.get() })
    {
        icon->SetSize({ stageArrowSize_, stageArrowSize_ });
        icon->SetColor(Vector4(1.0f, 1.0f, 1.0f, opacity));
        icon->Draw();
    }
}

void TitleMenu::DebugGui()
{
    ImGui::Begin("タイトル");
    binder_->DrawTextElement("TitleText", "タイトル");
    binder_->DrawTextElement("SubtitleText", "サブタイトル");
    binder_->DrawTextElement("StartText", "START GAME");
    if (ImGui::TreeNode("ステージ選択"))
    {
        binder_->DrawTextElement("StageLabelText", "見出しSTAGE");
        binder_->Draw("StageLabelOpacity", "見出しの濃さ");
        binder_->DrawTextElement("StageText", "ステージ名 (中身は選んでいるステージから作る)");
        for (const StageDefinition& stage : StageCatalog::GetStages())
        {
            binder_->Draw("StageName_" + stage.settingsName, stage.settingsName + " の名前");
            binder_->Draw("StageColor_" + stage.settingsName, stage.settingsName + " の名前の色");
        }
        binder_->Draw("StageArrowSize", "左右のボタンの絵の大きさ");
        binder_->Draw("StageArrowGap", "ステージ名から絵までの間");
        binder_->Draw("StageArrowBobDistance", "絵を揺らす幅");
        binder_->Draw("StageArrowBobSpeed", "絵を揺らす速さ");
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("パッド推奨"))
    {
        binder_->DrawTextElement("PadHintText", "文字");
        binder_->Draw("PadHintIconPos", "パッドの絵の位置 (右上から)");
        binder_->Draw("PadHintIconSize", "パッドの絵の大きさ");
        ImGui::TreePop();
    }
    binder_->Draw("FadeInTime", "黒い画面が明けるまで (秒)");
    binder_->Draw("TextFadeInDelay", "文字が出始めるまで (秒)");
    binder_->Draw("TextFadeTime", "文字が出きるまで (秒)");
    binder_->Draw("FadeOutTime", "決定後に文字が消えるまで (秒)");
    binder_->Draw("BlinkSpeed", "点滅の速さ");
    binder_->Draw("BlinkMinOpacity", "点滅で一番薄いとき");
    ImGui::End();
}
