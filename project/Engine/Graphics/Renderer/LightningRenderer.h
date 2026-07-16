#pragma once
#include "Mesh.h"
#include "RenderCommon.h"
#include "RenderEnvironment.h"

namespace FE
{

// 新しく1本の線を定義する構造体
struct LightningPath
{
    std::vector<Vector3> points;
    float thicknessScale = 1.0f; // 枝は細くするためのスケール
};

// 1本の雷のデータを管理する構造体
struct LightningInstance
{
    Vector3 startPos;       // 始点（雲）
    Vector3 endPos;         // 終点（地面）
    float lifeTime;         // 残り寿命（0になったら消滅）
    float maxLifeTime;      // 最大寿命
    uint32_t seed;          // 分岐や明滅パターンのシード値
    // 複数の経路を持つリスト
    std::vector<LightningPath> paths;
};

class LightningRenderer
{
public:
    LightningRenderer();
    void Initialize(const RenderEnvironment& env);
    void BeginFrame();
    void SpawnLightning(const Vector3& start, const Vector3& end, float duration);
    void Update();
    void Draw(const RenderEnvironment& env, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, const Vector3& cameraPos);

    void SetConfig(const LightningConfig& config) { config_ = config; }

private:
    // フラクタル形状（ジグザグ）を生成する再帰関数
    void GenerateFractalPath(
        LightningInstance& inst,
        std::vector<Vector3>& points,
        const Vector3& start,
        const Vector3& end,
        int depth,
        float displacement,
        float currentThickness);

    static constexpr int kFrameCount = 3;
    int currentFrameIndex_ = 0;

    // バッファの最大サイズ（安全のため多めに確保）
    static constexpr uint32_t kMaxVertices = 10000;
    static constexpr uint32_t kMaxIndices = 15000;

    std::vector<LightningInstance> activeLightnings_;

    // 動的頂点バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_[kFrameCount];
    LightningVertex* mappedVertices_[kFrameCount] = {};

    // 動的インデックスバッファ（追加）
    Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer_[kFrameCount];
    uint32_t* mappedIndices_[kFrameCount] = {};

    // WVPバッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_[kFrameCount];
    TransformationMatrix* mappedWvp_[kFrameCount] = {};

    // マテリアル定数バッファ（追加）
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_[kFrameCount];
    LightningMaterial* mappedMaterial_[kFrameCount] = {};

    LightningConfig config_;

    std::mt19937 randomEngine_;
};

}