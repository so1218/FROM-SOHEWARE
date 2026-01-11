#pragma once
#include <vector>  
#include <string>
#include <array>
#include <cstdint>

class Engine;

enum ParticleTextureID
{
    white1x1Particle,
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

    // ノイズテキスチャ
    noise_39,

    // 全てのパーティクルテクスチャIDの数
    PARTICLE_TEXTURES_COUNT
};

struct ParticleTextureDefinition
{
    ParticleTextureID id;
    const char* path;
};

class ParticleTextureHandle
{
public:
    // 初期化
    static void Initialize(Engine* engine);

    // IDからテクスチャハンドルを取得
    static uint32_t Get(ParticleTextureID id);

    // エディタ用：テクスチャ定義を参照
    static const auto& GetDefinitions() { return particleTextureDefinitions_; }

    // UI用：テクスチャ名リストを取得（キャッシュあり）
    static const std::vector<std::string>& GetTextureNames();

    // ImGui Comboにそのまま使えるconst char*配列を取得
    static const std::vector<const char*>& GetTextureItems();

private:
    static std::array<uint32_t, PARTICLE_TEXTURES_COUNT> particleTextureHandles_;
    static bool initialized_;

    // キャッシュ用変数
    static std::vector<std::string> cachedTextureNames_;
    static std::vector<const char*> cachedTextureItems_;

    // パーティクル専用の定義リスト
    static constexpr std::array<ParticleTextureDefinition, PARTICLE_TEXTURES_COUNT> particleTextureDefinitions_ =
    {
        {
            { white1x1Particle, "Assets/images/white1x1.png" },
            { circle_01,    "Assets/images/particles/circle_01.png" },
            { circle_02,    "Assets/images/particles/circle_02.png" },
            { circle_03,    "Assets/images/particles/circle_03.png" },
            { circle_04,    "Assets/images/particles/circle_04.png" },
            { circle_05,    "Assets/images/particles/circle_05.png" },
            { dirt_01,      "Assets/images/particles/dirt_01.png" },
            { dirt_02,      "Assets/images/particles/dirt_02.png" },
            { dirt_03,      "Assets/images/particles/dirt_03.png" },
            { fire_01,      "Assets/images/particles/fire_01.png" },
            { fire_02,      "Assets/images/particles/fire_02.png" },
            { flame_01,     "Assets/images/particles/flame_01.png" },
            { flame_02,     "Assets/images/particles/flame_02.png" },
            { flame_03,     "Assets/images/particles/flame_03.png" },
            { flame_04,     "Assets/images/particles/flame_04.png" },
            { flame_05,     "Assets/images/particles/flame_05.png" },
            { flame_06,     "Assets/images/particles/flame_06.png" },
            { flare_01,     "Assets/images/particles/flare_01.png" },
            { light_01,     "Assets/images/particles/light_01.png" },
            { light_02,     "Assets/images/particles/light_02.png" },
            { light_03,     "Assets/images/particles/light_03.png" },
            { magic_01,     "Assets/images/particles/magic_01.png" },
            { magic_02,     "Assets/images/particles/magic_02.png" },
            { magic_03,     "Assets/images/particles/magic_03.png" },
            { magic_04,     "Assets/images/particles/magic_04.png" },
            { magic_05,     "Assets/images/particles/magic_05.png" },
            { muzzle_01,    "Assets/images/particles/muzzle_01.png" },
            { muzzle_02,    "Assets/images/particles/muzzle_02.png" },
            { muzzle_03,    "Assets/images/particles/muzzle_03.png" },
            { muzzle_04,    "Assets/images/particles/muzzle_04.png" },
            { muzzle_05,    "Assets/images/particles/muzzle_05.png" },
            { scorch_01,    "Assets/images/particles/scorch_01.png" },
            { scorch_02,    "Assets/images/particles/scorch_02.png" },
            { scorch_03,    "Assets/images/particles/scorch_03.png" },
            { scratch_01,   "Assets/images/particles/scratch_01.png" },
            { slash_01,     "Assets/images/particles/slash_01.png" },
            { slash_02,     "Assets/images/particles/slash_02.png" },
            { slash_03,     "Assets/images/particles/slash_03.png" },
            { slash_04,     "Assets/images/particles/slash_04.png" },
            { smoke_01,     "Assets/images/particles/smoke_01.png" },
            { smoke_02,     "Assets/images/particles/smoke_02.png" },
            { smoke_03,     "Assets/images/particles/smoke_03.png" },
            { smoke_04,     "Assets/images/particles/smoke_04.png" },
            { smoke_05,     "Assets/images/particles/smoke_05.png" },
            { smoke_06,     "Assets/images/particles/smoke_06.png" },
            { smoke_07,     "Assets/images/particles/smoke_07.png" },
            { smoke_08,     "Assets/images/particles/smoke_08.png" },
            { smoke_09,     "Assets/images/particles/smoke_09.png" },
            { smoke_10,     "Assets/images/particles/smoke_10.png" },
            { spark_01,     "Assets/images/particles/spark_01.png" },
            { spark_02,     "Assets/images/particles/spark_02.png" },
            { spark_03,     "Assets/images/particles/spark_03.png" },
            { spark_04,     "Assets/images/particles/spark_04.png" },
            { spark_05,     "Assets/images/particles/spark_05.png" },
            { spark_06,     "Assets/images/particles/spark_06.png" },
            { spark_07,     "Assets/images/particles/spark_07.png" },
            { star_01,      "Assets/images/particles/star_01.png" },
            { star_02,      "Assets/images/particles/star_02.png" },
            { star_03,      "Assets/images/particles/star_03.png" },
            { star_04,      "Assets/images/particles/star_04.png" },
            { star_05,      "Assets/images/particles/star_05.png" },
            { star_06,      "Assets/images/particles/star_06.png" },
            { star_07,      "Assets/images/particles/star_07.png" },
            { star_08,      "Assets/images/particles/star_08.png" },
            { star_09,      "Assets/images/particles/star_09.png" },

            { noise_39,      "Assets/images/noise/Noise_Gradients/T_Random_59.png" },
        }
    };
};
