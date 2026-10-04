#pragma once
#include "GameObject.h"
#include "Model.h"
#include "Collider.h"
#include "Orb.h"
#include "PropertyBinder.h"

class OrbManager : public FE::GameObject
{
public:
    OrbManager(FE::Engine* engine, const std::string& groupName);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

    void AddOrb();

private:
    FE::Engine* engine_ = nullptr;
    std::string managerGroupName_;
    std::vector<std::unique_ptr<Orb>> orbs_;
    std::unique_ptr<FE::Model> sharedModel_;
    std::unique_ptr<FE::Model> sharedBubbleModel_;
    std::unique_ptr<FE::PropertyBinder> binder_;

    int orbCount_ = 0;

    // 共通パラメータ
    FE::Vector3 sharedScale_ = { 1.0f, 1.0f, 1.0f };
    FE::Vector4 sharedLightColor_ = { 0.2f, 0.6f, 1.0f, 1.0f };
    float tiltAngle_ = 15.0f;       // 傾き角度
    float rotationSpeed_ = 2.0f;    // Y軸回転スピード
};