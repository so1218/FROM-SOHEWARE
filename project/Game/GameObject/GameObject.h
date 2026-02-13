#pragma once

#include "Engine.h"
#include "ModelHandle.h"
#include "AnimationHandle.h"
#include "TextureHandleManager.h"

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
    GameObject(Engine* engine);
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
    // モデルのみの生成
    std::unique_ptr<Model> CreateModel(const std::string& modelName);
    // アニメーション付きモデルの生成
    std::unique_ptr<AnimationModel> CreateAnimationModel(const std::string& modelName, const std::string& animationName);
    std::unique_ptr<Sprite> CreateSprite(const std::string& textureName);

    Engine* engine_ = nullptr;
};