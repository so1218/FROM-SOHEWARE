#pragma once
#include "Engine.h"
#include "Camera.h"
#include "CollisionManager.h"
#include "CameraManager.h"
#include "GameObjectManager.h"
#include "ParticleSystemWrapper.h"
#include "PropertyBinder.h"

namespace FS
{

class BaseScene
{
public:
    BaseScene(Engine* engine, const std::string& sceneName)
        : engine_(engine), sceneName_(sceneName)
    {
        collisionManager_ = std::make_unique<CollisionManager>();
        objectManager_.SetCollisionManager(collisionManager_.get());

        camera_ = std::make_unique<Camera>();
        cameraManager_ = std::make_unique<CameraManager>(camera_.get());

        objectManager_.Create<ParticleSystemWrapper>(engine);

        binder_ = std::make_unique<FS::PropertyBinder>(engine_, sceneName_);
    }
    virtual ~BaseScene() = default;

    virtual void Initialize() final
    {

        // 共通の初期化
        OnInitialize();

        if (camera_)
        {
            camera_->BindProperties(*binder_, "Camera");
        }
        if (engine_->GetPostEffectManager())
        {
            engine_->GetPostEffectManager()->BindProperties(*binder_, "PostEffect");
        }
        if (engine_->GetLightManager())
        {
            engine_->GetLightManager()->BindProperties(*binder_, "Light");
        }

        objectManager_.Initialize();
    }

    virtual void Update() final
    {
        // シーンごとの独自処理
        OnUpdate();

        // 全オブジェクト更新
        objectManager_.Update();

        // 衝突判定
        collisionManager_->CheckAllCollisions();

        // カメラを更新
        if (cameraManager_)
        {
            cameraManager_->Update();
        }

        // カメラの行列更新
        if (camera_)
        {
            camera_->UpdateViewMatrix();
        }
    }

    virtual void Draw() final
    {
        objectManager_.Draw();
        if (isColliderVisible_)
        {
            collisionManager_->DrawColliders();
        }
        if (cameraManager_)
        {
            cameraManager_->Draw();
        }
        OnDraw();
    }

    virtual void DebugGui() final
    {
        objectManager_.DebugGui();
        if (cameraManager_)
        {
            cameraManager_->DebugGui();
        }

        ImGui::Begin(sceneName_.c_str());
        if (camera_) 
        {
            camera_->DebugGui(*binder_);
        }
        if (engine_->GetPostEffectManager())
        {
            engine_->GetPostEffectManager()->DebugGui(*binder_);
        }
        if (engine_->GetLightManager())
        {
            engine_->GetLightManager()->DebugGui(*binder_);
        }
        ImGui::End();

        OnDebugGui();
    }

    virtual void Finalize() final
    {
        if (collisionManager_)
        {
            collisionManager_->ResetCollisionHistory();
        }

        if (engine_ && engine_->GetRendererManager())
        {
            engine_->GetRendererManager()->ClearSceneRenderStates();
        }

        OnFinalize();
    }

    // SceneManagerをセット
    virtual void SetSceneManager(class SceneManager* sceneManager) { sceneManager_ = sceneManager; }

    // シーン名のゲッター
    const std::string& GetSceneName() const { return sceneName_; }

    // エンジンが情報を取りに来れるように
    Camera* GetActiveCamera() const { return camera_.get(); }

protected:
    virtual void OnInitialize() {}
    virtual void OnUpdate() {}
    virtual void OnDraw() {}
    virtual void OnDebugGui() {}
    virtual void OnFinalize() {}

protected:
    // メンバ変数
    Engine* engine_ = nullptr;
    std::string sceneName_;
    std::unique_ptr<Camera> camera_;
    SceneManager* sceneManager_ = nullptr;

    std::unique_ptr<CollisionManager> collisionManager_;
    GameObjectManager objectManager_;
    std::unique_ptr<CameraManager> cameraManager_;
    std::unique_ptr<PropertyBinder> binder_;

    bool isColliderVisible_ = true;
};

}
