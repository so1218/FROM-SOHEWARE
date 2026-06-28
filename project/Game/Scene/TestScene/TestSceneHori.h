#pragma once
#include "BaseScene.h"
#include "Player.h"
#include "FollowCamera.h"
#include "CameraRail.h"
#include "ParticleEmitter.h"
#include "Bubble.h"

class TestSceneHori : public FE::BaseScene
{
public:
    TestSceneHori(FE::Engine* engine);

    void OnInitialize() override;
    void OnUpdate() override;
    void OnDraw() override;
    void OnDebugDraw() override;
    void OnFinalize() override;

    void AddPlacedModel(std::unique_ptr<FE::Model> model)
    {
        placedModels_.push_back(std::move(model));
    }

    std::unique_ptr<FE::Model> RemovePlacedModel(FE::Model* targetPtr);

private:
    // メンバー変数
    Player* player_ = nullptr;
    Bubble* bubble_ = nullptr;
    std::unique_ptr<FollowCamera> followCamera_;
    std::unique_ptr<FE::CameraRail> openingRail_;

    std::unique_ptr<FE::ParticleEmitter> grassFieldEmitter_ = nullptr;

    // エディタ（ドラッグ＆ドロップ）で配置されたモデルのリスト
    std::vector<std::unique_ptr<FE::Model>> placedModels_;

    // ギズモの移動履歴記録用
    bool isGizmoUsingLastFrame_ = false;
    FE::Vector3 gizmoOldTranslation_ = { 0.0f, 0.0f, 0.0f };
    FE::Model* gizmoTargetModel_ = nullptr;
};

