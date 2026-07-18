#include "pch.h"
#include "LightningRenderer.h"
#include "BufferManager.h"
#include "PSOManager.h"
#include "RootSignatureManager.h"
#include "LightManager.h"
#include "SRVManager.h"
#include "GlobalConstants.h"
#include "CommandManager.h"
#include "GraphicsDevice.h"
#include "TimeManager.h"

namespace FE
{

LightningRenderer::LightningRenderer()
{
    std::random_device seedGen;
    randomEngine_.seed(seedGen());
}

void LightningRenderer::Initialize(const RenderEnvironment& env)
{
    auto device = env.device->GetDevice();

    for (int i = 0; i < kFrameCount; ++i)
    {
        vertexBuffer_[i] = BufferManager::CreateMappedBuffer(device, kMaxVertices, &mappedVertices_[i]);
        indexBuffer_[i] = BufferManager::CreateMappedBuffer(device, kMaxIndices, &mappedIndices_[i]);
        materialResource_[i] = BufferManager::CreateMappedConstantBuffer(device, &mappedMaterial_[i]);

        // マテリアルの初期値セット
        mappedMaterial_[i]->coreColor = { 1.0f, 1.0f, 1.0f };       // 芯は純白
        mappedMaterial_[i]->coreThickness = 0.15f;                  // 芯の細さ
        mappedMaterial_[i]->fringeColor = { 0.1f, 0.5f, 1.0f };     // 青紫の滲み
        mappedMaterial_[i]->corePower = 4.0f;                       // 芯の飛び具合
        mappedMaterial_[i]->glowPower = 2.5f;                       // 滲みの広がり
        mappedMaterial_[i]->flickerSpeed = 60.0f;                   // 激しい明滅
        mappedMaterial_[i]->flickerMin = 0.4f;
        mappedMaterial_[i]->flickerMax = 1.2f;
        mappedMaterial_[i]->emissiveIntensity = 50.0f;              // ブルームを反応させる異常値
        mappedMaterial_[i]->instanceSeed = 0.0f;                    // 描画時に上書きされる
    }
}

void LightningRenderer::BeginFrame()
{
    currentFrameIndex_ = (currentFrameIndex_ + 1) % kFrameCount;
}

// 中点変位法（フラクタル）による再帰的な経路生成
void LightningRenderer::GenerateFractalPath(
    LightningInstance& inst,
    std::vector<Vector3>& points,
    const Vector3& start,
    const Vector3& end,
    int depth,
    float displacement,
    float currentThickness)
{
    if (depth == 0)
    {
        points.push_back(end);
        return;
    }

    Vector3 mid = (start + end) * 0.5f;

    std::uniform_real_distribution<float> dist(-0.5f, 0.5f);

    float randX = dist(randomEngine_) * displacement;
    float randY = dist(randomEngine_) * displacement * 0.2f; // Yは逆行を防ぐため控えめに
    float randZ = dist(randomEngine_) * displacement;

    mid = mid + Vector3(randX, randY, randZ);

    // 前半のパスを生成
    GenerateFractalPath(inst, points, start, mid, depth - 1, displacement * 0.5f, currentThickness);

    // 枝分かれの生成判定
    std::uniform_real_distribution<float> probDist(0.0f, 1.0f);
    if (probDist(randomEngine_) < config_.branchProbability)
    {
        // 進行方向ベクトル
        Vector3 mainDir = end - start;

        // 枝の終点（メイン方向からランダムに逸れた位置）
        Vector3 branchOffset(dist(randomEngine_), dist(randomEngine_) * 0.5f, dist(randomEngine_));
        Vector3 branchEnd = mid + (mainDir * config_.branchLengthScale) + (branchOffset * displacement * 2.0f);

        // 新しいパス（枝）の作成
        LightningPath branchPath;
        branchPath.thicknessScale = currentThickness * 0.5f; // 枝は半分くらいの細さにする
        branchPath.points.push_back(mid); // 枝の始点は現在のmid

        // 枝に対して再帰処理（計算負荷を下げるため深さを-1しておく）
        GenerateFractalPath(inst, branchPath.points, mid, branchEnd, depth - 1, displacement * 0.5f, branchPath.thicknessScale);

        // インスタンスに枝を追加
        inst.paths.push_back(branchPath);
    }

    // 3. 後半のパスを生成
    GenerateFractalPath(inst, points, mid, end, depth - 1, displacement * 0.5f, currentThickness);
}

void LightningRenderer::SpawnLightning(const Vector3& start, const Vector3& end, float duration)
{
    LightningInstance inst;
    inst.startPos = start;
    inst.endPos = end;
    inst.maxLifeTime = duration;
    inst.lifeTime = duration;

    std::uniform_real_distribution<float> seedDist(0.0f, 10000.0f);
    inst.seed = seedDist(randomEngine_);

    // メインのパス（幹）を作成
    LightningPath mainPath;
    mainPath.thicknessScale = 1.0f;
    mainPath.points.push_back(start);

    // 再帰呼び出し（メインパスの points を渡す）
    GenerateFractalPath(inst, mainPath.points, start, end, config_.fractalDepth, config_.displacement, 1.0f);

    // インスタンスにメインパスを追加
    inst.paths.push_back(mainPath);

    activeLightnings_.push_back(inst);
}

void LightningRenderer::Update()
{
    // 寿命を減らし、0になったものを削除
    for (auto it = activeLightnings_.begin(); it != activeLightnings_.end();)
    {
        it->lifeTime -= TimeManager::GetInstance()->GetDeltaTime();
        if (it->lifeTime <= 0.0f) {
            it = activeLightnings_.erase(it);
        }
        else {
            ++it;
        }
    }
}

// カメラに向けた板ポリゴン生成と描画
void LightningRenderer::Draw(const RenderEnvironment& env, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, const Vector3& cameraPos)
{
    if (activeLightnings_.empty()) return;

    // 定数バッファに必要な設定を転送
    mappedMaterial_[currentFrameIndex_]->coreColor = config_.coreColor;
    mappedMaterial_[currentFrameIndex_]->coreThickness = config_.coreThickness;
    mappedMaterial_[currentFrameIndex_]->fringeColor = config_.fringeColor;
    mappedMaterial_[currentFrameIndex_]->corePower = config_.corePower;
    mappedMaterial_[currentFrameIndex_]->glowPower = config_.glowPower;
    mappedMaterial_[currentFrameIndex_]->emissiveIntensity = config_.emissiveIntensity;
    mappedMaterial_[currentFrameIndex_]->flickerSpeed = config_.flickerSpeed;

    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    LightningVertex* vMap = mappedVertices_[currentFrameIndex_];
    uint32_t* iMap = mappedIndices_[currentFrameIndex_];

    for (const auto& inst : activeLightnings_)
    {
        // 残り寿命からフェードアウトのアルファ値を計算
        float alpha = inst.lifeTime / inst.maxLifeTime;

        // マテリアル定数にシード値や明滅係数を流し込む
        mappedMaterial_[currentFrameIndex_]->instanceSeed = static_cast<float>(inst.seed);
        mappedMaterial_[currentFrameIndex_]->flickerMin = config_.flickerMin;
        mappedMaterial_[currentFrameIndex_]->flickerMax = config_.flickerMax;

        // インスタンスが持つすべてのパス（本流＋枝分かれしたパス）をすべてループ
        for (const auto& path : inst.paths)
        {
            const auto& pts = path.points;
            if (pts.size() < 2) continue; // 2点未満は板ポリゴンを形成できないためスキップ

            // 枝分かれの太さスケールを元の太さに掛け合わせる
            float thickness = config_.thickness * path.thicknessScale;
            uint32_t baseVertex = vertexCount;

            // 頂点生成フラグ
            bool vertexOverflow = false;

            for (uint32_t i = 0; i < static_cast<uint32_t>(pts.size()); ++i)
            {
                // 動的バッファの上限を超えないよう安全柵
                if (vertexCount + 2 > kMaxVertices)
                {
                    vertexOverflow = true;
                    break;
                }

                Vector3 curr = pts[i];

                // 進行方向ベクトル
                Vector3 dir;
                if (i < pts.size() - 1) dir = (pts[i + 1] - curr).Normalize();
                else dir = (curr - pts[i - 1]).Normalize();

                // カメラへのベクトルと外積を取って横幅ベクトルを算出
                Vector3 toCamera = (cameraPos - curr).Normalize();
                Vector3 right = (dir.Cross(toCamera)).Normalize();

                float v = static_cast<float>(i) / (static_cast<float>(pts.size()) - 1.0f);

                // 左頂点
                Vector3 leftPos = curr - right * (thickness * 0.5f);
                vMap[vertexCount].position = { leftPos.x, leftPos.y, leftPos.z, 1.0f };
                vMap[vertexCount].texcoord = { 0.0f, v };
                vMap[vertexCount].color = { 1.0f, 1.0f, 1.0f, alpha };
                vertexCount++;

                // 右頂点
                Vector3 rightPos = curr + right * (thickness * 0.5f);
                vMap[vertexCount].position = { rightPos.x, rightPos.y, rightPos.z, 1.0f };
                vMap[vertexCount].texcoord = { 1.0f, v };
                vMap[vertexCount].color = { 1.0f, 1.0f, 1.0f, alpha };
                vertexCount++;
            }

            // このパス内で実際に生成できた頂点ペア（セグメント）数を算出
            uint32_t generatedSegments = (vertexCount - baseVertex) / 2;
            if (generatedSegments < 2) continue; // セグメントが不十分なら描画スキップ

            // インデックスの構築（パスごとに独立した四角形ポリゴンをつなぐ）
            for (uint32_t i = 0; i < generatedSegments - 1; ++i)
            {
                if (indexCount + 6 > kMaxIndices) break;

                uint32_t localBase = baseVertex + (i * 2);
                iMap[indexCount++] = localBase;
                iMap[indexCount++] = localBase + 1;
                iMap[indexCount++] = localBase + 2;

                iMap[indexCount++] = localBase + 1;
                iMap[indexCount++] = localBase + 3;
                iMap[indexCount++] = localBase + 2;
            }

            // 頂点が限界に達していたらすべてのループから完全に抜ける
            if (vertexOverflow) break;
        }
    }

    if (vertexCount == 0 || indexCount == 0) return;

    // コマンドリストへの記録
    auto commandList = env.commandManager->GetCommandList();

    // RootSignatureとPSOをセット
    commandList->SetPipelineState(env.psoManager->GetPSO("Lightning"));
    commandList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Lightning"));

    // 頂点/インデックスバッファビューの設定
    D3D12_VERTEX_BUFFER_VIEW vbv{};
    vbv.BufferLocation = vertexBuffer_[currentFrameIndex_]->GetGPUVirtualAddress();
    vbv.SizeInBytes = vertexCount * sizeof(LightningVertex);
    vbv.StrideInBytes = sizeof(LightningVertex);
    commandList->IASetVertexBuffers(0, 1, &vbv);

    D3D12_INDEX_BUFFER_VIEW ibv{};
    ibv.BufferLocation = indexBuffer_[currentFrameIndex_]->GetGPUVirtualAddress();
    ibv.SizeInBytes = indexCount * sizeof(uint32_t);
    ibv.Format = DXGI_FORMAT_R32_UINT;
    commandList->IASetIndexBuffer(&ibv);

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 定数バッファのセット
    commandList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, materialResource_[currentFrameIndex_]->GetGPUVirtualAddress());

    // 描画
    commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
}

}