#pragma once
#include "Mesh.h"
#include "RenderCommon.h"
#include "RenderEnvironment.h"

namespace FE
{

class LineRenderer 
{
public:
    void Initialize(const RenderEnvironment& env);
    void BeginFrame();

    // ラインの登録
    void Submit(const Vector3& start, const Vector3& end, uint32_t color);

    // 描画実行
    void Draw(const RenderEnvironment& env, const Matrix4x4& viewProjection);

    uint32_t GetCount() const { return prevCount_; }
    uint32_t GetMaxCount() const { return kMaxCount; }

private:
    static const int32_t kMaxCount = 4096;
    static const int32_t kMaxVertices = kMaxCount * 2;

    struct LineVertex
    {
        Vector4 position;
        Vector4 color;
    };

    struct LineBatchResource
    {
        Mesh mesh;
        std::vector<LineVertex> verticesCPU;
        Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource;
        TransformationMatrix* mappedWvp = nullptr;
    } lineBatch_;

    uint32_t prevCount_ = 0;
};

}