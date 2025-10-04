#include "TextureHandle.h"

#include <cassert>

std::array<uint32_t, TEXTURES_COUNT> TextureHandle::textureHandles_{};
bool TextureHandle::initialized_ = false;

constexpr std::array<TextureDefinition, TEXTURES_COUNT> TextureHandle::textureDefinitions_;

void TextureHandle::Initialize(Engine* engine)
{
    if (initialized_) return;

    for (const auto& def : textureDefinitions_)
    {
        textureHandles_[def.id] = engine->LoadTexture(def.path);
    }

    initialized_ = true;
}

uint32_t TextureHandle::Get(TextureID id)
{
    assert(initialized_ && "TextureHandle::Initialize must be called before Get()");
    assert(id >= 0 && id < TEXTURES_COUNT);
    return textureHandles_[id];
}