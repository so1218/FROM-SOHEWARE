#pragma once
#include "Matrix4x4.h"
#include "Vector3.h" 

class Engine;
class ShadowMap;
class PostEffectManager;
class RenderCoordinator;
class CommandManager;
class RendererManager;
class SRVManager;
class RTVManager;
class SwapChain;
class RenderContext;
class DebugGuiManager;

// カメラ情報をまとめる構造体
struct RenderCameraState 
{
    Matrix4x4 view;
    Matrix4x4 projection;
    Vector3 eyePos;
};

class RenderPipeline
{
public:
    RenderPipeline();
    ~RenderPipeline();

    // 初期化
    void Initialize(Engine* engine,
        D3D12_CPU_DESCRIPTOR_HANDLE mainDsvHandle,
        D3D12_CPU_DESCRIPTOR_HANDLE offscreenDsvHandle);

    // エンジンから毎フレーム呼ばれる描画の元締め
    void Render(Engine* engine,
        RendererManager* rendererManager,
        CommandManager* commandManager,
        const RenderCameraState& cameraState);

    // Engine側からアクセスが必要な場合のゲッター
    ShadowMap* GetShadowMap() const { return shadowMap_.get(); }
    PostEffectManager* GetPostEffectManager() const { return postEffectManager_.get(); }
    RenderCoordinator* GetRenderCoordinator() const { return renderCoordinator_.get(); }

private:
    // 描画手順に特化したマネージャー群
    std::unique_ptr<ShadowMap> shadowMap_;
    std::unique_ptr<PostEffectManager> postEffectManager_;
    std::unique_ptr<RenderCoordinator> renderCoordinator_;
};
