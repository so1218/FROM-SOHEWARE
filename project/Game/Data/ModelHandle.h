#pragma once

#include "Engine.h"

// モデルID
enum class ModelID
{
    // 基本的なモデル
    cube,
    sphere,
    plane,

    // フィールド関連
    skydome,
    field,
    axe,
    knife,

    // プレイヤー関連
    player,

    // 敵関連
    enemy,

    count
};

struct ModelDefinition
{
    ModelID id;
    const char* path;
};

class ModelHandle
{
public:
    static void Initialize(Engine* engine);
    static void Finalize();
    static const ModelData* Get(ModelID id);

private:
    static std::array<std::unique_ptr<ModelData>, static_cast<size_t>(ModelID::count)> modelHandles_;
    static bool initialized_;
    static Engine* engine_;

    static constexpr std::array<ModelDefinition, static_cast<size_t>(ModelID::count)> modelDefinitions_ =
    {
        {
            // 基本的なモデル
            { ModelID::cube,       "Assets/Models/Primitives/Cube/normalCube.obj" },
            { ModelID::sphere,     "Assets/Models/Primitives/Sphere/sphere.obj" },
            { ModelID::plane,     "Assets/Models/Primitives/Plane/plane.obj" },

            // フィールド関連
            { ModelID::skydome,  "Assets/Models/Environment/Skydome/skydome.obj" },
            { ModelID::field,  "Assets/Models/Environment/Field/field.obj" },
            { ModelID::axe,  "Assets/Models/Characters/Player/weapons/axe/Axe.obj" },
            { ModelID::knife,  "Assets/Models/Characters/Player/weapons/knife/Knife.obj" },

            // プレイヤー関連
            { ModelID::player,  "Assets/Models/Characters/Player/player.gltf" },

            // 敵関連
            { ModelID::enemy ,  "Assets/Models/Characters/Enemy/zombi.gltf" },
        }
    };
};
