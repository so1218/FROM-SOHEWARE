#include "ParticleTextureHandle.h"
#include <cassert>

std::array<uint32_t, PARTICLE_TEXTURES_COUNT> ParticleTextureHandle::particleTextureHandles_{};
bool ParticleTextureHandle::initialized_ = false;

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