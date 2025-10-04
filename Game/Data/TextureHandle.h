#pragma once
#include "Engine.h"

// テクスチャID
enum TextureID
{
    white1x1,
    uvChecker,
    monsterBall,

    // PlayScene     
    skydome,

    // particle
    particlePurple,

    // 全てのテクスチャIDの数
    TEXTURES_COUNT
};

struct TextureDefinition
{
    TextureID id;
    const char* path;
};

class TextureHandle
{
public:
    static void Initialize(Engine* engine);
    static uint32_t Get(TextureID id);

private:
    static std::array<uint32_t, TEXTURES_COUNT> textureHandles_;
    static bool initialized_;

    static constexpr std::array<TextureDefinition, TEXTURES_COUNT> textureDefinitions_ = 
    { 
        {
            { white1x1, "Resources/images/white1x1.png" },
            { uvChecker,"Resources/images/uvChecker.png" },
            { monsterBall,   "Resources/images/monsterBall.png" },
            { skydome,   "Resources/images/sky_sphere.png" },
            { particlePurple,   "Resources/images/particle1.png" },
        }
    };
};