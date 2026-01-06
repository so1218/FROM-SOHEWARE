#pragma once
#include "Engine.h"

// テクスチャID
enum TextureID
{
    white1x1,
    uvChecker,
    monsterBall,
    toonRamp,

    // dds
    skyboxCubemapBlack,
    skyboxCubemap,

    // PlayScene     
    skydome,
    axe,
    knife,

    enemy,

    //文字フォント
    num1,
    num2,
    num3,
    num4,
    num5,
    num6,
    num7,
    num8,
    num9,
    num0,
    
    coron,
    hpGage,

    // カードフォント
    cardTextKnife,
    cardTextAxe,

    title,
    clear,
    axeLevelUp,
    knifeLevelUp,
    hpUp,
    speedUp,
    heal,

    cameraSousa,
    moveSousa,
    useController,
    pressSousa,

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
            { toonRamp,   "Resources/images/toonRamp_3.png" },

            // dds
            { skyboxCubemapBlack,   "Resources/images/black_cube.dds" },
            { skyboxCubemap,   "Resources/images/rostock_laage_airport_4k.dds" },

            { skydome,   "Resources/images/sky_sphere.png" },
            { axe,   "Resources/images/Woodcutter-Axe.jpg" },
            { knife,   "Resources/images/KnifeTexture..jpg" },

            { enemy,   "Resources/images/Blaze_baseColor.png" },

            { num1, "Resources/images/numFont/1.png" },
            { num2, "Resources/images/numFont/2.png" },
            { num3, "Resources/images/numFont/3.png" },
            { num4, "Resources/images/numFont/4.png" },
            { num5, "Resources/images/numFont/5.png" },
            { num6, "Resources/images/numFont/6.png" },
            { num7, "Resources/images/numFont/7.png" },
            { num8, "Resources/images/numFont/8.png" },
            { num9, "Resources/images/numFont/9.png" },
            { num0, "Resources/images/numFont/0.png" },

            { coron, "Resources/images/numFont/coron.png" },
            { hpGage, "Resources/images/hpGage.png" },

            { cardTextKnife, "Resources/images/numFont/9.png" },
            { cardTextAxe, "Resources/images/numFont/0.png" },

            { title, "Resources/images/title.png" },
            { clear, "Resources/images/clear.png" },
            { axeLevelUp, "Resources/images/axeLevelUp.png" },
            { knifeLevelUp, "Resources/images/knifeLevelUp.png" },
            { hpUp, "Resources/images/hpUp.png" },
            { speedUp, "Resources/images/speedUp.png" },
            { heal, "Resources/images/heal.png" },

            { cameraSousa, "Resources/images/cameraSousa.png" },
            { moveSousa, "Resources/images/moveSousa.png" },
            { useController, "Resources/images/useController.png" },
            { pressSousa, "Resources/images/pressSousa.png" },
        }
    };
};