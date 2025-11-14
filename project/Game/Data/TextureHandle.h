#pragma once
#include "Engine.h"

// テクスチャID
enum TextureID
{
    white1x1,
    uvChecker,
    monsterBall,

    // dds
    skyboxCubemapBlack,
    skyboxCubemap,

    // PlayScene     
    skydome,
    axe,
    knife,

    enemy,

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
            { monsterBall,   "Resources/images/Shrimp_TestTexture.png" },

            // dds
            { skyboxCubemapBlack,   "Resources/images/black_cube.dds" },
            { skyboxCubemap,   "Resources/images/rostock_laage_airport_4k.dds" },

            { skydome,   "Resources/images/sky_sphere.png" },
            { axe,   "Resources/images/Woodcutter-Axe.jpg" },
            { knife,   "Resources/images/KnifeTexture..jpg" },

            { enemy,   "Resources/images/Blaze_baseColor.png" },
        }
    };
};