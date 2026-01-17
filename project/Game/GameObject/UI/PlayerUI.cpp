#include "PlayerUI.h"
#include "TextureHandle.h"
#include "GlobalVariables.h"
#include "ImGuiManager.h"

PlayerUI::PlayerUI(Engine* engine, Camera* camera, Player* player) : GameObject(engine, camera), player_(player)
{
    xpBarBgSprite_ = std::make_unique<Sprite>(engine_);
    xpBarSprite_ = std::make_unique<Sprite>(engine_);
    hpBarBgSprite_ = std::make_unique<Sprite>(engine_);
    hpBarSprite_ = std::make_unique<Sprite>(engine_);

    spriteMove_ = std::make_unique<Sprite>(engine_);
    spriteCamera_ = std::make_unique<Sprite>(engine_);
    spriteExpFrame_ = std::make_unique<Sprite>(engine_);
    spriteHpFrame_ = std::make_unique<Sprite>(engine_);
    spriteIkinokore_ = std::make_unique<Sprite>(engine_);
}

void PlayerUI::Initialize()
{
    // ゲージ背景
    xpBarBgSprite_->SetTexture(TextureID::white1x1);
    xpBarBgSprite_->SetColor(0x444444FF);
    xpBarBgSprite_->SetLayerOrder(10);

    // ゲージ本体
    xpBarSprite_->SetTexture(TextureID::white1x1);
    xpBarSprite_->SetColor(0x00FF00FF);
    xpBarSprite_->SetLayerOrder(11);

    // HPバー背景
    hpBarBgSprite_->SetTexture(TextureID::white1x1);
    hpBarBgSprite_->SetColor(0x330000FF);
    hpBarBgSprite_->SetLayerOrder(10);

    // HPバー本体
    hpBarSprite_->SetTexture(TextureID::white1x1);
    hpBarSprite_->SetColor(0xFF0000FF);
    hpBarSprite_->SetLayerOrder(18);

    spritePosMove_ = { 251, 628 };
    spriteMove_->SetPosition(spritePosMove_);
    spriteSizeMove_ = { 454.0f, 70.0f };
    spriteMove_->SetSize(spriteSizeMove_);
    spriteMove_->SetAnchorPoint({ 0.5f, 0.5f });
    spriteMove_->SetTexture(TextureID::moveSousa);

    spritePosCamera_ = { 251, 681 };
    spriteCamera_->SetPosition(spritePosCamera_);
    spriteSizeCamera_ = { 449.0f, 70.0f };
    spriteCamera_->SetSize(spriteSizeCamera_);
    spriteCamera_->SetAnchorPoint({ 0.5f, 0.5f });
    spriteCamera_->SetTexture(TextureID::cameraSousa);

    spritePosExpFrame_ = { 645, 28.7f };
    spriteExpFrame_->SetPosition(spritePosExpFrame_);
    spriteSizeExpFrame_ = { 574.0f, 26.0f };
    spriteExpFrame_->SetSize(spriteSizeExpFrame_);
    spriteExpFrame_->SetAnchorPoint({ 0.5f, 0.5f });
    spriteExpFrame_->SetTexture(TextureID::hpGage);
    spriteExpFrame_->SetColor(0x000000ff);

    spritePosHpFrame_ = { 646, 29 };
    spriteHpFrame_->SetPosition(spritePosHpFrame_);
    spriteSizeHpFrame_ = { 124.0f, 28.0f };
    spriteHpFrame_->SetSize(spriteSizeHpFrame_);
    spriteHpFrame_->SetTexture(TextureID::hpGage);
    spriteHpFrame_->SetColor(0x000000ff);

    spritePosIkinokore_ = { 230, 562 };
    spriteIkinokore_->SetPosition(spritePosIkinokore_);
    spriteSizeIkinokore_ = { 403.0f, 90.0f };
    spriteIkinokore_->SetSize(spriteSizeIkinokore_);
    spriteIkinokore_->SetAnchorPoint({ 0.5f, 0.5f });
    spriteIkinokore_->SetTexture(TextureID::ikinokore);

    std::array<TextureID, 10> idMap = {
        TextureID::num0, TextureID::num1, TextureID::num2, TextureID::num3, TextureID::num4,
        TextureID::num5, TextureID::num6, TextureID::num7, TextureID::num8, TextureID::num9
    };
    for (int i = 0; i < 10; ++i) {
        digitTextureId_[i] = idMap[i];
    }

    auto* gv = GlobalVariables::GetInstance();
    auto groupName = GetGlobalVariableGroupName();
    gv->CreateGroup(groupName);

    gv->AddItem(groupName, "XP Bar Pos", xpBarPos_);
    gv->AddItem(groupName, "XP Bar Size", xpBarSize_);
    gv->AddItem(groupName, "HP Bar Size", hpBarSize_);
    gv->AddItem(groupName, "HP Offset Y", hpBarOffsetHeight_);
    gv->AddItem(groupName, "Level Num Pos", levelNumberPos_);
    gv->AddItem(groupName, "Level Num Space", numberSpace_);
    gv->AddItem(groupName, "Level Num Size", numberSize_);

    ApplyGlobalVariables();

    // 表示更新用の変数を初期化
    currentDisplayLevel_ = -1;
    levelNumberSprites_.clear();
}

