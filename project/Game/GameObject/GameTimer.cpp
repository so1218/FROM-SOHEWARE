#include "GameTimer.h"
#include "TextureHandle.h"
#include "TimeManager.h" 
#include "externals/imgui/imgui.h"

GameTimer::GameTimer(Engine* engine) : engine_(engine)
{
}

void GameTimer::Initialize(float limitMinutes)
{
    // 時間設定
    maxTime_ = limitMinutes * 60.0f;
    currentTime_ = maxTime_;
    isTimeUp_ = false;

    // テクスチャハンドルの取得
    std::array<TextureID, 10> idMap = {
        TextureID::num0, TextureID::num1, TextureID::num2, TextureID::num3, TextureID::num4,
        TextureID::num5, TextureID::num6, TextureID::num7, TextureID::num8, TextureID::num9
    };
    for (int i = 0; i < 10; ++i) {
        digitTextureHandles_[i] = TextureHandle::Get(idMap[i]);
    }
    // コロン用のテクスチャ
    colonTextureHandle_ = TextureHandle::Get(TextureID::white1x1);

    // スプライトの生成 (5文字分: MM:SS)
    for (int i = 0; i < 5; ++i)
    {
        sprites_[i] = std::make_unique<Sprite>(engine_);
        // コロン(インデックス2)以外は数字の0で初期化
        if (i == 2) {
            sprites_[i]->SetTextureHandle(colonTextureHandle_);
        }
        else {
            sprites_[i]->SetTextureHandle(digitTextureHandles_[0]);
        }
    }

    // GlobalVariablesの登録と適用
    auto* gv = GlobalVariables::GetInstance();
    auto groupName = GetGlobalVariableGroupName();
    gv->CreateGroup(groupName);
    gv->AddItem(groupName, "Position", position_);
    gv->AddItem(groupName, "Char Size", charSize_);
    gv->AddItem(groupName, "Spacing", charSpacing_);
    gv->AddItem(groupName, "Color", color_);

    ApplyGlobalVariables();
}

void GameTimer::ApplyGlobalVariables()
{
    auto* gv = GlobalVariables::GetInstance();
    auto groupName = GetGlobalVariableGroupName();

    position_ = gv->GetVector2Value(groupName, "Position");
    charSize_ = gv->GetVector2Value(groupName, "Char Size");
    charSpacing_ = gv->GetFloatValue(groupName, "Spacing");
    color_ = gv->GetVector4Value(groupName, "Color");

    // 配置とサイズの更新
    for (int i = 0; i < 5; ++i)
    {
        Vector2 pos = position_;
        pos.x += i * charSpacing_; // 横にずらす

        sprites_[i]->SetPosition(pos);
        sprites_[i]->SetSize(charSize_);
        sprites_[i]->SetColor(Math::ColorVectorToUint32(color_));

        // コロンだけサイズを変えたい場合はここで個別調整も可能
        if (i == 2) {
            // 例: コロンは細くする
            Vector2 colonSize = { charSize_.x * 0.5f, charSize_.y * 0.5f };
            Vector2 colonPos = { pos.x + (charSize_.x - colonSize.x) / 2.0f, pos.y + (charSize_.y - colonSize.y) / 2.0f };
            sprites_[i]->SetSize(colonSize);
            sprites_[i]->SetPosition(colonPos);
        }
    }
}

void GameTimer::Update()
{
    if (isTimeUp_) return;

    // 時間経過
    float dt = TimeManager::GetInstance()->GetDeltaTime();
    currentTime_ -= dt;

    if (currentTime_ <= 0.0f)
    {
        currentTime_ = 0.0f;
        isTimeUp_ = true;
    }

    // 分と秒の計算
    int totalSeconds = static_cast<int>(currentTime_);
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;

    // テクスチャの更新
    UpdateSpriteTextures(minutes, seconds);

    // デバッグ時の調整用
    ApplyGlobalVariables();
}

void GameTimer::UpdateSpriteTextures(int minutes, int seconds)
{
    // 分の10の位
    sprites_[0]->SetTextureHandle(digitTextureHandles_[(minutes / 10) % 10]);
    // 分の1の位
    sprites_[1]->SetTextureHandle(digitTextureHandles_[minutes % 10]);

    // 秒の10の位
    sprites_[3]->SetTextureHandle(digitTextureHandles_[(seconds / 10) % 10]);
    // 秒の1の位
    sprites_[4]->SetTextureHandle(digitTextureHandles_[seconds % 10]);
}

void GameTimer::Draw()
{
    for (auto& sprite : sprites_)
    {
        sprite->Draw();
    }
}

void GameTimer::DebugDraw()
{
    ImGui::Begin("ゲームタイマー");

    auto* gv = GlobalVariables::GetInstance();
    auto groupName = GetGlobalVariableGroupName();

    bool changed = false;

    ImGui::Separator();

    if (ImGui::DragFloat2("位置", &position_.x, 1.0f))
    {
        gv->SetValue(groupName, "Position", position_);
        changed = true;
    }
    if (ImGui::DragFloat2("文字サイズ", &charSize_.x, 1.0f))
    {
        gv->SetValue(groupName, "Char Size", charSize_);
        changed = true;
    }
    if (ImGui::DragFloat("文字間隔", &charSpacing_, 1.0f))
    {
        gv->SetValue(groupName, "Spacing", charSpacing_);
        changed = true;
    }
    if (ImGui::ColorEdit4("色", &color_.x))
    {
        gv->SetValue(groupName, "Color", color_);
        changed = true;
    }

    if (changed)
    {
        ApplyGlobalVariables();
    }

    ImGui::End();
}