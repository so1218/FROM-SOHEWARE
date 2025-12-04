#include "ParticleTextureHandle.h"
#include "Engine.h"
#include <cassert>

std::array<uint32_t, PARTICLE_TEXTURES_COUNT> ParticleTextureHandle::particleTextureHandles_{};
bool ParticleTextureHandle::initialized_ = false;
std::vector<std::string> ParticleTextureHandle::cachedTextureNames_;
std::vector<const char*> ParticleTextureHandle::cachedTextureItems_;

void ParticleTextureHandle::Initialize(Engine* engine)
{
    if (initialized_) return;

    for (const auto& def : particleTextureDefinitions_)
    {
        particleTextureHandles_[def.id] = engine->LoadTexture(def.path);
    }

    initialized_ = true;
}

uint32_t ParticleTextureHandle::Get(ParticleTextureID id)
{
    assert(initialized_ && "ParticleTextureHandle::Initialize must be called before Get()");
    assert(id >= 0 && id < PARTICLE_TEXTURES_COUNT);
    return particleTextureHandles_[id];
}

const std::vector<std::string>& ParticleTextureHandle::GetTextureNames()
{
    // 初回のみリストを作成する
    if (cachedTextureNames_.empty())
    {
        for (const auto& def : particleTextureDefinitions_)
        {
            std::string path = def.path;
            // ファイル名のみ抽出
            size_t lastSlash = path.find_last_of("/\\");
            if (lastSlash != std::string::npos) {
                cachedTextureNames_.push_back(path.substr(lastSlash + 1));
            }
            else {
                cachedTextureNames_.push_back(path);
            }
        }
    }
    return cachedTextureNames_;
}

const std::vector<const char*>& ParticleTextureHandle::GetTextureItems()
{
    // 名前リストがなければ作る
    if (cachedTextureNames_.empty()) {
        GetTextureNames();
    }

    // ImGui用のポインタ配列がなければ作る
    if (cachedTextureItems_.empty())
    {
        cachedTextureItems_.reserve(cachedTextureNames_.size());
        for (const auto& name : cachedTextureNames_) {
            cachedTextureItems_.push_back(name.c_str());
        }
    }
    return cachedTextureItems_;
}