#pragma once
#include "GameObject.h"
#include "Model.h"
#include "Collider.h"
#include "Enemy.h"
#include "PropertyBinder.h"
#include "FloatingBehavior.h"

class EnemyManager : public FS::GameObject
{
public:
    EnemyManager(FS::Engine* engine, const std::string& groupName);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugGui() override;

    // 敵の種類ごとにマテリアルを共有する関数
    void RebuildMaterialSharing();
    void AddEnemy(); 
    // 倒された敵も含めて、保存されている数だけ作り直す
    void ResetEnemies();
    // 墜落した敵が地面で爆発するのに使う
    void SetTerrain(const FS::Terrain* terrain) { terrain_ = terrain; }

private:
    std::unique_ptr<Enemy> CreateEnemy(int index);
    void SpawnEnemies();

    FS::Engine* engine_;
    std::string managerGroupName_;
    const FS::Terrain* terrain_ = nullptr;
    std::vector<std::unique_ptr<Enemy>> enemies_;

    std::unique_ptr<FS::PropertyBinder> binder_;
    // ワスプ共通のパラメータ。ステージをまたいで共有する
    FloatingEnemyConfig floatingConfig_;
    std::unique_ptr<FS::PropertyBinder> floatingBinder_;

    int enemyCount_ = 0; // JSONに保存される全体の数。倒されても減らさない
};