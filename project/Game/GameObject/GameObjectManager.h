#pragma once
#include <memory>  
#include <vector>

#include "GameObject.h"

class CollisionManager;

class GameObjectManager 
{
public:
    void AddObject(std::unique_ptr<GameObject> obj);

    void Initialize();
    void Update();
    void Draw();
    void DebugDraw();

    void AddAllCollidersToManager(CollisionManager* manager);

    void ClearEnemies();

private:
    std::vector<std::unique_ptr<GameObject>> objects_;
};