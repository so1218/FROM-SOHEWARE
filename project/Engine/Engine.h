#pragma once

#include "Window.h"
#include "SwapChain.h"
#include "RTVManager.h"
#include "DescriptorManager.h"
#include "RenderContext.h"
#include "RenderCoordinator.h"
#include "RootSignatureManager.h"
#include "PSOManager.h"
#include "MaterialManager.h"
#include "TextureManager.h"
#include "Mesh.h"
#include "LightManager.h"
#include "WorldTransform.h"
#include "Camera.h"
#include "DebugCamera.h"
#include "RenderCommon.h"
#include "debugGuiManager.h"
#include "Particle.h"
#include "CameraManager.h" 
#include "PostEffectManager.h" 
#include "AnimationLoader.h" 

constexpr int32_t kClientWidth = 1280;
constexpr int32_t kClientHeight = 720;

// デバッグ描画切り替えフラグ
constexpr bool useDebugView = true;

class Engine
{
public:
    ~Engine() {}

    // 初期化・終了
    void Initialize(Camera* camera, MaterialManager* materialManager);
    void Finalize();

    // フレーム処理
    void BeginFrame();
    void PreDraw();
    void EndFrame();

    // テクスチャ読み込み
    int LoadTexture(const std::string& texturePath);
    void LoadTextureArray(const std::vector<std::string>& texturePaths);

    // 描画カウンターをリセット（毎フレーム呼ぶ）
    void ResetDrawCounters();

    // 描画コマンド
    void DrawTriangle(WorldTransform& worldTransform, uint32_t color, WorldTransform& uvTransform, uint32_t textureHandle);
    void DrawSphere(WorldTransform& worldTransform, Camera& camera, WorldTransform& uvTransform, uint32_t textureHandle, uint32_t color = 0xffffffff);
    void DrawModel(WorldTransform& worldTransform, Camera& camera, ModelData& modelData, uint32_t textureHandle, uint32_t color = 0xffffffff);
    void DrawModel(WorldTransform& worldTransform, Camera& camera, ModelData& modelData, uint32_t textureHandle, uint32_t color
        , const WorldTransform& uvTransform);
    void DrawSkeleton(const Skeleton& skeleton, Camera& camera, uint32_t color);
    void DrawAnimatedModel(WorldTransform& worldTransform, Camera& camera, const AnimatedModelData& instance, const SkinCluster& skinCluster, uint32_t textureHandle, uint32_t color = 0xffffffff);
    void DrawGrid(WorldTransform& worldTransform, Camera& camera, ModelData& modelData, uint32_t textureHandle, uint32_t color = 0xffffffff);
    void DrawSprite(Vector2 position, Vector2 size, float rotation, uint32_t color, WorldTransform& uvTransform, uint32_t textureHandle);
    void DrawCube(WorldTransform& worldTransform, uint32_t color, WorldTransform& uvTransform, uint32_t textureHandle);
    void DrawLine(const Vector3& start, const Vector3& end, Camera& camera, uint32_t color);
    void DrawParticles(const Camera& camera);
    void SubmitParticleInstance(WorldTransform& worldTransform, uint32_t color, uint32_t textureIndex, float rotationZ);
    void DrawFullScreenQuadWithOffscreenTexture();

    // ブレンドモード設定
    void SetBlendMode(BlendMode blendMode) { currentBlendMode_ = blendMode; }

    // メッシュキャッシュ取得・作成
    Mesh* GetOrCreateMesh(const ModelData& modelData);

    // 描画カウント取得
    int32_t GetTriangleCount() const { return indexTriangle_; }
    int32_t GetSphereCount() const { return indexSphere_; }
    int32_t GetModelCount() const { return indexModel_; }
    int32_t GetSpriteCount() const { return indexSprite_; }
    int32_t GetCubeCount() const { return indexCube_; }
    int32_t GetLineCount() const { return indexLine_; }
    int32_t GetParticleCount() const { return indexParticle_; }

public:

    // 描画可能な最大数
    static const int32_t kMaxTriangleCount;
    static const int32_t kMaxSphereCount;
    static const int32_t kMaxModelCount;
    static const int32_t kMaxSpriteCount;
    static const int32_t kMaxCubeCount;
    static const int32_t kMaxLineCount;
    static const int32_t kMaxParticleCount;

private:
    // 各種初期化処理
    void InitializeSystem();
    void InitializeWindow();
    void InitializeInput();
    void InitializeGraphics();
    void InitializeRenderer();
    void InitializeResources();
    void InitializeImGui();
    void InitializeAudio();

