#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "PropertyBinder.h"
#include "GrassSystem.h"

class Player;

class GrassField : public GameObject
{
public:
    GrassField(Engine* engine, Player* player);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

private:
    // 草を指定数、ランダムに再配置する関数
    void GenerateGrass();

private:
    Player* player_;

    std::unique_ptr<GrassSystem> grassSystem_;
    std::unique_ptr<PropertyBinder> binder_;

    // 配置用のパラメータ
    int grassCount_ = 3000;         // 配置する数
    float spreadRadius_ = 20.0f;    // 配置する半径

    // 全体のスケール
    float baseScale_ = 1.0f;

    // 再生成（変更検知）用の記憶変数
    Vector3 prevPosition_;
    float prevBaseScale_ = 1.0f;

	WorldTransform transform_;
};