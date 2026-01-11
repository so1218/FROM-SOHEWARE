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
            { white1x1, "Assets/Textures/white1x1.png" },
            { uvChecker,"Assets/Textures/uvChecker.png" },
            { toonRamp,   "Assets/Textures/Ramps/toonRamp_3.png" },

            // dds
            { skyboxCubemapBlack,   "Assets/Textures/Environments/black_cube.dds" },
            { skyboxCubemap,   "Assets/Textures/Environments/rostock_laage_airport_4k.dds" },

            { skydome,   "Assets/Textures/sky_sphere.png" },
            { axe,   "Assets/Textures/Woodcutter-Axe.jpg" },
            { knife,   "Assets/Textures/KnifeTexture..jpg" },

            { num1, "Assets/Textures/UI/numFont/1.png" },
            { num2, "Assets/Textures/UI/numFont/2.png" },
            { num3, "Assets/Textures/UI/numFont/3.png" },
            { num4, "Assets/Textures/UI/numFont/4.png" },
            { num5, "Assets/Textures/UI/numFont/5.png" },
            { num6, "Assets/Textures/UI/numFont/6.png" },
            { num7, "Assets/Textures/UI/numFont/7.png" },
            { num8, "Assets/Textures/UI/numFont/8.png" },
            { num9, "Assets/Textures/UI/numFont/9.png" },
            { num0, "Assets/Textures/UI/numFont/0.png" },
                                    
            { coron, "Assets/Textures/UI/numFont/coron.png" },
            { hpGage, "Assets/Textures/hpGage.png" },

            { cardTextKnife, "Assets/Textures/UI/numFont/9.png" },
            { cardTextAxe, "Assets/Textures/UI/numFont/0.png" },

            { title, "Assets/Textures/title.png" },
            { clear, "Assets/Textures/clear.png" },
            { axeLevelUp, "Assets/Textures/axeLevelUp.png" },
            { knifeLevelUp, "Assets/Textures/knifeLevelUp.png" },
            { hpUp, "Assets/Textures/hpUp.png" },
            { speedUp, "Assets/Textures/speedUp.png" },
            { heal, "Assets/Textures/heal.png" },

            { cameraSousa, "Assets/Textures/cameraSousa.png" },
            { moveSousa, "Assets/Textures/moveSousa.png" },
            { useController, "Assets/Textures/useController.png" },
            { pressSousa, "Assets/Textures/pressSousa.png" },
            { ikinokore, "Assets/Textures/ikinokore.png" },

            { noise1, "Assets/Textures/Noise/Noise_Gradients/T_Random_59.png" },
        }
    };
};