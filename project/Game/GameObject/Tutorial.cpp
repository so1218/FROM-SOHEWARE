#include "pch.h"
#include "Tutorial.h"
#include "Player.h"
#include "PlayerStateSprint.h"
#include "PlayerStateDodge.h"
#include "PlayerStateJump.h"
#include "PlayerStateAiming.h"
#include "TextureLoader.h"
#include "TimeManager.h"
#include "Input.h"

using namespace FS;

namespace
{
    // Step と同じ並び。画像の名前は Assets/Textures 以下の PNG のファイル名
    const char* const kDefaultStepContents[][3] =
    {
        { "移動", "keyboard_w,keyboard_a,keyboard_s,keyboard_d", "xbox_stick_l" },
        { "ダッシュ", "keyboard_shift", "xbox_stick_l_press" },
        { "回避", "keyboard_ctrl", "xbox_button_b" },
        { "ジャンプ", "keyboard_space", "xbox_button_a" },
        { "エイム", "keyboard_k", "xbox_lt" },
        { "エイム中に射撃", "keyboard_j", "xbox_rt" },
        { "チュートリアル完了", "", "" },
    };

    std::vector<std::string> SplitByComma(const std::string& text)
    {
        std::vector<std::string> parts;
        std::istringstream textStream(text);
        std::string part;
        while (std::getline(textStream, part, ','))
        {
            if (!part.empty())
            {
                parts.push_back(part);
            }
        }
        return parts;
    }
}

Tutorial::Tutorial(Engine* engine, Player* player)
    : engine_(engine), player_(player)
{
    binder_ = std::make_unique<PropertyBinder>(engine_, "Tutorial");
    panel_ = std::make_unique<Sprite>(engine_);
    messageText_ = std::make_unique<Text>(engine_);

    for (const auto& defaultContent : kDefaultStepContents)
    {
        stepContents_.push_back({ defaultContent[0], defaultContent[1], defaultContent[2] });
    }
}

