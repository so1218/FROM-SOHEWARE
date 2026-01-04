#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <wrl/client.h> 
#include <d3d12.h>

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

#include "Mesh.h"
#include "WorldTransform.h"
#include "RenderCommon.h" 
#include "BlendMode.h" 
#include "MaterialManager.h"
#include "TextureManager.h"
#include "AnimationData.h"
#include "Structures.h"
#include "ParticleDefinition.h"
#include "ShadowMap.h"

class Renderer
{
public:
    Renderer();
    ~Renderer();

    // Engineから必要なコンポーネントを受け取る
    void Initialize(
        GraphicsDevice* device,
        CommandManager* commandManager,
        PSOManager* psoManager,
        RootSignatureManager* rootSignatureManager,
        TextureManager* textureManager,
        SRVManager* srvManager,
        LightManager* lightManager,
        GlobalConstants* globalConstants,
        MaterialManager* materialManager,
        Camera* camera,
        PostEffectManager* postEffectManager,
        int clientWidth, int clientHeight, ShadowMap* shadowMap
    );
    void Finalize();

    // フレーム処理
    void BeginFrame(); // 描画カウンターのリセットなど

    // テクスチャ読み込み
    int LoadTexture(const std::string& texturePath);
    void LoadTextureArray(const std::vector<std::string>& texturePaths);

    // 描画関数
    void SubmitModel(WorldTransform& worldTransform, Camera& camera, ModelData& modelData,
        uint32_t textureHandle, uint32_t envMapSrvHandle, uint32_t toonRampHandle, uint32_t color, MaterialHandle& materialHandle, BlendMode blendMode,
        bool enableOutline, float outlineWidth, const Vector4& outlineColor, RenderGroup group);
    void DrawSkeleton(const Skeleton& skeleton, Camera& camera, uint32_t color);
    void SubmitAnimationModel(WorldTransform& worldTransform, Camera& camera,
        const AnimatedModelData& instance, const SkinCluster& skinCluster,
        uint32_t textureHandle, uint32_t envMapSrvHandle, uint32_t toonRampHandle, uint32_t color,
        MaterialHandle& materialHandle, bool enableOutline, float outlineWidth, const Vector4& outlineColor,
        RenderGroup group);
    void SubmitGrid(WorldTransform& worldTransform, Camera& camera, ModelData& modelData, uint32_t textureHandle, uint32_t color, MaterialHandle& materialHandle);
    void SubmitSprite(Vector2 position, Vector2 size, float rotation, uint32_t color, const Vector2& anchorPoint, WorldTransform& uvTransform, uint32_t textureHandle, int layerOrder, MaterialHandle& materialHandle);
    void SubmitLine(const Vector3& start, const Vector3& end, Camera& camera, uint32_t color);
    void SubmitParticleInstance(WorldTransform& worldTransform, uint32_t color, uint32_t textureIndex, float rotationZ,
        BlendMode blendMode, bool isBillboard, float intensity);
    void SubmitSkybox(Camera& camera, WorldTransform& worldTransform, uint32_t color, uint32_t cubeTextureSrvIndex);
    void SubmitTrail(const std::vector<TrailPoint>& points, const TrailModule& config, Camera& camera);
    void DrawFullScreenQuadWithOffscreenTexture();
    // 単純にテクスチャをそのまま画面に出すメソッド
    void DrawFinalResult(uint32_t srvIndex);
    void DrawSceneForShadow();
    void DrawScene();

    // ブレンドモード設定
    void SetBlendMode(BlendMode blendMode) { currentBlendMode_ = blendMode; }
    // BlendMode (enum) を PSO名 (string) に変換するヘルパー
    std::string GetParticlePSOName(BlendMode mode);

    // メッシュキャッシュ取得・作成 
    Mesh* GetOrCreateMesh(const ModelData& modelData);

    // 描画カウント取得
    int32_t GetModelCount() const { return indexModel_; }
    int32_t GetSpriteCount() const { return indexSprite_; }
    int32_t GetLineCount() const { return indexLine_; }
    int32_t GetParticleCount() const { return indexParticle_; }
    int32_t GetTrailCount() const { return indexTrail_; }

    // Trail用のレンダリングデータ構造体
    struct TrailRenderData
    {
        Mesh mesh; // 動的頂点バッファを持つメッシュ
        Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource; // 行列バッファ
        TransformationMatrix* mappedWvp = nullptr; // 行列マップ用ポインタ
        Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
        TrailMaterialData* mappedMaterial = nullptr;
    };

    // デバッグ用
    void SetWireFrame(bool isWireFrame) { isWireFrame_ = isWireFrame; }

    BlendMode currentBlendMode_ = kBlendModeNormal;

    // 描画可能な最大数
    static const int32_t kMaxModelCount;
    static const int32_t kMaxSpriteCount;
    static const int32_t kMaxLineCount;
    static const int32_t kMaxLineVertices;
    static const int32_t kMaxParticleCount;
    static const int32_t kMaxTrailCount;
    static const int32_t kMaxTrailVertices;// 1つのトレイルの最大頂点数