void PlayerUI::ApplyGlobalVariables()
{
    auto* gv = GlobalVariables::GetInstance();
    auto groupName = GetGlobalVariableGroupName();

    xpBarPos_ = gv->GetVector2Value(groupName, "XP Bar Pos");
    xpBarSize_ = gv->GetVector2Value(groupName, "XP Bar Size");
    hpBarSize_ = gv->GetVector2Value(groupName, "HP Bar Size");
    hpBarOffsetHeight_ = gv->GetFloatValue(groupName, "HP Offset Y");
    levelNumberPos_ = gv->GetVector2Value(groupName, "Level Num Pos");
    numberSpace_ = gv->GetFloatValue(groupName, "Level Num Space");
    numberSize_ = gv->GetVector2Value(groupName, "Level Num Size");

    currentDisplayLevel_ = -1;
}

void PlayerUI::Update()
{
    if (!player_) return;

    // XPバー
    float ratio = player_->GetXpRatio();
    float currentWidth = xpBarSize_.x * std::clamp(ratio, 0.0f, 1.0f);

    xpBarBgSprite_->SetPosition(xpBarPos_);
    xpBarBgSprite_->SetSize(xpBarSize_);

    xpBarSprite_->SetPosition(xpBarPos_);
    xpBarSprite_->SetSize({ currentWidth, xpBarSize_.y });

    // HPバー
    float hpRatio = player_->GetHpRatio();
    float currentHPWidth = hpBarSize_.x * std::clamp(hpRatio, 0.0f, 1.0f);
    hpBarSprite_->SetSize({ currentHPWidth, hpBarSize_.y });
    hpBarBgSprite_->SetSize(hpBarSize_); // 背景サイズも更新

    // 3D位置から2Dスクリーン位置への変換
    Vector3 playerPos = player_->GetWorldPosition();

    playerPos.y += hpBarOffsetHeight_;

    Matrix4x4 viewProjection = player_->GetCamera()->GetViewProjectionMatrix();
    Vector2 screenPos = Math::WorldToScreen(playerPos, viewProjection, kClientWidth, kClientHeight);

    Vector2 centeredPosBg = { screenPos.x - hpBarSize_.x / 2.0f, screenPos.y };
    Vector2 centeredPosFg = { screenPos.x - hpBarSize_.x / 2.0f, screenPos.y };

    hpBarBgSprite_->SetPosition(centeredPosBg);
    hpBarSprite_->SetPosition(centeredPosFg);
    spriteHpFrame_->SetPosition({ screenPos.x - hpBarSize_.x / 2.0f - 5.0f, screenPos.y - 5.0f });

    // レベル数値
    int currentLevel = player_->GetLevel();

    // レベルが変わった、またはApplyGlobalVariablesで強制リセット(-1)された場合に再生成
    if (currentDisplayLevel_ != currentLevel)
    {
        currentDisplayLevel_ = currentLevel;
        levelNumberSprites_.clear();

        std::string levelStr = std::to_string(currentLevel);

        for (size_t i = 0; i < levelStr.size(); ++i)
        {
            int digit = levelStr[i] - '0';
            auto sprite = std::make_unique<Sprite>(engine_);
            sprite->SetTexture((TextureID)digitTextureId_[digit]);

            Vector2 pos = levelNumberPos_;
            pos.x += i * numberSpace_;

            sprite->SetPosition(pos);
            sprite->SetSize(numberSize_); 
            sprite->SetColor(0xFFFF44FF);
            levelNumberSprites_.push_back(std::move(sprite));
        }
    }
}

