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
    axe,
    knife,
    skyboxCubemap,

    // particle
    circle_01,
    circle_02,
    circle_03,
    circle_04,
    circle_05,
    dirt_01,
    dirt_02,
    dirt_03,
    fire_01,
    fire_02,
    flame_01,
    flame_02,
    flame_03,
    flame_04,
    flame_05,
    flame_06,
    flare_01,
    light_01,
    light_02,
    light_03,
    magic_01,
    magic_02,
    magic_03,
    magic_04,
    magic_05,
    muzzle_01,
    muzzle_02,
    muzzle_03,
    muzzle_04,
    muzzle_05,
    scorch_01,
    scorch_02,
    scorch_03,
    scratch_01,
    slash_01,
    slash_02,
    slash_03,
    slash_04,
    smoke_01,
    smoke_02,
    smoke_03,
    smoke_04,
    smoke_05,
    smoke_06,
    smoke_07,
    smoke_08,
    smoke_09,
    smoke_10,
    spark_01,
    spark_02,
    spark_03,
    spark_04,
    spark_05,
    spark_06,
    spark_07,
    star_01,
    star_02,
    star_03,
    star_04,
    star_05,
    star_06,
    star_07,
    star_08,
    star_09,

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
            { skydome,   "Resources/images/sky_sphere.png" },
            { axe,   "Resources/images/Woodcutter-Axe.jpg" },
            { knife,   "Resources/images/KnifeTexture..jpg" },
            { skyboxCubemap,   "Resources/images/rostock_laage_airport_4k.dds" },

			// パーティクル
            { circle_01,   "Resources/images/particles/circle_01.png" },
            { circle_02,   "Resources/images/particles/circle_02.png" },
            { circle_03,   "Resources/images/particles/circle_03.png" },
            { circle_04,   "Resources/images/particles/circle_04.png" },
            { circle_05,   "Resources/images/particles/circle_05.png" },
            { dirt_01,   "Resources/images/particles/dirt_01.png" },
            { dirt_02,   "Resources/images/particles/dirt_02.png" },
            { dirt_03,   "Resources/images/particles/dirt_03.png" },
            { fire_01,   "Resources/images/particles/fire_01.png" },
            { fire_02,   "Resources/images/particles/fire_02.png" },
            { flame_01,   "Resources/images/particles/flame_01.png" },
            { flame_02,   "Resources/images/particles/flame_02.png" },
            { flame_03,   "Resources/images/particles/flame_03.png" },
            { flame_04,   "Resources/images/particles/flame_04.png" },
            { flame_05,   "Resources/images/particles/flame_05.png" },
            { flame_06,   "Resources/images/particles/flame_06.png" },
            { flare_01,   "Resources/images/particles/flare_01.png" },
            { light_01,   "Resources/images/particles/light_01.png" },
            { light_02,   "Resources/images/particles/light_02.png" },
            { light_03,   "Resources/images/particles/light_03.png" },
            { magic_01,   "Resources/images/particles/magic_01.png" },
            { magic_02,   "Resources/images/particles/magic_02.png" },
            { magic_03,   "Resources/images/particles/magic_03.png" },
            { magic_04,   "Resources/images/particles/magic_04.png" },
            { magic_05,   "Resources/images/particles/magic_05.png" },
            { muzzle_01,   "Resources/images/particles/muzzle_01.png" },
            { muzzle_02,   "Resources/images/particles/muzzle_02.png" },
            { muzzle_03,   "Resources/images/particles/muzzle_03.png" },
            { muzzle_04,   "Resources/images/particles/muzzle_04.png" },
            { muzzle_05,   "Resources/images/particles/muzzle_05.png" },
            { scorch_01,   "Resources/images/particles/scorch_01.png" },
            { scorch_02,   "Resources/images/particles/scorch_02.png" },
            { scorch_03,   "Resources/images/particles/scorch_03.png" },
            { scratch_01,   "Resources/images/particles/scratch_01.png" },
            { slash_01,   "Resources/images/particles/slash_01.png" },
            { slash_02,   "Resources/images/particles/slash_02.png" },
            { slash_03,   "Resources/images/particles/slash_03.png" },
            { slash_04,   "Resources/images/particles/slash_04.png" },
            { smoke_01,   "Resources/images/particles/smoke_01.png" },
            { smoke_02,   "Resources/images/particles/smoke_02.png" },
            { smoke_03,   "Resources/images/particles/smoke_03.png" },
            { smoke_04,   "Resources/images/particles/smoke_04.png" },
            { smoke_05,   "Resources/images/particles/smoke_05.png" },
            { smoke_06,   "Resources/images/particles/smoke_06.png" },
            { smoke_07,   "Resources/images/particles/smoke_07.png" },
            { smoke_08,   "Resources/images/particles/smoke_08.png" },
            { smoke_09,   "Resources/images/particles/smoke_09.png" },
            { smoke_10,   "Resources/images/particles/smoke_10.png" },
            { spark_01,   "Resources/images/particles/spark_01.png" },
            { spark_02,   "Resources/images/particles/spark_02.png" },
            { spark_03,   "Resources/images/particles/spark_03.png" },
            { spark_04,   "Resources/images/particles/spark_04.png" },
            { spark_05,   "Resources/images/particles/spark_05.png" },
            { spark_06,   "Resources/images/particles/spark_06.png" },
            { spark_07,   "Resources/images/particles/spark_07.png" },
            { star_01,   "Resources/images/particles/star_01.png" },
            { star_02,   "Resources/images/particles/star_02.png" },
            { star_03,   "Resources/images/particles/star_03.png" },
            { star_04,   "Resources/images/particles/star_04.png" },
            { star_05,   "Resources/images/particles/star_05.png" },
            { star_06,   "Resources/images/particles/star_06.png" },
            { star_07,   "Resources/images/particles/star_07.png" },
            { star_08,   "Resources/images/particles/star_08.png" },
            { star_09,   "Resources/images/particles/star_09.png" },

        }
    };
};