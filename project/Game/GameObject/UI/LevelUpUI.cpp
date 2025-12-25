#include "LevelUpUI.h"
#include "GlobalVariables.h" 
#include "ImGuiManager.h"
#include "Input.h"
#include "TextureHandle.h"

LevelUpUI::LevelUpUI(Engine* engine) : engine_(engine)
{
}

void LevelUpUI::Initialize()
{
	for (int i = 0; i < 3; ++i)
	{
		// 背景用
		auto bg = std::make_unique<Sprite>(engine_);
		bg->SetTextureHandle(TextureHandle::Get(TextureID::white1x1));
		bg->SetAnchorPoint({ 0.5f, 0.5f }); // 中央基準にすると拡大縮小の中心がズレなくて良い
		cardBgSprites_.push_back(std::move(bg));

		// 中身用
		auto content = std::make_unique<Sprite>(engine_);
		content->SetAnchorPoint({ 0.5f, 0.5f });
		cardContentSprites_.push_back(std::move(content));
	}

	// グローバル変数の初期化
	auto* gv = GlobalVariables::GetInstance();
	auto groupName = GetGlobalVariableGroupName();
	gv->CreateGroup(groupName);

	gv->AddItem(groupName, "Card Start Pos", cardStartPos_);
	gv->AddItem(groupName, "Card Size", cardSize_);
	gv->AddItem(groupName, "Card Gap Y", cardGapY_);
	gv->AddItem(groupName, "Selected Scale", selectedScale_);

	ApplyGlobalVariables();
}

void LevelUpUI::ApplyGlobalVariables()
{
	auto* gv = GlobalVariables::GetInstance();
	auto groupName = GetGlobalVariableGroupName();

	cardStartPos_ = gv->GetVector2Value(groupName, "Card Start Pos");
	cardSize_ = gv->GetVector2Value(groupName, "Card Size");
	cardGapY_ = gv->GetFloatValue(groupName, "Card Gap Y");
	selectedScale_ = gv->GetFloatValue(groupName, "Selected Scale");
}

void LevelUpUI::Activate(const std::vector<UpgradeInfo>& options)
{
	currentOptions_ = options;
	selectedIndex_ = 0;
	isDecided_ = false;

	// テクスチャの割り当て
	for (int i = 0; i < 3; ++i)
	{
		if (i < currentOptions_.size())
		{
			// UpgradeInfoに持たせた画像ハンドルをセット
			cardContentSprites_[i]->SetTextureHandle(currentOptions_[i].textureHandle);
		}
	}
}

void LevelUpUI::Update()
{
    if (Input::GetInstance().IsKeyTriggered(DIK_UP) || Input::GetInstance().IsStickUpTriggered(0, Input::StickType::LeftStick))
    {
        selectedIndex_--;
        if (selectedIndex_ < 0)
        {
            selectedIndex_ = static_cast<int>(currentOptions_.size()) - 1;
        }
        // 効果音を鳴らすならここ
    }
    if (Input::GetInstance().IsKeyTriggered(DIK_DOWN) || Input::GetInstance().IsStickDownTriggered(0, Input::StickType::LeftStick))
    {
        selectedIndex_++;
        if (selectedIndex_ >= currentOptions_.size()) {
            selectedIndex_ = 0; // 一番上へループ
        }
    }

    // 決定
    if (Input::GetInstance().IsKeyTriggered(DIK_SPACE) || Input::GetInstance().IsControllerButtonTriggered(0, Input::ButtonA))
    {
        isDecided_ = true;
        // 決定音を鳴らすならここ
    }


    // 見た目の更新
    for (int i = 0; i < 3; ++i)
    {
        // 有効な選択肢の数を超えていたら非表示にしてスキップ
        if (i >= currentOptions_.size()) {
            // 本来は SetVisible(false) などがあると良い
            // ここでは簡易的にサイズ0にして隠す例
            cardBgSprites_[i]->SetSize({ 0,0 });
            cardContentSprites_[i]->SetSize({ 0,0 });
            continue;
        }

        // 座標計算 (縦並び)
        Vector2 pos = {
            cardStartPos_.x,
            cardStartPos_.y + i * cardGapY_
        };

        // 選択中かどうかでパラメータを変える
        bool isSelected = (i == selectedIndex_);
        float scale = isSelected ? selectedScale_ : 1.0f;

        uint32_t color = isSelected ? 0xFFFFFFFF : 0x888888FF;

        // 背景スプライト更新
        cardBgSprites_[i]->SetPosition(pos);
        cardBgSprites_[i]->SetSize({ cardSize_.x * scale, cardSize_.y * scale });
        cardBgSprites_[i]->SetColor(color);

        // 中身スプライト更新
        cardContentSprites_[i]->SetPosition(pos);
        cardContentSprites_[i]->SetSize({ cardSize_.x * scale, cardSize_.y * scale });
        cardContentSprites_[i]->SetColor(color);
    }
}

void LevelUpUI::Draw()
{
    // 有効な枚数分だけ描画
    for (int i = 0; i < currentOptions_.size(); ++i)
    {
        cardBgSprites_[i]->Draw();
        cardContentSprites_[i]->Draw();
    }
}

void LevelUpUI::DebugDraw()
{
    ImGui::Begin("レベルアップUI");

    bool changed = false;
    auto* gv = GlobalVariables::GetInstance();
    auto groupName = GetGlobalVariableGroupName();

    if (ImGui::DragFloat2("開始位置", &cardStartPos_.x, 1.0f)) changed = true;
    if (ImGui::DragFloat2("大きさ", &cardSize_.x, 1.0f)) changed = true;
    if (ImGui::DragFloat("縦間隔", &cardGapY_, 1.0f)) changed = true;
    if (ImGui::DragFloat("選択時スケール", &selectedScale_, 0.01f)) changed = true;

    // 値が変わったら保存
    if (changed) 
    {
        gv->SetValue(groupName, "Card Start Pos", cardStartPos_);
        gv->SetValue(groupName, "Card Size", cardSize_);
        gv->SetValue(groupName, "Card Gap Y", cardGapY_);
        gv->SetValue(groupName, "Selected Scale", selectedScale_);
    }

    // デバッグ情報表示
    ImGui::Text("Selected Index: %d", selectedIndex_);
    ImGui::Text("Is Decided: %d", isDecided_);

    ImGui::End();
}

UpgradeInfo LevelUpUI::GetDecision() const
{
    if (selectedIndex_ >= 0 && selectedIndex_ < currentOptions_.size())
    {
        return currentOptions_[selectedIndex_];
    }
    return {};
}

