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
    num1, num2, num3, num4, num5,
    num6, num7, num8, num9, num0,
    
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

    lut_natural,

    normal_01,

    // 全てのテクスチャIDの数
    TEXTURES_COUNT
};

enum class TextureType
{
    Albedo,     // 通常
    Normal,     // ノーマルマップ
    Toon,       // トゥーン
    Noise,      // ノイズ
    CubeMap     // スカイボックス (DDS)
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

    // タイプを取得する関数 (Binderで使う)
    static TextureType GetType(TextureID id);

    // IDからファイル名だけを取得する関数
    static std::string GetFileName(TextureID id);

private:
    static std::array<uint32_t, TEXTURES_COUNT> textureHandles_;
    static bool initialized_;

    // 判定したタイプを保存しておく配列
    static std::array<TextureType, TEXTURES_COUNT> textureTypes_;

    static constexpr std::array<TextureDefinition, TEXTURES_COUNT> textureDefinitions_ = 
    { 
        {
            { white1x1, "Assets/Textures/white1x1.png" },
            { uvChecker,"Assets/Textures/uvChecker.png" },
            { toonRamp,   "Assets/Textures/Ramps/toonRamp_3.png" },

            // dds
            { skyboxCubemapBlack,   "Assets/Textures/Environments/black_cube.dds" },
            { skyboxCubemap,   "Assets/Textures/Environments/night.dds" },

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

            { noise1, "Assets/Textures/Noise/noise_59.png" },

            { lut_natural, "Assets/Textures/LUTs/RGBTable16x1.png" },

            { normal_01, "Assets/Textures/Normal/normal_23.png" },
        }
    };
};