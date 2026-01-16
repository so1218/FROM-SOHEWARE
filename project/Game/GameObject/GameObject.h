#pragma once

#include "Engine.h"
#include "ModelHandle.h"
#include "AnimationHandle.h"
#include "TextureHandle.h"

class Sprite;
class Model;
class AnimationModel;

enum class GameObjectType : int
{
    Background,
    FollowCamera,
    Player,
    PlayerWeapon,
    Enemy,
    Bullet,
    Grid,
    Effect,
    UI,

    Count  
};

struct GameObjectPriority 
{
    GameObjectType type;
    int updatePriority;
};

class GameObject 
{
public:
    GameObject(Engine* engine, Camera* camera);
    virtual ~GameObject() = default;

    virtual void Initialize() {}
    virtual void Update() {}
    virtual void Draw() {}
    virtual void DebugDraw() {}
    virtual bool IsDead() const { return false; }

    virtual GameObjectType GetType() const = 0;

    int GetUpdatePriority() const { return GetPriority().updatePriority; }

private:
    const GameObjectPriority& GetPriority() const
    {
        return priorities[static_cast<int>(GetType())];
    }

    static constexpr std::array<GameObjectPriority, static_cast<size_t>(GameObjectType::Count)> priorities = 
    { 
        {
            { GameObjectType::Background,  0 },
            { GameObjectType::FollowCamera, 12 },
            { GameObjectType::Player, 10 },
            { GameObjectType::PlayerWeapon, 15 },
            { GameObjectType::Enemy, 20 },
            { GameObjectType::Bullet, 30 },
            { GameObjectType::Grid, 50 },
            { GameObjectType::Effect, 95 },
            { GameObjectType::UI, 100 }
        }
    };

protected:
	std::unique_ptr<Model> CreateModel(ModelID modelID);
    std::unique_ptr<AnimationModel> CreateAnimationModel(ModelID modelId, AnimationID animationId);
    std::unique_ptr<Sprite> CreateSprite(uint32_t textureHandle);

    Engine* engine_ = nullptr;
	Camera* camera_ = nullptr;
};