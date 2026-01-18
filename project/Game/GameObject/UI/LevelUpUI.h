#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "UpgradeInfo.h"
#include "Sprite.h"
#include <vector>
#include <memory>

class LevelUpUI : public GameObject
{
public:
    LevelUpUI(Engine* engine);

    GameObjectType GetType() const override { return GameObjectType::UI; }

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

    // 選択肢をセットして、選択状態をリセットする
    void Activate(const std::vector<UpgradeInfo>& options);

    // 決定されたか
    bool IsDecided() const { return isDecided_; }

    // 決定された情報を取得
    UpgradeInfo GetDecision() const;

    // 調整項目の適用
    void ApplyGlobalVariables();
    std::vector<std::string> GetGlobalVariableGroupName() const { return { "LevelUpUI" }; }

private:
    // 選択肢データ
    std::vector<UpgradeInfo> currentOptions_;

    // 選択中のインデックス
    int selectedIndex_ = 0;

    // 決定フラグ
    bool isDecided_ = false;

    // スプライト
    // 背景
    std::vector<std::unique_ptr<Sprite>> cardBgSprites_;
    // 中身
    std::vector<std::unique_ptr<Sprite>> cardContentSprites_;
    // 中身の枠
    std::vector<std::unique_ptr<Sprite>> cardContentFrameSprites_;

    Vector2 cardStartPos_ = { 640.0f, 150.0f };
    Vector2 cardSize_ = { 400.0f, 120.0f };    
    float cardGapY_ = 140.0f; // 縦の間隔

    // 選択時の強調パラメータ
    float selectedScale_ = 1.1f;  
    Vector4 selectedColor_ = { 1.0f, 1.0f, 1.0f, 1.0f }; 
    Vector4 unselectedColor_ = { 0.6f, 0.6f, 0.6f, 1.0f }; 
};