void Tutorial::Initialize()
{
    binder_->Bind("IsEnabled", &isEnabled_, true);
    binder_->Bind("StartDelay", &startDelay_, 1.0f, 0.01f, 0.0f, 10.0f);
    binder_->Bind("FadeTime", &fadeTime_, 0.3f, 0.01f, 0.01f, 3.0f);
    binder_->Bind("ClearedWaitTime", &clearedWaitTime_, 0.6f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("RequiredHoldTime", &requiredHoldTime_, 1.0f, 0.01f, 0.0f, 10.0f);
    binder_->Bind("CompleteShowTime", &completeShowTime_, 2.0f, 0.01f, 0.0f, 10.0f);

    binder_->Bind("PanelOffset", &panelOffset_, { 0.0f, 200.0f }, 1.0f);
    binder_->Bind("PanelSize", &panelSize_, { 1280.0f, 130.0f }, 1.0f, 1.0f, 4000.0f);
    binder_->BindColor("PanelColor", &panelColor_, { 0.0f, 0.0f, 0.0f, 0.7f });
    binder_->Bind("PanelEdgeFade", &panelEdgeFade_, 0.35f, 0.01f, 0.0f, 0.5f,
        [this]() { CreatePanelTexture(); });

    binder_->BindColor("TextColor", &textColor_, { 1.0f, 1.0f, 1.0f, 1.0f });
    binder_->Bind("FontSize", &fontSize_, 28.0f, 0.5f, 4.0f, 200.0f);
    binder_->Bind("TextOffsetY", &textOffsetY_, -24.0f, 0.5f);
    binder_->BindFontName("Font", messageText_->GetFontName(),
        [this](const std::string& fontName) { messageText_->SetFont(fontName); });

    binder_->Bind("IconSize", &iconSize_, 48.0f, 0.5f, 4.0f, 256.0f);
    binder_->Bind("IconSpacing", &iconSpacing_, 8.0f, 0.5f, 0.0f, 100.0f);
    binder_->Bind("IconOffsetY", &iconOffsetY_, 26.0f, 0.5f);

    for (size_t stepIndex = 0; stepIndex < stepContents_.size(); ++stepIndex)
    {
        StepContent& content = stepContents_[stepIndex];
        const std::string prefix = std::format("Step{}_", stepIndex);

        binder_->BindString(prefix + "Message", content.message,
            [this, &content](const std::string& message) { content.message = message; StartStep(currentStep_); });
    }

    panel_->SetScreenAnchor({ 0.5f, 0.5f });
    panel_->SetAnchorPoint({ 0.5f, 0.5f });
    panel_->SetLayerOrder(20);
    CreatePanelTexture();

    messageText_->SetAnchor(UIAnchor::Center);
    messageText_->SetAlign(TextAlign::Center);
    messageText_->SetLayerOrder(21);

    isUsingController_ = Input::GetInstance().IsControllerConnected(0);
    StartStep(isEnabled_ ? Step::Move : Step::Finished);
}

void Tutorial::CreatePanelTexture()
{
    // 横 1 列の白い画像を作り、左右の端だけ透明にしていく。色は panelColor_ で付ける
    constexpr int kTextureWidth = 256;
    DirectX::ScratchImage gradientImage;
    if (FAILED(gradientImage.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, kTextureWidth, 1, 1, 1)))
    {
        return;
    }

    uint8_t* pixels = gradientImage.GetPixels();
    for (int pixelX = 0; pixelX < kTextureWidth; ++pixelX)
    {
        const float distanceFromEdge = std::min(pixelX + 0.5f, kTextureWidth - pixelX - 0.5f) / kTextureWidth;
        float edgeAlpha = 1.0f;
        if (panelEdgeFade_ > 0.0f)
        {
            edgeAlpha = std::clamp(distanceFromEdge / panelEdgeFade_, 0.0f, 1.0f);
            edgeAlpha = edgeAlpha * edgeAlpha * (3.0f - 2.0f * edgeAlpha);
        }

        pixels[pixelX * 4 + 0] = 255;
        pixels[pixelX * 4 + 1] = 255;
        pixels[pixelX * 4 + 2] = 255;
        pixels[pixelX * 4 + 3] = static_cast<uint8_t>(edgeAlpha * 255.0f);
    }

    // 同じ名前だと作り直されないので、値を名前に入れる
    const std::string textureKey = std::format("TutorialPanel_{:.3f}", panelEdgeFade_);
    panel_->SetTextureHandle(engine_->GetTextureLoader()->LoadFromImage(textureKey, gradientImage));
}

void Tutorial::StartStep(Step step)
{
    if (step != currentStep_ || phase_ == Phase::Delay)
    {
        phase_ = (step == Step::Move) ? Phase::Delay : Phase::FadeIn;
        phaseTimer_ = 0.0f;
        visibility_ = 0.0f;
        heldTime_ = 0.0f;
        shotCountAtStart_ = player_->GetShotCount();
    }
    currentStep_ = step;

    if (step != Step::Finished)
    {
        messageText_->SetText(stepContents_[static_cast<size_t>(step)].message);
    }
    ApplyIcons();
}

void Tutorial::UpdateInputDevice()
{
    Input& input = Input::GetInstance();

    // 最後に触った方の画像を出す
    static const int kControllerButtons[] =
    {
        Input::ButtonA, Input::ButtonB, Input::ButtonX, Input::ButtonY,
        Input::ButtonLB, Input::ButtonRB, Input::ButtonLT, Input::ButtonRT, Input::ButtonL3, Input::ButtonR3,
    };
    static const BYTE kKeyboardKeys[] =
    {
        DIK_W, DIK_A, DIK_S, DIK_D, DIK_LSHIFT, DIK_LCONTROL, DIK_SPACE, DIK_K, DIK_J,
    };

    bool isControllerTouched = false;
    if (input.IsControllerConnected(0))
    {
        isControllerTouched =
            std::abs(input.GetLeftStickX(0)) > STICK_THRESHOLD || std::abs(input.GetLeftStickY(0)) > STICK_THRESHOLD;
        for (int button : kControllerButtons)
        {
            isControllerTouched = isControllerTouched || input.IsControllerButtonPressed(0, button);
        }
    }

    bool isKeyboardTouched = false;
    for (BYTE key : kKeyboardKeys)
    {
        isKeyboardTouched = isKeyboardTouched || input.IsKeyPressed(key);
    }

    const bool wasUsingController = isUsingController_;
    if (isControllerTouched)
    {
        isUsingController_ = true;
    }
    else if (isKeyboardTouched)
    {
        isUsingController_ = false;
    }

    if (isUsingController_ != wasUsingController)
    {
        ApplyIcons();
    }
}

