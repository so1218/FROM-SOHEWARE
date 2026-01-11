#pragma once
#include "Engine.h"

// テクスチャID
enum TextureID
{
    white1x1,
    uvChecker,
    toonRamp,

    // dds
    skyboxCubemapBlack,
    skyboxCubemap,

    // PlayScene     
    skydome,
    axe,
    knife,

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
    ikinokore,

    noise1,

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
            { white1x1, "Assets/images/white1x1.png" },
            { uvChecker,"Assets/images/uvChecker.png" },
            { toonRamp,   "Assets/images/toonRamp_3.png" },

            // dds
            { skyboxCubemapBlack,   "Assets/images/black_cube.dds" },
            { skyboxCubemap,   "Assets/images/rostock_laage_airport_4k.dds" },

            { skydome,   "Assets/images/sky_sphere.png" },
            { axe,   "Assets/images/Woodcutter-Axe.jpg" },
            { knife,   "Assets/images/KnifeTexture..jpg" },

            { num1, "Assets/images/numFont/1.png" },
            { num2, "Assets/images/numFont/2.png" },
            { num3, "Assets/images/numFont/3.png" },
            { num4, "Assets/images/numFont/4.png" },
            { num5, "Assets/images/numFont/5.png" },
            { num6, "Assets/images/numFont/6.png" },
            { num7, "Assets/images/numFont/7.png" },
            { num8, "Assets/images/numFont/8.png" },
            { num9, "Assets/images/numFont/9.png" },
            { num0, "Assets/images/numFont/0.png" },

            { coron, "Assets/images/numFont/coron.png" },
            { hpGage, "Assets/images/hpGage.png" },

            { cardTextKnife, "Assets/images/numFont/9.png" },
            { cardTextAxe, "Assets/images/numFont/0.png" },

            { title, "Assets/images/title.png" },
            { clear, "Assets/images/clear.png" },
            { axeLevelUp, "Assets/images/axeLevelUp.png" },
            { knifeLevelUp, "Assets/images/knifeLevelUp.png" },
            { hpUp, "Assets/images/hpUp.png" },
            { speedUp, "Assets/images/speedUp.png" },
            { heal, "Assets/images/heal.png" },

            { cameraSousa, "Assets/images/cameraSousa.png" },
            { moveSousa, "Assets/images/moveSousa.png" },
            { useController, "Assets/images/useController.png" },
            { pressSousa, "Assets/images/pressSousa.png" },
            { ikinokore, "Assets/images/ikinokore.png" },

            { noise1, "Assets/images/noise/Noise_Gradients/T_Random_59.png" },
        }
    };
};