void PlayerUI::Draw()
{
    xpBarBgSprite_->Draw();
    xpBarSprite_->Draw();

    hpBarBgSprite_->Draw();
    hpBarSprite_->Draw();

    spriteMove_->Draw();
    spriteCamera_->Draw();
    spriteExpFrame_->Draw();
    spriteHpFrame_->Draw();
    spriteIkinokore_->Draw();

    for (auto& sprite : levelNumberSprites_)
    {
        sprite->Draw();
    }
}

void PlayerUI::DebugDraw()
{
#ifdef IS_DEVELOPMENT
    ImGui::Begin("PlayerUI");

    auto* gv = GlobalVariables::GetInstance();
    auto groupName = GetGlobalVariableGroupName();
    bool changed = false;

    if (ImGui::TreeNode("経験値バー"))
    {
        if (ImGui::DragFloat2("位置", &xpBarPos_.x, 1.0f)) {
            gv->SetValue(groupName, "XP Bar Pos", xpBarPos_);
            changed = true;
        }
        if (ImGui::DragFloat2("サイズ", &xpBarSize_.x, 1.0f)) {
            gv->SetValue(groupName, "XP Bar Size", xpBarSize_);
            changed = true;
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("HPバー"))
    {
        if (ImGui::DragFloat2("サイズ", &hpBarSize_.x, 1.0f)) {
            gv->SetValue(groupName, "HP Bar Size", hpBarSize_);
            changed = true;
        }
        if (ImGui::DragFloat("高さオフセット", &hpBarOffsetHeight_, 0.01f)) {
            gv->SetValue(groupName, "HP Offset Y", hpBarOffsetHeight_);
            changed = true;
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("レベル表示"))
    {
        if (ImGui::DragFloat2("位置", &levelNumberPos_.x, 1.0f)) {
            gv->SetValue(groupName, "Level Num Pos", levelNumberPos_);
            changed = true;
        }
        if (ImGui::DragFloat("文字間隔", &numberSpace_, 0.5f)) {
            gv->SetValue(groupName, "Level Num Space", numberSpace_);
            changed = true;
        }
        if (ImGui::DragFloat2("文字サイズ", &numberSize_.x, 1.0f)) {
            gv->SetValue(groupName, "Level Num Size", numberSize_);
            changed = true;
        }
        ImGui::TreePop();
    }

    if (ImGui::DragFloat2("Sprite Pos Move", &spritePosMove_.x, 1.0f))
    {
        spriteMove_->SetPosition(spritePosMove_);
    }
    if (ImGui::DragFloat2("Sprite Size Move", &spriteSizeMove_.x, 1.0f))
    {
        spriteMove_->SetSize(spriteSizeMove_);
    }
    if (ImGui::DragFloat2("Sprite Pos Camera", &spritePosCamera_.x, 1.0f))
    {
        spriteCamera_->SetPosition(spritePosCamera_);
    }
    if (ImGui::DragFloat2("Sprite Size Camera", &spriteSizeCamera_.x, 1.0f))
    {
        spriteCamera_->SetSize(spriteSizeCamera_);
    }
    if (ImGui::DragFloat2("Sprite Pos HpFrame", &spritePosExpFrame_.x, 1.0f))
    {
        spriteExpFrame_->SetPosition(spritePosExpFrame_);
    }
    if (ImGui::DragFloat2("Sprite Size ExpFrame", &spriteSizeExpFrame_.x, 1.0f))
    {
        spriteExpFrame_->SetSize(spriteSizeExpFrame_);
    }
    if (ImGui::DragFloat2("Sprite Size HpFrame", &spriteSizeHpFrame_.x, 1.0f))
    {
        spriteHpFrame_->SetSize(spriteSizeHpFrame_);
    }
    if (ImGui::DragFloat2("Sprite Pos Ikinokore", &spritePosIkinokore_.x, 1.0f))
    {
        spriteIkinokore_->SetPosition(spritePosIkinokore_);
    }
    if (ImGui::DragFloat2("Sprite Size Ikinokore", &spriteSizeIkinokore_.x, 1.0f))
    {
        spriteIkinokore_->SetSize(spriteSizeIkinokore_);
    }

    // 値が変更されたら適用
    if (changed)
    {
        ApplyGlobalVariables();
    }

    ImGui::End();
#endif
}