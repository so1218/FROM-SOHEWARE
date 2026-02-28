#pragma once

class GraphicsDevice;
class CommandManager;
class PSOManager;
class RootSignatureManager;
class SRVManager;
class LightManager;
class GlobalConstants;
class MaterialManager;
class Camera;
class PostEffectManager;
class ModelRenderer;
class SpriteRenderer;
class LineRenderer;
class ParticleRenderer;
class TrailRenderer;
class SkyboxRenderer;

#include "Mesh.h"
#include "RenderCommon.h" 
#include "BlendMode.h" 
#include "TextureLoader.h"
#include "Structures.h"
#include "ShadowMap.h"
#include "RenderEnvironment.h"
#include "ParticleDefinition.h"

class RendererManager
{
public:
    RendererManager();
    ~RendererManager();

    void Initialize(
        GraphicsDevice* device,
        CommandManager* commandManager,
        PSOManager* psoManager,
        RootSignatureManager* rootSignatureManager,
        TextureLoader* textureLoader,
        SRVManager* srvManager,
        LightManager* lightManager,
        GlobalConstants* globalConstants,
        MaterialManager* materialManager,
        PostEffectManager* postEffectManager,
        int clientWidth, int clientHeight, ShadowMap* shadowMap
    );
    void Finalize();

    // フレーム処理
    void BeginFrame(); // 描画カウンターのリセットなど

    // このフレームで使うカメラ行列をセットする
    void SetCameraState(const Matrix4x4& view, const Matrix4x4& projection, const Vector3& cameraPosition);

    // テクスチャ読み込み
    int LoadTexture(const std::string& texturePath);

    // 描画関数
    void SubmitModel(const WorldTransform& worldTransform, const ModelData& modelData,
        const std::vector<MaterialHandle>& materials, BlendMode blendMode, CullMode cullMode,
        DepthMode depthMode, RenderGroup group, const Vector4& instanceColor);
    void SubmitAnimationModel(const WorldTransform& worldTransform,
        const AnimatedModelData& instance, const SkinCluster& skinCluster,
        const std::vector<MaterialHandle>& materials, BlendMode blendMode,
        RenderGroup group, const Vector4& instanceColor);
    void SubmitSprite(const Vector2 position, const Vector2 size, float rotation, uint32_t color, const Vector2& anchorPoint, const WorldTransform& uvTransform, uint32_t textureHandle,
        uint32_t dissolveTextureHandle, int layerOrder, const MaterialHandle& materialHandle);
    void SubmitLine(const Vector3& start, const Vector3& end, uint32_t color);
    void SubmitParticleInstance(const WorldTransform& worldTransform, uint32_t color, uint32_t textureIndex, float rotationZ,
        BlendMode blendMode, bool isBillboard, float intensity);
    void SubmitSkybox(const WorldTransform& worldTransform, uint32_t color, uint32_t cubeTextureSrvIndex);
    void SubmitTrail(const std::vector<TrailPoint>& points, const TrailModule& config);
    void DrawFullScreenQuadWithOffscreenTexture();
    // テクスチャをそのまま画面に出すメソッド
    void DrawFinalResult(uint32_t srvIndex);
    void DrawSceneForShadow();
    void Draw3D();
    void DrawUI();

    // ブレンドモード設定
    void SetBlendMode(BlendMode blendMode) { currentBlendMode_ = blendMode; }

    // 描画カウント取得
    uint32_t GetModelCount() const;
    uint32_t GetSpriteCount() const;
    uint32_t GetLineCount() const;
    uint32_t GetParticleCount() const;
    uint32_t GetTrailCount() const;

    uint32_t GetMaxModelCount() const;
    uint32_t GetMaxSpriteCount() const;
    uint32_t GetMaxLineCount() const;
    uint32_t GetMaxParticleCount() const;
    uint32_t GetMaxTrailCount() const;

    // デバッグ用
    void SetWireFrame(bool isWireFrame) { isWireFrame_ = isWireFrame; }

    BlendMode currentBlendMode_ = kBlendModeNormal;

    bool isWireFrame_ = false;

private:
    // Engineから受け取るポインタ
    GraphicsDevice* device_ = nullptr;
    CommandManager* commandManager_ = nullptr;
    PSOManager* psoManager_ = nullptr;
    RootSignatureManager* rootSignatureManager_ = nullptr;
    TextureLoader* textureLoader_ = nullptr;
    SRVManager* srvManager_ = nullptr;
    LightManager* lightManager_ = nullptr;
    GlobalConstants* globalConstants_ = nullptr;
    MaterialManager* materialManager_ = nullptr;
    PostEffectManager* postEffectManager_ = nullptr;
    ShadowMap* shadowMap_ = nullptr;

    // 現在設定されているカメラ行列
    Matrix4x4 viewMatrix_;
    Matrix4x4 projectionMatrix_;
    Matrix4x4 viewProjectionMatrix_;
    // カメラのワールド座標
    Vector3 cameraPosition_;

    // 描画インデックスと描画情報（各プリミティブ）
    RenderEnvironment env_; // 各種マネージャーのポインタをまとめた構造体
    std::unique_ptr<ModelRenderer> modelRenderer_;
    std::unique_ptr<SpriteRenderer> spriteRenderer_;
    std::unique_ptr<LineRenderer> lineRenderer_;
    std::unique_ptr<ParticleRenderer> particleRenderer_;
    std::unique_ptr<TrailRenderer> trailRenderer_;
    std::unique_ptr<SkyboxRenderer> skyboxRenderer_;

    // 1バッチごとの管理
    struct GrassBatch
    {
        const ModelData* modelData;
        MaterialHandle materialHandle;
        uint32_t instanceCount = 0;
        uint32_t maxInstanceCount = 0;
        Microsoft::WRL::ComPtr<ID3D12Resource> instanceBuffer;
        GrassInstanceData* mappedData = nullptr;
        uint32_t srvIndex = 0; 
    };
    std::vector<GrassBatch> grassBatches_;

};