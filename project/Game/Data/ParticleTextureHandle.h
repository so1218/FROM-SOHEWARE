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
            { white1x1Particle, "Assets/Textures/white1x1.png" },
            { circle_01,    "Assets/Textures/Particles/circle_01.png" },
            { circle_02,    "Assets/Textures/Particles/circle_02.png" },
            { circle_03,    "Assets/Textures/Particles/circle_03.png" },
            { circle_04,    "Assets/Textures/Particles/circle_04.png" },
            { circle_05,    "Assets/Textures/Particles/circle_05.png" },
            { dirt_01,      "Assets/Textures/Particles/dirt_01.png" },
            { dirt_02,      "Assets/Textures/Particles/dirt_02.png" },
            { dirt_03,      "Assets/Textures/Particles/dirt_03.png" },
            { fire_01,      "Assets/Textures/Particles/fire_01.png" },
            { fire_02,      "Assets/Textures/Particles/fire_02.png" },
            { flame_01,     "Assets/Textures/Particles/flame_01.png" },
            { flame_02,     "Assets/Textures/Particles/flame_02.png" },
            { flame_03,     "Assets/Textures/Particles/flame_03.png" },
            { flame_04,     "Assets/Textures/Particles/flame_04.png" },
            { flame_05,     "Assets/Textures/Particles/flame_05.png" },
            { flame_06,     "Assets/Textures/Particles/flame_06.png" },
            { flare_01,     "Assets/Textures/Particles/flare_01.png" },
            { light_01,     "Assets/Textures/Particles/light_01.png" },
            { light_02,     "Assets/Textures/Particles/light_02.png" },
            { light_03,     "Assets/Textures/Particles/light_03.png" },
            { magic_01,     "Assets/Textures/Particles/magic_01.png" },
            { magic_02,     "Assets/Textures/Particles/magic_02.png" },
            { magic_03,     "Assets/Textures/Particles/magic_03.png" },
            { magic_04,     "Assets/Textures/Particles/magic_04.png" },
            { magic_05,     "Assets/Textures/Particles/magic_05.png" },
            { muzzle_01,    "Assets/Textures/Particles/muzzle_01.png" },
            { muzzle_02,    "Assets/Textures/Particles/muzzle_02.png" },
            { muzzle_03,    "Assets/Textures/Particles/muzzle_03.png" },
            { muzzle_04,    "Assets/Textures/Particles/muzzle_04.png" },
            { muzzle_05,    "Assets/Textures/Particles/muzzle_05.png" },
            { scorch_01,    "Assets/Textures/Particles/scorch_01.png" },
            { scorch_02,    "Assets/Textures/Particles/scorch_02.png" },
            { scorch_03,    "Assets/Textures/Particles/scorch_03.png" },
            { scratch_01,   "Assets/Textures/Particles/scratch_01.png" },
            { slash_01,     "Assets/Textures/Particles/slash_01.png" },
            { slash_02,     "Assets/Textures/Particles/slash_02.png" },
            { slash_03,     "Assets/Textures/Particles/slash_03.png" },
            { slash_04,     "Assets/Textures/Particles/slash_04.png" },
            { smoke_01,     "Assets/Textures/Particles/smoke_01.png" },
            { smoke_02,     "Assets/Textures/Particles/smoke_02.png" },
            { smoke_03,     "Assets/Textures/Particles/smoke_03.png" },
            { smoke_04,     "Assets/Textures/Particles/smoke_04.png" },
            { smoke_05,     "Assets/Textures/Particles/smoke_05.png" },
            { smoke_06,     "Assets/Textures/Particles/smoke_06.png" },
            { smoke_07,     "Assets/Textures/Particles/smoke_07.png" },
            { smoke_08,     "Assets/Textures/Particles/smoke_08.png" },
            { smoke_09,     "Assets/Textures/Particles/smoke_09.png" },
            { smoke_10,     "Assets/Textures/Particles/smoke_10.png" },
            { spark_01,     "Assets/Textures/Particles/spark_01.png" },
            { spark_02,     "Assets/Textures/Particles/spark_02.png" },
            { spark_03,     "Assets/Textures/Particles/spark_03.png" },
            { spark_04,     "Assets/Textures/Particles/spark_04.png" },
            { spark_05,     "Assets/Textures/Particles/spark_05.png" },
            { spark_06,     "Assets/Textures/Particles/spark_06.png" },
            { spark_07,     "Assets/Textures/Particles/spark_07.png" },
            { star_01,      "Assets/Textures/Particles/star_01.png" },
            { star_02,      "Assets/Textures/Particles/star_02.png" },
            { star_03,      "Assets/Textures/Particles/star_03.png" },
            { star_04,      "Assets/Textures/Particles/star_04.png" },
            { star_05,      "Assets/Textures/Particles/star_05.png" },
            { star_06,      "Assets/Textures/Particles/star_06.png" },
            { star_07,      "Assets/Textures/Particles/star_07.png" },
            { star_08,      "Assets/Textures/Particles/star_08.png" },
            { star_09,      "Assets/Textures/Particles/star_09.png" },

            { noise_39,      "Assets/Textures/Noise/Noise_Gradients/T_Random_59.png" },
        }
    };
};
