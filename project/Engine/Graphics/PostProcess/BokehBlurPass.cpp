#include "BokehBlurPass.h"
#include "BufferManager.h"
#include "Engine.h" // BufferManager等のため

void BokehBlurPass::Initialize(Engine* engine, UINT width, UINT height, PSOManager* psoManager)
{
    // 基底クラスの初期化（RT作成など）
    // ★重要: ボケは少し解像度を落としたほうが綺麗で軽いので、
    // マネージャ側で width/2, height/2 を渡すことを推奨
    InitializeBase(engine, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT);

    engine_ = engine;
    psoManager_ = psoManager;

    // 定数バッファ作成
    ID3D12Device* device = engine->graphicsDevice_->GetDevice();
    constantBuffer_ = BufferManager::CreateBufferResource(device, sizeof(DoFSettingsData));
    constantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&cbData_));

    // 初期値
    cbData_->resolution = { (float)width, (float)height };
}

// コンパイルエラー回避用のダミー実装
void BokehBlurPass::Execute(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE inputSRV)
{
    // このパスでは深度も必要なため、この関数は使いません。
    // 間違って呼ばれたら止まるようにしても良いです
    assert(false && "Don't call this 2-arg Execute for BokehBlurPass.");
}

void BokehBlurPass::Execute(ID3D12GraphicsCommandList* cmdList,
    D3D12_GPU_DESCRIPTOR_HANDLE sceneSrvGPU,
    D3D12_GPU_DESCRIPTOR_HANDLE depthSrvGPU)
{
    // 1. RTを描画可能にセット（基底クラスの処理）
    PreDraw(cmdList);

    // 2. PSOとRootSignature設定
    cmdList->SetGraphicsRootSignature(
    engine_->rootSignatureManager_->GetRootSignature("BokehBlur"));
    // ※ "BokehBlur" 用のPSOとRootSignatureを用意しておく必要があります
    cmdList->SetPipelineState(psoManager_->GetPSO("BokehBlur"));
    // ルートシグネチャは共通の "PostProcess" を使う前提であればそのままでOKですが、
    // t0, t1 を受けるためにディスクリプタテーブルの設定に注意が必要です。

    // 3. 定数バッファ設定 (例: b0)
    cmdList->SetGraphicsRootConstantBufferView(0, constantBuffer_->GetGPUVirtualAddress());

    cmdList->SetGraphicsRootConstantBufferView(
        1,
        engine_->cameraManager_
        ->GetCameraResource()
        ->GetGPUVirtualAddress()
    );


    // 4. テクスチャ設定
    // ルートシグネチャの定義によりますが、一般的に以下の2パターンがあります

    // 【パターンA】 ルートパラメータで t0 と t1 が別のテーブルとして定義されている場合
    // cmdList->SetGraphicsRootDescriptorTable(1, sceneSrvGPU); // t0
    // cmdList->SetGraphicsRootDescriptorTable(2, depthSrvGPU); // t1

    // 【パターンB】 t0-t? が1つのレンジになっている場合 (一般的)
    // この場合、SRVManagerを使って「シーンSRV」と「深度SRV」が連続した
    // テンポラリなディスクリプタヒープ領域を作成し、その先頭ハンドルを渡す必要があります。
    // CombinePass::SetupInputViews と同じような仕組みが必要です。
    // ここでは簡略化のため、「CombinePassと同じ仕組みでセットアップ済み」のハンドルを受け取るか、
    // 個別にセットできるルートシグネチャ構成にすることを推奨します。

    // 仮実装: ルートパラメータ[1]が t0(Scene), [2]が t1(Depth) と定義されているとする
    cmdList->SetGraphicsRootDescriptorTable(2, sceneSrvGPU);
    cmdList->SetGraphicsRootDescriptorTable(3, depthSrvGPU);

    // 5. 描画
    cmdList->DrawInstanced(3, 1, 0, 0);

    // 6. RTをSRVに戻す（基底クラスの処理）
    PostDraw(cmdList);
}