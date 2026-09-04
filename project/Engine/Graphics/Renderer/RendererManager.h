#pragma once

#include "Mesh.h"
#include "RenderCommon.h" 
#include "BlendMode.h" 
#include "TextureLoader.h"
#include "Structures.h"
#include "ShadowMap.h"
#include "RenderEnvironment.h"
#include "ParticleDefinition.h"
#include "Terrain.h"
#include "FoliageRenderer.h"

namespace FE
{

// インタラクションデータ保持用の構造体
struct WorldInteractionData 
{
    uint32_t srvIndex = 0;
    float worldSize = 40.0f;
    Vector2 centerWorldPos = { 0.0f, 0.0f };
    D3D12_GPU_VIRTUAL_ADDRESS cbAddress = 0;
};

class Camera;
class ModelRenderer;
class SpriteRenderer;
class LineRenderer;
class ParticleRenderer;
class TrailRenderer;
class SkyboxRenderer;
class GrassRenderer;
class TreeRenderer;
class PebbleRenderer;
class SkydomeRenderer;
class TerrainRenderer;
class TerrainChunk;
class LightningRenderer;

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
    void UpdateCullingFrustums();

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
    void SubmitTrail(const std::vector<TrailPoint>& points, const TrailModule& config,
        float instanceSeed);
    void GenerateGrass(
        const GrassGenerationData& genData,
        uint32_t heightMapSrvHandle,
        uint32_t densityMapSrvHandle);
    void SubmitTree(const WorldTransform& worldTransform, const ModelData& modelData,
        const TreeMaterialHandle& treeMaterial, const Vector4& colorVariation, float lodFade);
    void GeneratePebbles(
        const PebbleGenerationData& genData,
        uint32_t heightMapSrvHandle,
        uint32_t densityMapSrvHandle);
    void SubmitSkydome(const WorldTransform& worldTransform, uint32_t color, uint32_t cloudNoiseSrvIndex, const AtmosphereSkyData& weather);
    void SubmitTerrain(const WorldTransform& worldTransform, const TerrainChunk* chunk,
        const Vector4& uvTransform,
        const MaterialHandle& material, const Vector4& instanceColor,
        const Terrain::Parameters& params, uint32_t heightMapHandle);
    void DrawFullScreenQuadWithOffscreenTexture();
    // テクスチャをそのまま画面に出すメソッド
    void DrawFinalResult(uint32_t srvIndex);
    void DrawSceneForShadow(uint32_t cascadeIndex);
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

    TreeRenderer* GetTreeRenderer() const { return treeRenderer_.get(); }
    const Frustum& GetCameraFrustum() const { return cameraFrustum_; }

    // デバッグ用
    void SetWireFrame(bool isWireFrame) { isWireFrame_ = isWireFrame; }

    BlendMode currentBlendMode_ = kBlendModeNormal;

    bool isWireFrame_ = false;

    void InitializeGrass();
    void SetGrassRenderingParams(uint32_t windMapHandle, const GrassMaterialData& materialData, const GrassCullingData& cullingData);

    void InitializePebble();
    void SetPebbleRenderingParams(
        uint32_t skyboxSrvHandle,
        uint32_t albedoSrvHandle,
        uint32_t normalSrvHandle,
        const Mesh& pebbleMesh,
        const PebbleMaterialData& materialData,
        const PebbleCullingData& cullingData);

    // 初期化 (構成のリストを渡す)
    void InitializeFoliage(const std::vector<FoliageTypeConfig>& configs);
    // 毎フレームのカリング設定などを渡す
    void SetFoliageRenderingParams(const FoliageCullingData& cullingData);
    void UpdateFoliageConfigs(const std::vector<FoliageTypeConfig>& configs);
    // 地形生成・配置
    void GenerateFoliage(
        uint32_t heightMapSrvHandle,
        uint32_t terrainWidth, uint32_t terrainDepth);

    void SetWindMap(uint32_t windMapSrvIndex) { windMapSrvIndex_ = windMapSrvIndex; }

    // 雷を発生させる
    void SpawnLightning(const Vector3& start, const Vector3& end, float duration);
    void UpdateLightnings();

