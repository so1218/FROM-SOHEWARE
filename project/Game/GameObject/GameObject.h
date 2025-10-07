#pragma once

#include "Engine.h"

enum class GameObjectType : int
{
    Background,
    Player,
    Enemy,
    Bullet,
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
    virtual const char* GetGlobalVariableGroupName() const = 0;

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
            { GameObjectType::Background,  0,   0 },
            { GameObjectType::Player,     10,  10 },
            { GameObjectType::Enemy,      20,  20 },
            { GameObjectType::Bullet,     30,  30 },
            { GameObjectType::UI,        100, 100 }
        }
    };
};