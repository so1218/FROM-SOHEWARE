#pragma once
#include "GameObject.h"
#include "Model.h"
#include "Collider.h"
#include "Ammo.h"
#include "PropertyBinder.h"

class AmmoManager : public FE::GameObject
{
public:
    AmmoManager(FE::Engine* engine, const std::string& groupName);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

    void AddAmmo();

private:
    FE::Engine* engine_ = nullptr;
    std::string managerGroupName_;
    std::vector<std::unique_ptr<Ammo>> ammo_;
    std::unique_ptr<FE::Model> sharedModel_;
    std::unique_ptr<FE::Model> sharedBubbleModel_;
    std::unique_ptr<FE::PropertyBinder> binder_;

    int ammoCount_ = 0;

    // 共通パラメータ
    FE::Vector3 sharedScale_ = { 1.0f, 1.0f, 1.0f };
    FE::Vector4 sharedLightColor_ = { 0.2f, 0.6f, 1.0f, 1.0f };
    float tiltAngle_ = 15.0f;       // 傾き角度
    float rotationSpeed_ = 2.0f;    // Y軸回転スピード
};