void Tutorial::ApplyIcons()
{
    iconCount_ = 0;
    if (currentStep_ == Step::Finished)
    {
        return;
    }

    const StepContent& content = stepContents_[static_cast<size_t>(currentStep_)];
    const std::vector<std::string> iconNames =
        SplitByComma(isUsingController_ ? content.controllerIcons : content.keyboardIcons);

    while (iconSprites_.size() < iconNames.size())
    {
        auto iconSprite = std::make_unique<Sprite>(engine_);
        iconSprite->SetScreenAnchor({ 0.5f, 0.5f });
        iconSprite->SetAnchorPoint({ 0.5f, 0.5f });
        iconSprite->SetLayerOrder(21);
        iconSprites_.push_back(std::move(iconSprite));
    }

    for (size_t iconIndex = 0; iconIndex < iconNames.size(); ++iconIndex)
    {
        iconSprites_[iconIndex]->SetTexture(iconNames[iconIndex]);
    }
    iconCount_ = iconNames.size();
}

bool Tutorial::IsStepCleared()
{
    const float deltaTime = TimeManager::GetInstance()->GetDeltaTime();
    const State<Player>* playerState = player_->GetStateMachine()->GetCurrentState();

    switch (currentStep_)
    {
    case Step::Move:
        if (player_->GetMoveDirection().LengthSq() > 0.01f)
        {
            heldTime_ += deltaTime;
        }
        return heldTime_ >= requiredHoldTime_;

    case Step::Sprint:
        if (playerState == PlayerStateSprint::GetInstance())
        {
            heldTime_ += deltaTime;
        }
        return heldTime_ >= requiredHoldTime_;

    case Step::Dodge:
        return playerState == PlayerStateDodge::GetInstance();

    case Step::Jump:
        return playerState == PlayerStateJump::GetInstance();

    case Step::Aim:
        if (playerState == PlayerStateAiming::GetInstance())
        {
            heldTime_ += deltaTime;
        }
        return heldTime_ >= requiredHoldTime_;

    case Step::Shoot:
        return player_->GetShotCount() > shotCountAtStart_;

    case Step::Complete:
        heldTime_ += deltaTime;
        return heldTime_ >= completeShowTime_;

    case Step::Finished:
        return false;
    }
    return false;
}

void Tutorial::Update()
{
    if (currentStep_ == Step::Finished)
    {
        return;
    }

    UpdateInputDevice();

    const float deltaTime = TimeManager::GetInstance()->GetDeltaTime();
    phaseTimer_ += deltaTime;

    switch (phase_)
    {
    case Phase::Delay:
        if (phaseTimer_ >= startDelay_)
        {
            phase_ = Phase::FadeIn;
            phaseTimer_ = 0.0f;
        }
        break;

    case Phase::FadeIn:
        visibility_ = std::min(phaseTimer_ / fadeTime_, 1.0f);
        if (visibility_ >= 1.0f)
        {
            phase_ = Phase::Waiting;
            phaseTimer_ = 0.0f;
        }
        break;

    case Phase::Waiting:
        if (IsStepCleared())
        {
            phase_ = Phase::Cleared;
            phaseTimer_ = 0.0f;
        }
        break;

    case Phase::Cleared:
        if (phaseTimer_ >= clearedWaitTime_)
        {
            phase_ = Phase::FadeOut;
            phaseTimer_ = 0.0f;
        }
        break;

    case Phase::FadeOut:
        visibility_ = 1.0f - std::min(phaseTimer_ / fadeTime_, 1.0f);
        if (visibility_ <= 0.0f)
        {
            StartStep(static_cast<Step>(static_cast<int>(currentStep_) + 1));
        }
        break;
    }
}

