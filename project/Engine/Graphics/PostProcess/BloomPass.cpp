#include "pch.h"
#include "BloomPass.h"
#include "BufferManager.h"
#include "Engine.h"
#include "PIXColors.h"

namespace FE
{

void BloomPass::Initialize(Engine* engine, uint32_t w, uint32_t h, PSOManager* pso)
{
    // 出力用テクスチャの初期化（IPostEffect共通処理）
    InitializeBase(engine, w, h);
    engine_ = engine;
    psoManager_ = pso;

    settings_.radius = 1.0f;
    mipChain_.resize(kMaxMipLevels);

    uint32_t curW = Math::MyMax(1u, w / 2); // 半分のサイズからスタート
    uint32_t curH = Math::MyMax(1u, h / 2);

    auto* offscreenRTV = engine_->GetOffscreenRTVManager();
    ID3D12Device* device = engine_->GetGraphicsDevice()->GetDevice();

    for (uint32_t i = 0; i < kMaxMipLevels; ++i)
    {
        auto& mip = mipChain_[i];
        mip.width = curW;
        mip.height = curH;

        // ビューポートとシザー矩形の設定
        mip.viewport = { 0.0f, 0.0f, static_cast<float>(curW), static_cast<float>(curH), 0.0f, 1.0f };
        mip.scissorRect = { 0, 0, static_cast<LONG>(curW), static_cast<LONG>(curH) };

        // オフスクリーンレンダーターゲットの生成
        auto [resource, rtvHandle, srvIndex, uavIndex] = offscreenRTV->CreateOffscreenRenderTarget(
            curW, curH, Vector4(0, 0, 0, 0), DXGI_FORMAT_R16G16B16A16_FLOAT
        );

        mip.resource = resource;
        mip.rtvHandle = rtvHandle;
        mip.srvIndex = srvIndex;

        // 定数バッファの作成
        mip.constantBuffer = BufferManager::CreateMappedConstantBuffer<BloomSettings>(device, &mip.cbData);
        mip.cbData->texelSize = { 1.0f / curW, 1.0f / curH };
        mip.cbData->radius = 1.0f;

        // 次の階層へ縮小
        curW = Math::MyMax(1u, curW / 2);
        curH = Math::MyMax(1u, curH / 2);
    }
}

void BloomPass::Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context,
    D3D12_GPU_DESCRIPTOR_HANDLE overrideInput)
{
    PIXScopedEvent(cmdList, PIXColors::PostProcess, "Bloom Dual Filter Pass");

    D3D12_GPU_DESCRIPTOR_HANDLE inputSRV = (overrideInput.ptr != 0)
        ? overrideInput
        : context.GetGPUHandle(context.sceneColorSrvIndex);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // =========================================================
    // Downsample パス（多段階でぼかしながら縮小）
    // =========================================================
    cmdList->SetPipelineState(psoManager_->GetPSO("Downsample"));

    D3D12_GPU_DESCRIPTOR_HANDLE currentInputSRV = inputSRV;

    for (size_t i = 0; i < mipChain_.size(); ++i)
    {
        auto& mip = mipChain_[i];

        // 描画先を mip[i] の RT に変更 (ResourceBarrier: SRV -> RT)
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            mip.resource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET
        );
        cmdList->ResourceBarrier(1, &barrier);

        cmdList->RSSetViewports(1, &mip.viewport);
        cmdList->RSSetScissorRects(1, &mip.scissorRect);
        cmdList->OMSetRenderTargets(1, &mip.rtvHandle, FALSE, nullptr);

        // リソース設定
        cmdList->SetGraphicsRootDescriptorTable(2, currentInputSRV);
        cmdList->SetGraphicsRootConstantBufferView(0, mip.constantBuffer->GetGPUVirtualAddress());

        cmdList->DrawInstanced(3, 1, 0, 0);

        // 状態を戻す (ResourceBarrier: RT -> SRV)
        barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            mip.resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        );
        cmdList->ResourceBarrier(1, &barrier);

        // 次の Mip の入力として使用
        currentInputSRV = context.GetGPUHandle(mip.srvIndex);
    }

    // =========================================================
    // Upsample パス（拡大しながら加算合成）
    // =========================================================
    cmdList->SetPipelineState(psoManager_->GetPSO("Upsample"));

    for (int i = static_cast<int>(mipChain_.size()) - 2; i >= 0; --i)
    {
        auto& inputMip = mipChain_[i + 1];
        auto& targetMip = mipChain_[i];

        targetMip.cbData->radius = settings_.radius;

        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            targetMip.resource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET
        );
        cmdList->ResourceBarrier(1, &barrier);

        cmdList->RSSetViewports(1, &targetMip.viewport);
        cmdList->RSSetScissorRects(1, &targetMip.scissorRect);
        cmdList->OMSetRenderTargets(1, &targetMip.rtvHandle, FALSE, nullptr);

        cmdList->SetGraphicsRootDescriptorTable(2, context.GetGPUHandle(inputMip.srvIndex));
        cmdList->SetGraphicsRootConstantBufferView(0, targetMip.constantBuffer->GetGPUVirtualAddress());

        cmdList->DrawInstanced(3, 1, 0, 0);

        barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            targetMip.resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        );
        cmdList->ResourceBarrier(1, &barrier);
    }

    // =========================================================
    // BloomPass 自身の出力テクスチャへコピー
    // =========================================================
    PreDraw(cmdList);
    cmdList->SetPipelineState(psoManager_->GetPSO("PostProcessCopy")); // 単純描画/コピー用PSO
    cmdList->SetGraphicsRootDescriptorTable(2, context.GetGPUHandle(mipChain_[0].srvIndex));
    cmdList->DrawInstanced(3, 1, 0, 0);
    PostDraw(cmdList);
}

}