    bool isWireFrame_ = false;

private:
    // 描画用オブジェクト作成処理
    void CreateObjects();
    void CreateModels();
    void CreateSprites();
    void CreateLineBatch();
    void CreateParticles();
    void CreateSkybox();
    void CreateTrails();

    // 実際の描画コマンド発行を行う内部関数
    void DrawSprite(const ModelSubmission& sub);
    void DrawModel(const ModelSubmission& sub);
    void DrawGrid(const ModelSubmission& sub);
    void FlushLines(Camera& camera);
    void DrawParticles(const Camera& camera);
    void DrawSkybox(const ModelSubmission& sub);
    void DrawTrails(const Camera& camera);

private:
    // Engineから受け取るポインタ
    GraphicsDevice* device_ = nullptr;
    CommandManager* commandManager_ = nullptr;
    PSOManager* psoManager_ = nullptr;
    RootSignatureManager* rootSignatureManager_ = nullptr;
    TextureManager* textureManager_ = nullptr;
    SRVManager* srvManager_ = nullptr;
    LightManager* lightManager_ = nullptr;
    GlobalConstants* globalConstants_ = nullptr;
    MaterialManager* materialManager_ = nullptr;
    Camera* camera_ = nullptr;
    PostEffectManager* postEffectManager_ = nullptr;
    ShadowMap* shadowMap_ = nullptr;

    // 描画インデックスと描画情報（各プリミティブ）
    uint32_t indexModel_ = 0;
    std::vector<RenderData> models_;
    std::unordered_map<const ModelData*, size_t> modelDataToIndex_;
    std::unordered_map<const ModelData*, Mesh> meshCache;
    // 描画リクエストを貯めるリスト
    std::vector<ModelSubmission> modelSubmissions_;
    // 定数バッファリソースの配列
    struct PerObjectBuffer {
        Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource;
        TransformationMatrix* wvpMapped = nullptr;

        Microsoft::WRL::ComPtr<ID3D12Resource> outlineResource;
        OutlineData* outlineMapped = nullptr;
    };
    std::vector<PerObjectBuffer> perObjectBuffers_;

    uint32_t indexSprite_ = 0;
    std::vector<RenderData> sprites_;

    uint32_t indexLine_ = 0;
    std::vector<RenderData> lines_;
    // 線描画用のリソース
    struct LineBatchResource 
    {
        Mesh mesh; // 動的頂点バッファ用のメッシュ
        std::vector<LineVertex> verticesCPU; // CPU側の一時保管場所

        // WVP行列は全ての線で共通なので1つ
        Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource;
        TransformationMatrix* mappedWvp = nullptr;
    } lineBatch_;

    uint32_t indexParticle_ = 0;
    std::vector<RenderData> particles_;
    std::vector<ParticleInstanceData> instanceData_;
    int indexInstance_ = 0;
    Mesh particleMesh_;
    bool hasParticles_ = false;

    // 定数フレームバッファ数
    static constexpr int kFrameCount = 3;

    // パーティクルインスタンスバッファ（フレーム毎）
    Microsoft::WRL::ComPtr<ID3D12Resource> particleInstanceBuffer_[kFrameCount];
    ParticleInstanceData* mappedInstanceData_[kFrameCount] = {};
    int currentFrameIndex_ = 0;

    // テクスチャ配列関連
    D3D12_GPU_DESCRIPTOR_HANDLE textureArraySrvHandleGPU_{};
    std::vector<TextureManager::TextureResources> textures_;
    TextureManager::TextureResources textureArrayResource_; // Texture2DArray本体とSRVの管理用

    // 各テクスチャIDごとにParticleInstanceDataの配列を持つ
    std::map<BlendMode, std::map<uint32_t, std::vector<ParticleInstanceData>>> particleBatches_;

    // スカイボックス
    Mesh skyboxMesh_;
    Microsoft::WRL::ComPtr<ID3D12Resource> skyboxWvpResource_;
    TransformationMatrix* mappedSkyboxWvp_ = nullptr;
    MaterialHandle skyboxMaterialHandle_;

    // トレイル用のバッチ構造体
    // [New] 実際にSubmitされたトレイルの数を数えるカウンタ
    int32_t indexTrail_ = 0;
    struct TrailBatch
    {
        uint32_t startVertexIndex;  // このバッチの開始頂点インデックス
        uint32_t vertexCount;       // 頂点数
        uint32_t textureHandle;     // テクスチャ
        uint32_t dissolveHandle;    // ディゾルブテクスチャ
        TrailMaterialData materialData; // マテリアル設定（定数バッファ用）
    };
    // バッチ描画用リソース
    struct TrailBatchResource
    {
        Mesh mesh; // 巨大な動的頂点バッファ
        std::vector<VertexDataTrail> verticesCPU; // CPU側の一時バッファ

        // マテリアル用定数バッファ（描画直前に更新して使う）
        Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
        TrailMaterialData* mappedMaterial = nullptr;

        // WVP行列用（基本Identityで、頂点計算済み座標を使うなら不要だが、VP行列を渡すなら必要）
        Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource;
        TransformationMatrix* mappedWvp = nullptr;
    } trailBatch_;

    // 1フレーム中のバッチリスト
    std::vector<TrailBatch> trailBatches_;

    int clientWidth_ = 0;
    int clientHeight_ = 0;
};