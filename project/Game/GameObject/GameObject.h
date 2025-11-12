#pragma once

#include "Engine.h"

enum class GameObjectType : int
{
    Background,
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
    int drawPriority;
};

class GameObject 
{
public:
    virtual ~GameObject() = default;

    virtual void Initialize() {}
    virtual void Update() {}
    virtual void Draw() {}
    virtual void DebugDraw() {}
    virtual void ApplyGlobalVariables() {}
    virtual void SaveGlobalVariables() {}
    virtual bool IsDead() const { return false; }

    virtual GameObjectType GetType() const = 0;

    int GetUpdatePriority() const { return GetPriority().updatePriority; }

    int GetDrawPriority() const { return GetPriority().drawPriority; }

private:
    const GameObjectPriority& GetPriority() const
    {
        return priorities[static_cast<int>(GetType())];
    }

    static constexpr std::array<GameObjectPriority, static_cast<size_t>(GameObjectType::Count)> priorities = 
    { 
        {
            // 不透明
            { GameObjectType::Background,  0,   0 },
            { GameObjectType::Player,     10,  10 },
            { GameObjectType::PlayerWeapon,     15,  15 },
            { GameObjectType::Enemy,      20,  20 },
            { GameObjectType::Bullet,     30,  30 },

            // 半透明 
            { GameObjectType::Grid,     50,  50 },
            { GameObjectType::Effect,     95,  95 },

            // UI
            { GameObjectType::UI,        100, 100 }
        }
    };
};