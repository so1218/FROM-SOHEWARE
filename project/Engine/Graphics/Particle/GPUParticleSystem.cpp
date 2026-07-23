#include "pch.h"
#include "GPUParticleSystem.h"

void GPUParticleSystem::Initialize(ID3D12Device* device)
{
    // A. StructuredBuffer (パーティクル本体) のリソース作成
    uint64_t bufferSize = sizeof(Particle) * kMaxParticles;

    D3D12_HEAP_PROPERTIES defaultHeapProps = {};
    defaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = bufferSize;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    // Compute Shaderでの書き込み(UAV)を許可
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    device->CreateCommittedResource(
        &defaultHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&particleBuffer_)
    );

    // B. EmitterData (定数バッファ) のリソース作成
    D3D12_HEAP_PROPERTIES uploadHeapProps = {};
    uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC cbDesc = {};
    cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    cbDesc.Width = (sizeof(EmitterData) + 255) & ~255; // 256バイトアライメント
    cbDesc.Height = 1;
    cbDesc.DepthOrArraySize = 1;
    cbDesc.MipLevels = 1;
    cbDesc.Format = DXGI_FORMAT_UNKNOWN;
    cbDesc.SampleDesc.Count = 1;
    cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &cbDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&emitterBuffer_)
    );

    // ※必要に応じてここで CreateUnorderedAccessView (UAV) や 
    // CreateShaderResourceView (SRV) をディスクリプタヒープに作成します
}

// ---------------------------------------------------------
// 2. Update (Compute Shader実行)
// ---------------------------------------------------------
void GPUParticleSystem::Update(ID3D12GraphicsCommandList* commandList, float deltaTime, DirectX::XMFLOAT3 emitterPos)
{
    totalTime_ += deltaTime;

    // A. 定数バッファの更新
    EmitterData cbData = {};
    cbData.emitterPos = emitterPos;
    cbData.deltaTime = deltaTime;
    cbData.time = totalTime_;

    void* mappedPtr = nullptr;
    emitterBuffer_->Map(0, nullptr, &mappedPtr);
    memcpy(mappedPtr, &cbData, sizeof(EmitterData));
    emitterBuffer_->Unmap(0, nullptr);

    // B. リソースバリア: 描画参照(SRV)から書き込み(UAV)へ遷移
    D3D12_RESOURCE_BARRIER barrierToUAV = {};
    barrierToUAV.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierToUAV.Transition.pResource = particleBuffer_.Get();
    barrierToUAV.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrierToUAV.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrierToUAV.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrierToUAV);

    // C. Compute ShaderのセットとDispatch
    commandList->SetComputeRootSignature(computeRootSignature_);
    commandList->SetPipelineState(computePSO_);

    // 定数バッファとUAVのバインド (エンジンのルートパラメーター設定に依存)
    commandList->SetComputeRootConstantBufferView(0, emitterBuffer_->GetGPUVirtualAddress());
    // commandList->SetComputeRootDescriptorTable(...) などでUAVをバインド

    // スレッドグループのディスパッチ (10000個 / 64スレッド = 157グループ)
    UINT groupCountX = (kMaxParticles + 63) / 64;
    commandList->Dispatch(groupCountX, 1, 1);

    // D. リソースバリア: 書き込み(UAV)から描画参照(SRV)へ戻す
    D3D12_RESOURCE_BARRIER barrierToSRV = {};
    barrierToSRV.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierToSRV.Transition.pResource = particleBuffer_.Get();
    barrierToSRV.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrierToSRV.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrierToSRV.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrierToSRV);
}

// ---------------------------------------------------------
// 3. Draw (Graphics Shader実行)
// ---------------------------------------------------------
void GPUParticleSystem::Draw(ID3D12GraphicsCommandList* commandList)
{
    // A. パイプライン・ルートシグネチャのセット
    commandList->SetGraphicsRootSignature(graphicsRootSignature_);
    commandList->SetPipelineState(graphicsPSO_);

    // B. SRVバッファ（更新されたパーティクルデータ）を頂点シェーダー用にバインド
    // commandList->SetGraphicsRootDescriptorTable(...) または SetGraphicsRootShaderResourceView(...)

    // C. プリミティブトポロジの設定 (1パーティクルあたり板ポリ = 6頂点)
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // D. GPU Instancing 描画 (6頂点 x 10000インスタンス)
    commandList->DrawInstanced(6, kMaxParticles, 0, 0);
}