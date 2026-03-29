#pragma once
#include "Engine.h"
#include "Camera.h"
#include "CollisionManager.h"
#include "GameObjectManager.h"
#include "ParticleSystemWrapper.h"

class BaseScene
{
public:
    BaseScene(Engine* engine)
        : engine_(engine)
    {
        collisionManager_ = std::make_unique<CollisionManager>();
        camera_ = std::make_unique<Camera>();

        objectManager_.Create<ParticleSystemWrapper>(engine);
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

        // 全オブジェクト更新
        objectManager_.Update();

        // 衝突判定
        HandleCollisions();

        // カメラの行列更新
        if (camera_)
        {
            camera_->UpdateViewMatrix();
        }
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

    // エンジンが情報を取りに来れるように
    Camera* GetActiveCamera() const { return camera_.get(); }

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
    std::unique_ptr<Camera> camera_;
    SceneManager* sceneManager_ = nullptr;

    GameObjectManager objectManager_;
    std::unique_ptr<CollisionManager> collisionManager_;
    std::unique_ptr<ParticleSystemWrapper> particleSystemWrapper_;
};

