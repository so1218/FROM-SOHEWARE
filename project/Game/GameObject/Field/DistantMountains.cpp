#include "pch.h"
#include "DistantMountains.h"
#include "ImGuiManager.h"

using namespace FS;

namespace
{

// 歩ける地形の下に沈める深さ。粗い格子が細かい地形を突き抜けないようにする
constexpr float kSinkDepth = 2.0f;

float Hash(int x, int z, int seed)
{
    uint32_t value = static_cast<uint32_t>(x) * 374761393u + static_cast<uint32_t>(z) * 668265263u + static_cast<uint32_t>(seed) * 2147483647u;
    value = (value ^ (value >> 13)) * 1274126177u;
    value ^= value >> 16;
    return static_cast<float>(value & 0xFFFFFFu) / static_cast<float>(0xFFFFFF);
}

float SmoothStep(float edge0, float edge1, float value)
{
    const float t = std::clamp((value - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// 0 ~ 1 のなめらかなノイズ。格子の角の乱数を 5 次の曲線で補間する
float ValueNoise(float x, float z, int seed)
{
    const float cellX = std::floor(x);
    const float cellZ = std::floor(z);
    const int ix = static_cast<int>(cellX);
    const int iz = static_cast<int>(cellZ);
    const float fx = x - cellX;
    const float fz = z - cellZ;
    const float ux = fx * fx * fx * (fx * (fx * 6.0f - 15.0f) + 10.0f);
    const float uz = fz * fz * fz * (fz * (fz * 6.0f - 15.0f) + 10.0f);

    const float value00 = Hash(ix, iz, seed);
    const float value10 = Hash(ix + 1, iz, seed);
    const float value01 = Hash(ix, iz + 1, seed);
    const float value11 = Hash(ix + 1, iz + 1, seed);
    const float bottom = value00 + (value10 - value00) * ux;
    const float top = value01 + (value11 - value01) * ux;
    return bottom + (top - bottom) * uz;
}

// 折り返したノイズを重ねて、尾根のある山の形にする。0 ~ 1
float RidgedNoise(float x, float z, int seed, float sharpness)
{
    constexpr int kOctaveCount = 5;
    float sum = 0.0f;
    float amplitude = 1.0f;
    float amplitudeSum = 0.0f;
    float frequency = 1.0f;
    for (int octave = 0; octave < kOctaveCount; ++octave)
    {
        const float ridge = 1.0f - std::abs(ValueNoise(x * frequency, z * frequency, seed + octave * 101) * 2.0f - 1.0f);
        sum += std::pow(ridge, sharpness) * amplitude;
        amplitudeSum += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    return sum / amplitudeSum;
}

}

DistantMountains::DistantMountains(Engine* engine, const Terrain* playableTerrain, const std::string& groupName)
    : GameObject(), engine_(engine), playableTerrain_(playableTerrain)
{
    vistaTerrain_ = std::make_unique<Terrain>(engine_);
    vistaTerrain_->SetDrawAsVista(true);
    vistaTerrain_->ShareMaterialFrom(playableTerrain_);
    binder_ = std::make_unique<PropertyBinder>(engine_, groupName);
}

void DistantMountains::Initialize()
{
    binder_->Bind("Enable", &isEnabled_, true);
    binder_->Bind("Extent", &extent_, 1000.0f, 10.0f, 100.0f, 5000.0f);
    binder_->Bind("CellSize", &cellSize_, 4.0f, 0.5f, 1.0f, 32.0f);
    binder_->Bind("FlatWidth", &flatWidth_, 40.0f, 1.0f, 0.0f, 1000.0f);
    binder_->Bind("RiseLength", &riseLength_, 300.0f, 5.0f, 10.0f, 3000.0f);
    binder_->Bind("MountainHeight", &mountainHeight_, 150.0f, 1.0f, 0.0f, 2000.0f);
    binder_->Bind("NoiseScale", &noiseScale_, 0.004f, 0.0001f, 0.0001f, 0.1f);
    binder_->Bind("RidgeSharpness", &ridgeSharpness_, 2.0f, 0.05f, 0.5f, 6.0f);
    binder_->Bind("Seed", &seed_, 1, 1.0f, 0, 9999);
}

void DistantMountains::Update()
{
    // 設定か歩ける地形が変わったときだけ作り直す
    if (needsRegenerate_ || playableTerrain_->GetShapeVersion() != generatedTerrainVersion_)
    {
        Generate();
    }
}

void DistantMountains::Draw()
{
    if (isEnabled_)
    {
        vistaTerrain_->Draw();
    }
}

void DistantMountains::Generate()
{
    needsRegenerate_ = false;
    generatedTerrainVersion_ = playableTerrain_->GetShapeVersion();

    const Vector2 playableSize = playableTerrain_->GetSizeXZ();
    if (playableSize.x <= 0.0f || playableSize.y <= 0.0f)
    {
        return;
    }

    const Vector2 playableCenter = playableTerrain_->GetCenterXZ();
    const Vector2 playableMin = playableCenter - playableSize * 0.5f;
    const Vector2 playableMax = playableCenter + playableSize * 0.5f;

    // 頂点を歩ける地形の端の頂点にそろえ、つなぎ目の段差を小さくする
    const float cellSize = std::max(cellSize_, 1.0f);
    const int outerCellCount = static_cast<int>(std::ceil(extent_ / cellSize));
    const float originX = playableMin.x - outerCellCount * cellSize;
    const float originZ = playableMin.y - outerCellCount * cellSize;
    const uint32_t width = static_cast<uint32_t>(std::ceil(playableSize.x / cellSize)) + outerCellCount * 2 + 1;
    const uint32_t height = static_cast<uint32_t>(std::ceil(playableSize.y / cellSize)) + outerCellCount * 2 + 1;

    std::vector<float> heights(static_cast<size_t>(width) * height);
    float minHeight = FLT_MAX;
    float maxHeight = -FLT_MAX;
    for (uint32_t z = 0; z < height; ++z)
    {
        for (uint32_t x = 0; x < width; ++x)
        {
            const float value = CalculateHeight(originX + x * cellSize, originZ + z * cellSize, playableMin, playableMax);
            heights[static_cast<size_t>(z) * width + x] = value;
            minHeight = std::min(minHeight, value);
            maxHeight = std::max(maxHeight, value);
        }
    }

    // 高さを 0 ~ 1 に直す。地形の位置の Y を真ん中の高さにする
    const float heightRange = std::max(maxHeight - minHeight, 1.0f);
    const float middleHeight = (minHeight + maxHeight) * 0.5f;
    for (float& value : heights)
    {
        value = (value - middleHeight) / heightRange + 0.5f;
    }

    const Vector2 vistaSize = { (width - 1) * cellSize, (height - 1) * cellSize };
    vistaTerrain_->GetPosition() = { originX + vistaSize.x * 0.5f, middleHeight, originZ + vistaSize.y * 0.5f };
    vistaTerrain_->LoadFromHeightRatios(heights, width, height, cellSize, heightRange);

    // 歩ける地形の下にあるチャンクは描かない。端は粗い格子がはみ出すので少し内側にする
    const Vector2 margin = { cellSize * 2.0f, cellSize * 2.0f };
    vistaTerrain_->SetHiddenArea(playableMin + margin, playableMax - margin);

    isGenerated_ = true;
    ++shapeVersion_;
}

float DistantMountains::GetHeight(float worldX, float worldZ) const
{
    float height = 0.0f;
    if (isGenerated_ && vistaTerrain_->GetHeightAt(worldX, worldZ, height))
    {
        return height;
    }

    // まだ作っていないときや遠景の外は、作るときと同じ式で求める
    const Vector2 playableSize = playableTerrain_->GetSizeXZ();
    const Vector2 playableCenter = playableTerrain_->GetCenterXZ();
    return CalculateHeight(worldX, worldZ, playableCenter - playableSize * 0.5f, playableCenter + playableSize * 0.5f);
}

float DistantMountains::CalculateHeight(float worldX, float worldZ, const Vector2& playableMin, const Vector2& playableMax) const
{
    // 歩ける地形の中で一番近い点。端ちょうどは高さが取れないので少し内側にする
    constexpr float kEdgeInset = 0.01f;
    const float nearestX = std::clamp(worldX, playableMin.x, playableMax.x - kEdgeInset);
    const float nearestZ = std::clamp(worldZ, playableMin.y, playableMax.y - kEdgeInset);
    const float edgeHeight = playableTerrain_->GetHeight(nearestX, nearestZ);

    const float outsideX = worldX - nearestX;
    const float outsideZ = worldZ - nearestZ;
    const float outsideDistance = std::sqrt(outsideX * outsideX + outsideZ * outsideZ);

    // 歩ける地形の中は、その下に沈めて隠す。端では沈めずにつなげる
    if (outsideDistance <= 0.0f)
    {
        const float insideDistance = std::min({ worldX - playableMin.x, playableMax.x - worldX, worldZ - playableMin.y, playableMax.y - worldZ });
        return edgeHeight - kSinkDepth * SmoothStep(0.0f, cellSize_ * 2.0f, insideDistance);
    }

    // 山の立ち上がりが四角い輪にならないよう、始まる距離をゆっくり変える
    const float sampleX = worldX * noiseScale_;
    const float sampleZ = worldZ * noiseScale_;
    const float riseOffset = ValueNoise(sampleX * 0.5f + 37.0f, sampleZ * 0.5f + 11.0f, seed_ + 7) * 2.0f - 1.0f;
    const float riseStart = std::max(flatWidth_ * (1.0f + riseOffset * 0.5f), 0.0f);
    const float rise = SmoothStep(riseStart, riseStart + riseLength_, outsideDistance);

    const float shape = 0.25f + 0.75f * RidgedNoise(sampleX, sampleZ, seed_, ridgeSharpness_);
    return edgeHeight + rise * mountainHeight_ * shape;
}

void DistantMountains::DebugGui()
{
    ImGui::Begin("遠景の山");

    binder_->Draw("Enable", "表示する");
    needsRegenerate_ |= binder_->Draw("Extent", "地形の端から外へ広げる距離");
    needsRegenerate_ |= binder_->Draw("CellSize", "頂点の間隔 (大きいほど軽く粗い)");
    needsRegenerate_ |= binder_->Draw("FlatWidth", "端から平らなまま続く幅");
    needsRegenerate_ |= binder_->Draw("RiseLength", "山の高さになるまでの距離");
    needsRegenerate_ |= binder_->Draw("MountainHeight", "山の高さ");
    needsRegenerate_ |= binder_->Draw("NoiseScale", "山の細かさ (小さいほど大きな山)");
    needsRegenerate_ |= binder_->Draw("RidgeSharpness", "尾根の鋭さ");
    needsRegenerate_ |= binder_->Draw("Seed", "形の種");

    ImGui::End();
}
