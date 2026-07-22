#include "pch.h"
#include "GrassSystem.h"
#include "Engine.h"
#include "ModelManager.h"

namespace FE
{

GrassSystem::GrassSystem(Engine* engine, const std::string& windMapTextureName)
    : engine_(engine)
{
    // メッシュの初期化は不要。レンダラー側のバッファ初期化のみ呼ぶ
    engine_->GetRendererManager()->InitializeGrass();
    SetWindMapTexture(windMapTextureName);
}

void GrassSystem::AddGrass(const Vector3& position, float height, float rotationY, float width, const Vector4& color)
{
    Instance inst;
    inst.position = position;
    inst.height = height;
    inst.rotationY = rotationY;
    inst.width = width;
    inst.packedColor = PackColor(color);

    instances_.push_back(inst);
}

void GrassSystem::Clear()
{
    instances_.clear();
}

void GrassSystem::Draw()
{
    if (instances_.empty() || !engine_) return;

    auto* rendererManager = engine_->GetRendererManager();

    // マテリアルとテクスチャの設定をマネージャーに伝達
    rendererManager->SetGrassRenderingParams(windMapTextureHandle_, materialData_);

    for (const auto& inst : instances_)
    {
        rendererManager->SubmitGrass(inst.position, inst.height, inst.rotationY, inst.width, inst.packedColor);
    }
}

void GrassSystem::SetWindMapTexture(const std::string& textureName)
{
    windMapTextureHandle_ = TextureManager::GetInstance().Get(textureName);
}

uint32_t GrassSystem::PackColor(const Vector4& c)
{
    uint8_t r = static_cast<uint8_t>(std::clamp(c.x * 255.0f, 0.0f, 255.0f));
    uint8_t g = static_cast<uint8_t>(std::clamp(c.y * 255.0f, 0.0f, 255.0f));
    uint8_t b = static_cast<uint8_t>(std::clamp(c.z * 255.0f, 0.0f, 255.0f));
    uint8_t a = static_cast<uint8_t>(std::clamp(c.w * 255.0f, 0.0f, 255.0f));
    return (a << 24) | (b << 16) | (g << 8) | r;
}

}