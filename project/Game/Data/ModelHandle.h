#pragma once

#include "Engine.h"

// モデルID
enum class ModelID
{
    // 基本的なモデル
    cube,
    sphere,
    plane,
    multiMesh,
    multiMaterial,
    walk,

	// フィールド関連
    skydome,
    field,
    axe,
    knife,

	// プレイヤー関連
    sneakWalk,
    shrimp,
    ryu,

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
    static ModelData* Get(ModelID id);

private:
    static std::array<std::unique_ptr<ModelData>, static_cast<size_t>(ModelID::count)> modelHandles_;
    static bool initialized_;
	static Engine* engine_;

    static constexpr std::array<ModelDefinition, static_cast<size_t>(ModelID::count)> modelDefinitions_ =
    { 
        {
            // 基本的なモデル
            { ModelID::cube,       "Resources/models/cube/normalCube.obj" },
            { ModelID::sphere,     "Resources/models/sphere/sphere.obj" },
            { ModelID::plane,     "Resources/models/plane/plane.obj" },
            { ModelID::multiMesh,  "Resources/models/multiMesh/multiMesh.obj" },
            { ModelID::multiMaterial,  "Resources/models/multiMaterial/multiMaterial.obj" },
            { ModelID::walk,  "Resources/models/animated/walk.gltf" },

            // フィールド関連
            { ModelID::skydome,  "Resources/models/skydome/skydome.obj" },
            { ModelID::field,  "Resources/models/field/field.obj" },
            { ModelID::axe,  "Resources/models/player/weapons/axe/Axe.obj" },
            { ModelID::knife,  "Resources/models/player/weapons/knife/Knife.obj" },

            // プレイヤー関連
            { ModelID::sneakWalk,  "Resources/models/animated/sneakWalk2.gltf" },
            { ModelID::shrimp,  "Resources/models/animated/Shrimp_03.gltf" },
            { ModelID::ryu,  "Resources/models/shrimp/ShrimpTailFripAnimation.gltf" },

            // 敵関連
            { ModelID::enemy ,  "Resources/models/enemy/blaze.obj" },
        }
    };
};