void Tutorial::Draw()
{
    if (currentStep_ == Step::Finished || visibility_ <= 0.0f)
    {
        return;
    }

    panel_->SetPosition(panelOffset_);
    panel_->SetSize(panelSize_);
    panel_->SetColor(Vector4(panelColor_.x, panelColor_.y, panelColor_.z, panelColor_.w * visibility_));
    panel_->Draw();

    // 画像がないとき (完了など) は文章を帯の真ん中に出す
    const float textY = (iconCount_ > 0) ? panelOffset_.y + textOffsetY_ : panelOffset_.y;
    messageText_->SetPosition({ panelOffset_.x, textY });
    messageText_->SetFontSize(fontSize_);
    messageText_->SetColor(Vector4(textColor_.x, textColor_.y, textColor_.z, textColor_.w * visibility_));
    messageText_->Draw();

    // 画像は横に並べて、全体を中央にそろえる
    const float rowWidth = iconSize_ * iconCount_ + iconSpacing_ * (iconCount_ > 0 ? iconCount_ - 1 : 0);
    float iconCenterX = panelOffset_.x - rowWidth * 0.5f + iconSize_ * 0.5f;
    for (size_t iconIndex = 0; iconIndex < iconCount_; ++iconIndex)
    {
        Sprite* iconSprite = iconSprites_[iconIndex].get();
        iconSprite->SetPosition({ iconCenterX, panelOffset_.y + iconOffsetY_ });
        iconSprite->SetSize({ iconSize_, iconSize_ });
        iconSprite->SetColor(Vector4(1.0f, 1.0f, 1.0f, visibility_));
        iconSprite->Draw();
        iconCenterX += iconSize_ + iconSpacing_;
    }
}

void Tutorial::DebugDraw()
{
#ifdef ENABLE_IMGUI
    ImGui::Begin("チュートリアル");

    if (currentStep_ == Step::Finished)
    {
        ImGui::Text("終了");
    }
    else
    {
        ImGui::Text("表示中: %s (%s)", stepContents_[static_cast<size_t>(currentStep_)].message.c_str(),
            isUsingController_ ? "コントローラー" : "キーボード");
    }

    if (ImGui::Button("最初から"))
    {
        currentStep_ = Step::Finished;
        StartStep(Step::Move);
    }
    ImGui::SameLine();
    if (ImGui::Button("次へ飛ばす") && currentStep_ != Step::Finished)
    {
        StartStep(static_cast<Step>(static_cast<int>(currentStep_) + 1));
    }

    binder_->Draw("IsEnabled", "シーン開始時に表示する");
    binder_->Draw("StartDelay", "最初に出すまでの時間 (秒)");
    binder_->Draw("FadeTime", "フェードの時間 (秒)");
    binder_->Draw("ClearedWaitTime", "できてから消えるまで (秒)");
    binder_->Draw("RequiredHoldTime", "移動・ダッシュ・エイムを続ける時間 (秒)");
    binder_->Draw("CompleteShowTime", "完了を出しておく時間 (秒)");

    if (ImGui::CollapsingHeader("帯"))
    {
        binder_->Draw("PanelOffset", "位置 (画面中央から)");
        binder_->Draw("PanelSize", "大きさ");
        binder_->Draw("PanelColor", "色");
        binder_->Draw("PanelEdgeFade", "左右が透明になる幅 (割合)");
    }

    if (ImGui::CollapsingHeader("文字"))
    {
        binder_->Draw("TextColor", "色");
        binder_->Draw("FontSize", "大きさ");
        binder_->Draw("TextOffsetY", "帯の中心からの高さ");
        binder_->Draw("Font", "フォント");
    }

    if (ImGui::CollapsingHeader("キー・ボタンの画像"))
    {
        binder_->Draw("IconSize", "大きさ");
        binder_->Draw("IconSpacing", "間隔");
        binder_->Draw("IconOffsetY", "帯の中心からの高さ");
    }

    if (ImGui::CollapsingHeader("説明の内容"))
    {
        for (size_t stepIndex = 0; stepIndex < stepContents_.size(); ++stepIndex)
        {
            binder_->Draw(std::format("Step{}_Message", stepIndex), std::format("{} 番目", stepIndex + 1));
        }
    }

    ImGui::End();
#endif
}
