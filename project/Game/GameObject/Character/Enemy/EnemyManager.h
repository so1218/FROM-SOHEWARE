#pragma once
#include "GameObject.h"
#include "Model.h"
#include "Collider.h"
#include "Enemy.h"
#include "PropertyBinder.h"

class EnemyManager : public FE::GameObject
{
public:
    EnemyManager(FE::Engine* engine, const std::string& groupName);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

    void AddEnemy(); 

private:
    FE::Engine* engine_;
    std::string managerGroupName_;
    std::vector<std::unique_ptr<Enemy>> enemies_;

    std::unique_ptr<FE::PropertyBinder> binder_;
    int enemyCount_ = 0; // JSONに保存される全体の数
};