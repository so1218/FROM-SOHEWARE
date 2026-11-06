#pragma once
#include "WorldTransform.h"
#include "Structures.h"

namespace FS
{

class Engine;
class PropertyBinder;

// 落雷を管理する。形を作り、寿命と明滅を決め、雷の板と光を毎フレーム出す
class LightningSystem
{
public:
    LightningSystem(Engine* engine);
    ~LightningSystem();

    void Initialize();
    void Update();
    void DebugGui();

    void SpawnStrike(const Vector3& start, const Vector3& end);

private:
    struct Strike
    {
        std::vector<LightningPath> paths; // 0 番目が幹、それ以降が枝
        Vector3 lightPosition;
        float duration = 0.0f;
        float remainingTime = 0.0f;
        float flickerPhase = 0.0f;        // 雷ごとに明滅をずらす
    };

    // 中点を少しずつずらして分割し、ジグザグの線と枝を作る
    void GenerateFractalPath(Strike& strike, std::vector<Vector3>& points,
        const Vector3& start, const Vector3& end, int depth, float displacement, float thicknessScale);

    float CalculateFlicker(float flickerPhase) const;

    Engine* engine_ = nullptr;
    std::unique_ptr<PropertyBinder> binder_;
    LightningConfig config_;

    std::vector<Strike> strikes_;
    std::mt19937 randomEngine_;
};

}
