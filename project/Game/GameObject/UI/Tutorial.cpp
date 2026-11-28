#include "pch.h"
#include "Tutorial.h"
#include "UITextStyle.h"
#include "Player.h"
#include "EnemyManager.h"
#include "PlayerWeapon.h"
#include "PlayerStateSprint.h"
#include "PlayerStateDodge.h"
#include "PlayerStateJump.h"
#include "PlayerStateAiming.h"
#include "TextureLoader.h"
#include "TimeManager.h"
#include "GameInput.h"
#include "TextureManager.h"
#include "UISound.h"

using namespace FS;

namespace
{
    const char* const kDefaultStepContents[][4] =
    {
        { "Step0", "移動", "keyboard_w,keyboard_a,keyboard_s,keyboard_d", "xbox_stick_l" },
        { "Step1", "ダッシュ", "keyboard_shift", "xbox_stick_l_press" },
        { "Step2", "回避", "keyboard_ctrl", "xbox_button_b" },
        { "Step3", "ジャンプ", "keyboard_space", "xbox_button_a" },
        { "StepCrawl", "這う", "keyboard_c", "xbox_stick_r_press" },
        { "StepHide", "草むらで這うか霧の中に入ると、敵から隠れられる", "", "" },
        { "Step4", "エイム", "mouse_right", "xbox_lt" },
        { "Step5", "エイム中に射撃", "mouse_left", "xbox_rt" },
        { "Step6", "リロード", "keyboard_r", "xbox_button_x" },
        { "StepSwitchAmmo", "弾を切り替える", "keyboard_q", "xbox_button_y" },
        { "StepIgnite", "焼夷弾で草を撃つと燃やせる", "mouse_left", "xbox_rt" },
        { "StepBurnZombie", "燃えている草にゾンビを誘い込むと、ゾンビも燃やせる", "", "" },
        { "Step7", "準備完了だ！！", "", "" },
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
    messageText_ = std::make_unique<Text>(engine_, "Maru");
    UITextStyle::ApplyMaruShadow(*messageText_);

    // 右下に置く。右端から左へ、文字・画像の順に並べる
    skipText_ = std::make_unique<Text>(engine_, "Maru");
    UITextStyle::ApplyMaruShadow(*skipText_);
    skipText_->SetScreenAnchor({ 1.0f, 1.0f });
    skipText_->SetPivot({ 1.0f, 0.5f });
    skipText_->SetLayerOrder(21);
    skipIcon_ = std::make_unique<Sprite>(engine_);
    skipBarBack_ = std::make_unique<Sprite>(engine_);
    skipBarFill_ = std::make_unique<Sprite>(engine_);
    for (Sprite* sprite : { skipIcon_.get(), skipBarBack_.get(), skipBarFill_.get() })
    {
        sprite->SetScreenAnchor({ 1.0f, 1.0f });
        sprite->SetAnchorPoint({ 0.0f, 0.5f });
        sprite->SetLayerOrder(21);
    }

    for (const auto& defaultContent : kDefaultStepContents)
    {
        stepContents_.push_back({ defaultContent[0], defaultContent[1], defaultContent[2], defaultContent[3] });
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
    binder_->Bind("BurnZombieWaitTime", &burnZombieWaitTime_, 5.0f, 0.01f, 0.0f, 30.0f);
    binder_->Bind("SkipHoldTime", &skipHoldTime_, 1.0f, 0.01f, 0.1f, 5.0f);
    binder_->Bind("SkipHintMargin", &skipHintMargin_, { 40.0f, 40.0f }, 1.0f);
    binder_->Bind("SkipHintFontSize", &skipHintFontSize_, 22.0f, 0.5f, 4.0f, 200.0f);
    binder_->Bind("SkipHintIconSize", &skipHintIconSize_, 36.0f, 0.5f, 4.0f, 256.0f);
    binder_->Bind("SkipHintSpacing", &skipHintSpacing_, 8.0f, 0.5f, 0.0f, 100.0f);
    binder_->BindColor("SkipHintColor", &skipHintColor_, { 1.0f, 1.0f, 1.0f, 0.8f });
    binder_->Bind("SkipBarHeight", &skipBarHeight_, 4.0f, 0.1f, 0.0f, 50.0f);
    binder_->Bind("SkipBarOffsetY", &skipBarOffsetY_, 26.0f, 0.5f);
    binder_->BindColor("SkipBarBackColor", &skipBarBackColor_, { 1.0f, 1.0f, 1.0f, 0.2f });
    binder_->BindColor("SkipBarFillColor", &skipBarFillColor_, { 1.0f, 1.0f, 1.0f, 0.9f });

    binder_->Bind("PanelOffset", &panelOffset_, { 0.0f, 200.0f }, 1.0f);
    binder_->Bind("PanelSize", &panelSize_, { 1280.0f, 130.0f }, 1.0f, 1.0f, 4000.0f);
    binder_->BindColor("PanelColor", &panelColor_, { 0.0f, 0.0f, 0.0f, 0.7f });
    binder_->Bind("PanelEdgeFade", &panelEdgeFade_, 0.35f, 0.01f, 0.0f, 0.5f,
        [this]() { CreatePanelTexture(); });

    binder_->BindColor("TextColor", &textColor_, { 1.0f, 1.0f, 1.0f, 1.0f });
    binder_->Bind("FontSize", &fontSize_, 28.0f, 0.5f, 4.0f, 200.0f);
    binder_->Bind("TextOffsetY", &textOffsetY_, -24.0f, 0.5f);
    binder_->BindFontName("Font", messageText_->GetFontName(),
        [this](const std::string& fontName) { messageText_->SetFont(fontName); skipText_->SetFont(fontName); });

    binder_->Bind("IconSize", &iconSize_, 48.0f, 0.5f, 4.0f, 256.0f);
    binder_->Bind("IconSpacing", &iconSpacing_, 8.0f, 0.5f, 0.0f, 100.0f);
    binder_->Bind("IconOffsetY", &iconOffsetY_, 26.0f, 0.5f);

    for (size_t stepIndex = 0; stepIndex < stepContents_.size(); ++stepIndex)
    {
        StepContent& content = stepContents_[stepIndex];
        const std::string prefix = content.saveKey + "_";

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

    isUsingController_ = GameInput::IsUsingController();
    ApplySkipIcon();
    StartStep(Step::Finished);
}

void Tutorial::Begin()
{
    if (!isEnabled_)
    {
        resumeStep_ = Step::Finished;
        return;
    }

    // 長押しで飛ばせるので、前に終えていても毎回最初から出す
    resumeStep_ = Step::Move;
    isSkipped_ = false;
    skipHoldTimer_ = 0.0f;
    skipHintVisibility_ = 0.0f;

    // ゲームが始まってから少し待って出す
    currentStep_ = Step::Finished;
    StartStep(resumeStep_);
    phase_ = Phase::Delay;
}

void Tutorial::CreatePanelTexture()
{
    // 横 1 列の白い画像を作り、左右の端だけ透明にしていく
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
        reloadCountAtStart_ = player_->GetWeapon()->GetReloadCount();
        igniteCountAtStart_ = player_->GetWeapon()->GetIgniteCount();
        ammoTypeAtStart_ = static_cast<int>(player_->GetWeapon()->GetAmmoType());
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
    // 最後に触った方の画像を出す
    const bool wasUsingController = isUsingController_;
    isUsingController_ = GameInput::IsUsingController();

    if (isUsingController_ != wasUsingController)
    {
        ApplyIcons();
        ApplySkipIcon();
    }
}

void Tutorial::ApplySkipIcon()
{
    // 画像が置かれていなければ、キーの名前を文字で出す
    const char* iconName = isUsingController_ ? "xbox_button_view" : "keyboard_tab";
    hasSkipIcon_ = FS::TextureManager::GetInstance().GetMetaData(iconName) != nullptr;
    if (hasSkipIcon_)
    {
        skipIcon_->SetTexture(iconName);
        skipText_->SetText("長押しでスキップ");
    }
    else
    {
        skipText_->SetText(isUsingController_ ? "View 長押しでスキップ" : "Tab 長押しでスキップ");
    }
}

bool Tutorial::UpdateSkip(float deltaTime)
{
    // 完了を出している間は、もう飛ばすものがない
    if (currentStep_ == Step::Complete)
    {
        skipHoldTimer_ = 0.0f;
        skipHintVisibility_ = std::max(skipHintVisibility_ - deltaTime / fadeTime_, 0.0f);
        return false;
    }

    skipHintVisibility_ = std::min(skipHintVisibility_ + deltaTime / fadeTime_, 1.0f);

    if (GameInput::IsPressed(GameAction::SkipTutorial))
    {
        skipHoldTimer_ += deltaTime;
    }
    else
    {
        // 離したらすばやく戻す
        skipHoldTimer_ = std::max(skipHoldTimer_ - deltaTime * 3.0f, 0.0f);
    }

    if (skipHoldTimer_ < skipHoldTime_)
    {
        return false;
    }

    UISound::PlayConfirm();
    isSkipped_ = true;
    skipHoldTimer_ = 0.0f;
    skipHintVisibility_ = 0.0f;
    resumeStep_ = Step::Finished;
    StartStep(Step::Finished);
    return true;
}

void Tutorial::ApplyIcons()
{
    iconCount_ = 0;
    if (currentStep_ == Step::Finished)
    {
        return;
    }

    const StepContent& content = stepContents_[static_cast<size_t>(currentStep_)];
    // 画像が置かれていないボタンは出さない
    std::vector<std::string> iconNames;
    for (const std::string& iconName : SplitByComma(isUsingController_ ? content.controllerIcons : content.keyboardIcons))
    {
        if (TextureManager::GetInstance().GetMetaData(iconName))
        {
            iconNames.push_back(iconName);
        }
    }

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
    const StateMachine<Player>* playerStateMachine = player_->GetStateMachine();

    switch (currentStep_)
    {
    case Step::Move:
        if (player_->IsMoving())
        {
            heldTime_ += deltaTime;
        }
        return heldTime_ >= requiredHoldTime_;

    case Step::Sprint:
        if (playerStateMachine->IsCurrentState<PlayerStateSprint>())
        {
            heldTime_ += deltaTime;
        }
        return heldTime_ >= requiredHoldTime_;

    case Step::Dodge:
        return playerStateMachine->IsCurrentState<PlayerStateDodge>();

    case Step::Jump:
        return playerStateMachine->IsCurrentState<PlayerStateJump>();

    case Step::Crawl:
        if (player_->IsCrawling())
        {
            heldTime_ += deltaTime;
        }
        return heldTime_ >= requiredHoldTime_;

    case Step::Hide:
        if (player_->IsHidden())
        {
            heldTime_ += deltaTime;
        }
        return heldTime_ >= requiredHoldTime_;

    case Step::Aim:
        if (playerStateMachine->IsCurrentState<PlayerStateAiming>())
        {
            heldTime_ += deltaTime;
        }
        return heldTime_ >= requiredHoldTime_;

    case Step::Shoot:
        return player_->GetShotCount() > shotCountAtStart_;

    case Step::Reload:
    {
        // 満タンだとリロードできず先へ進めなくなるので、そのときも済んだことにする
        const PlayerWeapon* weapon = player_->GetWeapon();
        const bool isFull = weapon->GetAmmo() == weapon->GetMagazineSize() && !weapon->IsReloading();
        return weapon->GetReloadCount() > reloadCountAtStart_ || isFull;
    }

    case Step::SwitchAmmo:
        return static_cast<int>(player_->GetWeapon()->GetAmmoType()) != ammoTypeAtStart_;

    case Step::Ignite:
        return player_->GetWeapon()->GetIgniteCount() > igniteCountAtStart_;

    case Step::BurnZombie:
        // 実際にゾンビに火がついたら、少し待ってから先へ進む
        if (heldTime_ > 0.0f || (enemyManager_ && enemyManager_->HasBurningEnemy()))
        {
            heldTime_ += deltaTime;
        }
        return heldTime_ >= burnZombieWaitTime_;

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
    if (UpdateSkip(deltaTime))
    {
        return;
    }
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
            resumeStep_ = static_cast<Step>(static_cast<int>(currentStep_) + 1);

            // 這えたら立たせて、次の説明を立った姿勢から始める
            if (currentStep_ == Step::Crawl)
            {
                player_->StandUp();
            }
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
    if (currentStep_ == Step::Finished)
    {
        return;
    }

    // 説明の切り替わりで帯が消えている間も、案内は出したままにする
    DrawSkipHint();

    if (visibility_ <= 0.0f)
    {
        return;
    }

    panel_->SetPosition(panelOffset_);
    panel_->SetSize(panelSize_);
    panel_->SetColor(Vector4(panelColor_.x, panelColor_.y, panelColor_.z, panelColor_.w * visibility_));
    panel_->Draw();

    // 画像がないときは文章を帯の真ん中に出す
    const float textY = (iconCount_ > 0) ? panelOffset_.y + textOffsetY_ : panelOffset_.y;
    messageText_->SetPosition({ panelOffset_.x, textY });
    messageText_->SetFontSize(fontSize_);
    // 影も一緒に薄くするため、色ではなく不透明度で消す
    messageText_->SetColor(textColor_);
    messageText_->SetOpacity(visibility_);
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

void Tutorial::DrawSkipHint()
{
    if (skipHintVisibility_ <= 0.0f)
    {
        return;
    }

    skipText_->SetFontSize(skipHintFontSize_);
    const float textWidth = skipText_->MeasureSize().x;
    const float iconWidth = hasSkipIcon_ ? skipHintIconSize_ + skipHintSpacing_ : 0.0f;
    const float rowWidth = iconWidth + textWidth;
    const float rightX = -skipHintMargin_.x;
    const float leftX = rightX - rowWidth;
    const float rowY = -skipHintMargin_.y;

    skipText_->SetPosition({ rightX, rowY });
    skipText_->SetColor(skipHintColor_);
    skipText_->SetOpacity(skipHintVisibility_);
    skipText_->Draw();

    if (hasSkipIcon_)
    {
        skipIcon_->SetPosition({ leftX, rowY });
        skipIcon_->SetSize({ skipHintIconSize_, skipHintIconSize_ });
        skipIcon_->SetColor({ 1.0f, 1.0f, 1.0f, skipHintColor_.w * skipHintVisibility_ });
        skipIcon_->Draw();
    }

    // 押している長さを、左から伸びる線で見せる
    const float barY = rowY + skipBarOffsetY_;
    Vector4 backColor = skipBarBackColor_;
    backColor.w *= skipHintVisibility_;
    skipBarBack_->SetPosition({ leftX, barY });
    skipBarBack_->SetSize({ rowWidth, skipBarHeight_ });
    skipBarBack_->SetColor(backColor);
    skipBarBack_->Draw();

    const float holdRatio = std::clamp(skipHoldTimer_ / std::max(skipHoldTime_, 0.001f), 0.0f, 1.0f);
    if (holdRatio > 0.0f)
    {
        Vector4 fillColor = skipBarFillColor_;
        fillColor.w *= skipHintVisibility_;
        skipBarFill_->SetPosition({ leftX, barY });
        skipBarFill_->SetSize({ rowWidth * holdRatio, skipBarHeight_ });
        skipBarFill_->SetColor(fillColor);
        skipBarFill_->Draw();
    }
}

void Tutorial::DebugGui()
{
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
        resumeStep_ = Step::Move;
        currentStep_ = Step::Finished;
        StartStep(Step::Move);
    }
    ImGui::SameLine();
    if (ImGui::Button("次へ飛ばす") && currentStep_ != Step::Finished)
    {
        resumeStep_ = static_cast<Step>(static_cast<int>(currentStep_) + 1);
        StartStep(resumeStep_);
    }

    binder_->Draw("IsEnabled", "ゲーム開始時に表示する");
    binder_->Draw("StartDelay", "最初に出すまでの時間 (秒)");
    binder_->Draw("FadeTime", "フェードの時間 (秒)");
    binder_->Draw("ClearedWaitTime", "できてから消えるまで (秒)");
    binder_->Draw("RequiredHoldTime", "移動・ダッシュ・エイムを続ける時間 (秒)");
    binder_->Draw("CompleteShowTime", "完了を出しておく時間 (秒)");
    binder_->Draw("BurnZombieWaitTime", "ゾンビに火がついてから次へ進むまで (秒)");
    binder_->Draw("SkipHoldTime", "長押しで飛ばすまでの時間 (秒)");

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

    if (ImGui::CollapsingHeader("長押しでスキップの案内"))
    {
        binder_->Draw("SkipHintMargin", "画面の右下からの距離");
        binder_->Draw("SkipHintFontSize", "文字の大きさ");
        binder_->Draw("SkipHintIconSize", "ボタンの絵の大きさ");
        binder_->Draw("SkipHintSpacing", "絵と文字の間隔");
        binder_->Draw("SkipHintColor", "色");
        binder_->Draw("SkipBarHeight", "線の太さ");
        binder_->Draw("SkipBarOffsetY", "線の位置 (文字の中心から下へ)");
        binder_->Draw("SkipBarBackColor", "線の下地の色");
        binder_->Draw("SkipBarFillColor", "押している長さの色");
    }

    if (ImGui::CollapsingHeader("説明の内容"))
    {
        for (size_t stepIndex = 0; stepIndex < stepContents_.size(); ++stepIndex)
        {
            binder_->Draw(stepContents_[stepIndex].saveKey + "_Message", std::format("{} 番目", stepIndex + 1));
        }
    }

    ImGui::End();
}