    void SetLightningConfig(const LightningConfig& config);

    // 地形ハイトマップSRVインデックスのゲッター
    uint32_t GetTerrainHeightMapSRVIndex() const { return terrainHeightMapSrvIndex_; }

    // ワールドインタラクションデータの設定
    void SetWorldInteractionData(uint32_t srvIndex, float worldSize, const Vector2& centerPos, D3D12_GPU_VIRTUAL_ADDRESS cbAddress)
    {
        interactionData_.srvIndex = srvIndex;
        interactionData_.worldSize = worldSize;
        interactionData_.centerWorldPos = Vector2(centerPos.x, centerPos.y);
        interactionData_.cbAddress = cbAddress;
    }
    // インタラクション対象の登録
    void SubmitInteractionEntity(const InteractionEntity& entity) {
        if (interactionEntities_.size() < 64) { // kMaxEntities 上限チェック
            interactionEntities_.push_back(entity);
        }
    }
    // エンティティリストのゲッター (Scene等から集約したリストを返す)
    const std::vector<InteractionEntity>& GetInteractionEntities() const { return interactionEntities_; }
    void SetInteractionEntities(const std::vector<InteractionEntity>& entities) { interactionEntities_ = entities; }
    // ワールドインタラクションパラメータの送信
    void SubmitWorldInteractionParams(const InteractionConstants& constants) {
        interactionConstants_ = constants;
    }
    // パス参照用ゲッター
    const InteractionConstants& GetWorldInteractionConstants() const { return interactionConstants_; }
    uint32_t GetWorldInteractionSRVIndex() const { return interactionData_.srvIndex; }
    // ゲーム側から追従対象の座標を受け取る関数
    void SetWorldInteractionCenter(const Vector2& center) { interactionCenter_ = center; }
    Vector2 GetWorldInteractionCenter() const { return interactionCenter_; }

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
    Vector3 cameraPosition_;

    uint32_t grassTextureHandle_ = 0;
    GrassMaterialData grassMaterialData_ = {};
    GrassCullingData grassCullingData_{};

    uint32_t windMapSrvIndex_ = 0;

    uint32_t pebbleSkyboxSrvHandle_ = 0;
    uint32_t pebbleAlbedoSrvHandle_ = 0;
    uint32_t pebbleNormalSrvHandle_ = 0;
    Mesh pebbleMesh_;
    PebbleMaterialData pebbleMaterialData_ = {};
    PebbleCullingData pebbleCullingData_ = {};

    FoliageCullingData foliageCullingData_ = {};

    // カリング用のキャッシュ
    Frustum cameraFrustum_;
    std::vector<Frustum> shadowFrustums_;

    WorldInteractionData interactionData_{};
    uint32_t terrainHeightMapSrvIndex_ = 0;
    std::vector<InteractionEntity> interactionEntities_;
    InteractionConstants interactionConstants_{};
    Vector2 interactionCenter_{ 0.0f, 0.0f }; // デフォルト値

    // 描画インデックスと描画情報（各プリミティブ）
    RenderEnvironment env_; // 各種マネージャーのポインタをまとめた構造体
    std::unique_ptr<ModelRenderer> modelRenderer_;
    std::unique_ptr<SpriteRenderer> spriteRenderer_;
    std::unique_ptr<LineRenderer> lineRenderer_;
    std::unique_ptr<ParticleRenderer> particleRenderer_;
    std::unique_ptr<TrailRenderer> trailRenderer_;
    std::unique_ptr<SkyboxRenderer> skyboxRenderer_;
    std::unique_ptr<GrassRenderer> grassRenderer_;
    std::unique_ptr<TreeRenderer> treeRenderer_;
    std::unique_ptr<PebbleRenderer> pebbleRenderer_;
    std::unique_ptr<FoliageRenderer> foliageRenderer_;
    std::unique_ptr<SkydomeRenderer> skydomeRenderer_;
    std::unique_ptr<TerrainRenderer> terrainRenderer_;
    std::unique_ptr<LightningRenderer> lightningRenderer_;
};

}