    // 描画用オブジェクト作成処理
    void CreateObjects();
    void CreateTriangles();
    void CreateSpheres();
    void CreateModels();
    void CreateSprites();
    void CreateCubes();
    void CreateLines();
    void CreateParticles();

public:
    uint64_t fenceValue_ = 0; // GPU同期用フェンス値

    // システム関連オブジェクト
    std::unique_ptr<Window> window_;
    std::unique_ptr<GraphicDevice> graphicDevice_;
    std::unique_ptr<CommandManager> commandManager_;
    std::unique_ptr<SwapChain> swapChain_;
    std::unique_ptr<RTVManager> rtvManager_;
    std::unique_ptr<OffscreenRTVManager> offscreenRTVManager_;
    std::unique_ptr<DescriptorManager> descriptorManager_;
    std::unique_ptr<RenderContext> renderContext_;
    std::unique_ptr<RenderCoordinator> renderCoordinator_;
    std::unique_ptr<RootSignatureManager> rootSignatureManager_;
    std::unique_ptr<PSOManager> psoManager_;
    MaterialManager* materialManager_ = nullptr;
    std::unique_ptr<TextureManager> textureManager_;
    std::unique_ptr<SRVManager> srvManager_;
    std::unique_ptr<LightManager> lightManager_;
    std::unique_ptr<DebugCamera> debugCamera_;
    std::unique_ptr<DebugGuiManager> debugGuiManager_;
    std::unique_ptr<ParticleSystem> particleSystem_;
    std::unique_ptr<CameraManager> cameraManager_;
    std::unique_ptr<PostEffectManager> postEffectManager_;
    std::unique_ptr<SRVAllocator> srvAllocator_;
    Camera* camera_ = nullptr;

    // DirectX関連
    Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_;
    Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
    Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_;
    Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_;
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_;
    HANDLE fenceEvent_ = nullptr;

    uint32_t descriptorSizeSRV_ = 0;
    uint32_t descriptorSizeRTV_ = 0;
    uint32_t descriptorSizeDSV_ = 0;

    // 描画インデックスと描画情報（各プリミティブ）
    uint32_t indexTriangle_ = 0;
    std::vector<RenderData> triangles_;

    uint32_t indexSphere_ = 0;
    std::vector<RenderData> spheres_;

    uint32_t indexModel_ = 0;
    std::vector<RenderData> models_;
    std::unordered_map<const ModelData*, size_t> modelDataToIndex_;
    std::unordered_map<const ModelData*, Mesh> meshCache;

    uint32_t indexSprite_ = 0;
    std::vector<RenderData> sprites_;

    uint32_t indexCube_ = 0;
    std::vector<RenderData> cubes_;

    uint32_t indexLine_ = 0;
    std::vector<RenderData> lines_;
    std::vector<LineVertex> lineVertexBuffer_;

    uint32_t indexParticle_ = 0;
    std::vector<RenderData> particles_;
    std::vector<ParticleInstanceData> instanceData_;
    int indexInstance_ = 0;
    Mesh particleMesh_;

    bool isWireFrame_ = false;

    // 定数フレームバッファ数
    static constexpr int kFrameCount = 3;

    // GPU用カメラバッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> cameraBuffer_;
    CameraBuffer* mappedCamera_ = nullptr;

    // パーティクルインスタンスバッファ（フレーム毎）
    Microsoft::WRL::ComPtr<ID3D12Resource> particleInstanceBuffer_[kFrameCount];
    ParticleInstanceData* mappedInstanceData_[kFrameCount] = {};
    int currentFrameIndex_ = 0;

    // テクスチャ配列関連
    D3D12_GPU_DESCRIPTOR_HANDLE textureArraySrvHandleGPU_{};
    std::vector<TextureManager::TextureResources> textures_;
    TextureManager::TextureResources textureArrayResource_; // Texture2DArray本体とSRVの管理用
    // 各テクスチャIDごとにParticleInstanceDataの配列を持つ
    std::unordered_map<uint32_t, std::vector<ParticleInstanceData>> particlesByTexture_;


    BlendMode currentBlendMode_ = kBlendModeNormal;

    uint32_t offscreenSrvIndex_;

	// ウィンドウタイトル
    static std::wstring windowTitle_;
};