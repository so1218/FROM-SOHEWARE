#pragma once
#include "GameObject.h"
#include "Model.h"
#include "Collider.h"
#include "PropertyBinder.h"

class Orb : public FE::GameObject
{
	Orb(FE::Engine engine_);

public:
    Orb(FE::Engine* engine, int id);
    ~Orb() override;

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

    // 衝突時のコールバック
    void OnCollisionStay(FE::Collider* mine, FE::Collider* other) override;

private:
    FE::Engine* engine_;
    std::unique_ptr<FE::Model> model_;       // オーブの見た目
    std::unique_ptr<FE::Collider> collider_; // オーブの当たり判定
    std::unique_ptr<FE::PropertyBinder> binder_;

    int id_;

    // ポイントライト
    int pointLightIndex_ = -1;
    FE::Vector4 lightColor_ = { 0.2f, 0.6f, 1.0f, 1.0f };
    float lightIntensity_ = 5.0f;
    float lightRadius_ = 10.0f;
    float lightVolumetricScatteringIntensity_ = 1.0f;
};
