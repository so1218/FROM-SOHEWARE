#pragma once
#include <memory>

#include "Engine.h"
#include "Camera.h"
#include "CollisionManager.h"
#include "GameObjectManager.h"
#include "ParticleSystemWrapper.h"

class BaseScene
{
public:
    BaseScene(Engine* engine, Camera* camera)
        : engine_(engine), camera_(camera)
    {
        collisionManager_ = std::make_unique<CollisionManager>();
        particleSystemWrapper_ = std::make_unique<ParticleSystemWrapper>(engine, camera);

        objectManager_.AddObject(std::move(particleSystemWrapper_));
    }
    virtual ~BaseScene() = default;

    virtual void Initialize() final
    {
        // 共通の初期化
        OnInitialize();
        objectManager_.Initialize();
    }

    virtual void Update() final
    {
        // シーンごとの独自処理
        OnUpdate();

        // 衝突判定
        HandleCollisions();

        // 全オブジェクト更新
        objectManager_.Update();
    }

    virtual void Draw() final
    {
        objectManager_.Draw();
        OnDraw();
    }

    virtual void DebugDraw() final
    {
        objectManager_.DebugDraw();
        OnDebugDraw();
    }

    virtual void Finalize() final
    {
        OnFinalize();
    }

    // SceneManagerをセット
    virtual void SetSceneManager(class SceneManager* sceneManager) { sceneManager_ = sceneManager; }

protected:
    virtual void OnInitialize() {}
    virtual void OnUpdate() {}
    virtual void OnDraw() {}
    virtual void OnDebugDraw() {}
    virtual void OnFinalize() {}

    void HandleCollisions()
    {
        collisionManager_->ClearColliders();
        objectManager_.AddAllCollidersToManager(collisionManager_.get());
        collisionManager_->CheckAllCollisions();
    }

protected:
    // メンバ変数
    Engine* engine_ = nullptr;
    Camera* camera_ = nullptr;
    SceneManager* sceneManager_ = nullptr;

    GameObjectManager objectManager_;
    std::unique_ptr<CollisionManager> collisionManager_;
    std::unique_ptr<ParticleSystemWrapper> particleSystemWrapper_;
};

