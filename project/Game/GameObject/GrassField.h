#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "PropertyBinder.h"
#include "GrassSystem.h"
#include "Terrain.h"

class Player;

class GrassField : public FE::GameObject
{
public:
    GrassField(FE::Engine* engine, Player* player);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

    void SetTerrain(FE::Terrain* terrain) { terrain_ = terrain; }

private:
    // 草を指定数、ランダムに再配置する関数
    void GenerateGrass();

private:
    FE::Engine* engine_;
    Player* player_;
    FE::Terrain* terrain_ = nullptr;

    std::unique_ptr<FE::GrassSystem> grassSystem_;
    std::unique_ptr<FE::PropertyBinder> binder_;

    // 配置用のパラメータ
    int grassCount_ = 3000;         // 配置する数
    float spreadRadius_ = 20.0f;    // 配置する半径
    int prevGrassCount_ = 50;
    float prevSpreadRadius_ = 10.0f;

    // 全体のスケール
    float baseScale_ = 1.0f;

    // 再生成用の記憶変数
    FE::Vector3 prevPosition_;
    float prevBaseScale_ = 1.0f;

    FE::WorldTransform transform_;

    FE::Vector4 prevBaseGrassColor_;
    float baseWidth_ = 1.0f;      
    float prevBaseWidth_ = 1.0f;
    float baseHeight_ = 1.0f;
    float prevBaseHeight_ = 1.0f;
    FE::Vector4 baseGrassColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
};