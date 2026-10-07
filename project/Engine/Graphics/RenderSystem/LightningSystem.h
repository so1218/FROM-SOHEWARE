#pragma once
#include "WorldTransform.h"
#include "Structures.h"

namespace FE
{

class Engine;
class Camera;
class PropertyBinder;

class LightningSystem
{
public:
    LightningSystem(Engine* engine);
    ~LightningSystem();

    void Initialize();
    void Update();
    void DebugDraw();

    // 外部から雷を落とす命令
    void SpawnStrike(const Vector3& start, const Vector3& end);

private:
    void TriggerSingleStrike(const Vector3& start, const Vector3& end);

    Engine* engine_ = nullptr;
    std::unique_ptr<PropertyBinder> binder_;

    LightningConfig config_;

    // アクティブなポイントライトの管理システム
    struct ActiveLight 
    {
        Vector3 position{ 0.0f, 0.0f, 0.0f };
        float maxDuration = 0.0f;   // 初期寿命
        float currentDuration = 0.0f; // 残り寿命
        float seed = 0.0f;          // 明滅を同期するためのシード値
    };
    std::vector<ActiveLight> activeLights_;
    std::mt19937 rng_{ 1337 };
};

}