#pragma once
#include "Mesh.h"
#include "RenderCommon.h"
#include "ShadowMap.h"
#include "RenderEnvironment.h"

namespace FE
{

class GrassRenderer 
{
public:
    void Initialize(const RenderEnvironment& env);
    void BeginFrame();

    // 描画リクエストの受付
    void Submit(const Vector3& position, float height, float rotationY, float width, uint32_t packedColor);

    // 描画実行
    void Draw(const RenderEnvironment& env, uint32_t windMapTextureHandle, ShadowMap* shadowMap, const GrassMaterialData& materialData, const GrassCullingData& cullingData);

    // 明示的にデータを消去する関数を追加
    void ClearInstances();

private:
    static const int32_t kMaxInstances = 1500000;
    static constexpr int kFrameCount = 3;

    Mesh mesh_;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> passHeap_;

    // [入力] CPUから全草を転送するバッファ (SRVとしてCSに渡す)
    Microsoft::WRL::ComPtr<ID3D12Resource> inputInstanceBuffer_[kFrameCount];
    GrassInstanceData* mappedInputData_[kFrameCount] = {};

    // [出力] CSが生き残った草を書き込むバッファ (UAVとしてCSへ、SRVとしてVSへ渡す)
    Microsoft::WRL::ComPtr<ID3D12Resource> outputInstanceBuffer_[kFrameCount];

    // [間接描画引数] CSがインスタンス数をカウントアップするバッファ (UAV)
    Microsoft::WRL::ComPtr<ID3D12Resource> indirectArgsBuffer_[kFrameCount];

    // [リセット用] 間接描画引数を初期化するためのアップロードバッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> indirectArgsUploadBuffer_;

    // マテリアル & カリング設定用バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_[kFrameCount];
    Microsoft::WRL::ComPtr<ID3D12Resource> cullingDataResource_[kFrameCount];
    GrassMaterialData* mappedMaterial_[kFrameCount] = {};
    GrassCullingData* mappedCullingData_[kFrameCount] = {};

    // SRV/UAVのヒープインデックス管理
    uint32_t inputSrvIndex_[kFrameCount];
    uint32_t outputUavIndex_[kFrameCount];
    uint32_t outputSrvIndex_[kFrameCount];
    uint32_t indirectUavIndex_[kFrameCount];

    // 間接描画コマンドシグネチャ
    Microsoft::WRL::ComPtr<ID3D12CommandSignature> commandSignature_;

    int currentFrameIndex_ = 0;
    std::vector<GrassInstanceData> instanceQueue_;

    // kFrameCount(通常2〜3)フレーム分、全バッファを更新するためのカウンター
    int dirtyFrames_ = 0;
};

}