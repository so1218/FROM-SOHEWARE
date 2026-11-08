#pragma once
#include "GameObject.h"
#include "Model.h"
#include "Collider.h"
#include "Enemy.h"
#include "PropertyBinder.h"
#include "FloatingBehavior.h"

class CoinManager;

class EnemyManager : public FS::GameObject
{
public:
    // terrain は墜落した敵が地面で爆発するのに、coinManager は倒された敵の位置でお金を出すのに使う
    EnemyManager(FS::Engine* engine, const std::string& groupName, const FS::Terrain* terrain, CoinManager* coinManager);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugGui() override;

    // 敵の種類ごとにマテリアルを共有する関数
    void RebuildMaterialSharing();
    void AddEnemy(); 
    // 倒された敵も含めて、保存されている数だけ作り直す
    void ResetEnemies();

private:
    std::unique_ptr<Enemy> CreateEnemy(int index);
    void SpawnEnemies();

    FS::Engine* engine_;
    std::string managerGroupName_;
    const FS::Terrain* terrain_ = nullptr;
    CoinManager* coinManager_ = nullptr;

    // 全機のスラスターがマテリアルを共有する元のモデル
    std::unique_ptr<FS::Model> thrusterModel_;

    std::vector<std::unique_ptr<Enemy>> enemies_;

    std::unique_ptr<FS::PropertyBinder> binder_;
    // 共通のパラメータ
    FloatingEnemyConfig floatingConfig_;
    std::unique_ptr<FS::PropertyBinder> floatingBinder_;

    int enemyCount_ = 0; // JSONに保存される全体の数
};