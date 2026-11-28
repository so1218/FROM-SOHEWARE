#pragma once
#include "Engine.h"
#include "PropertyBinder.h"

namespace FS
{
class RendererManager;
}

struct BurnConfig
{
    float fireThreshold = 0.2f;    
    float damagePerSecond = 8.0f;  
    float tickInterval = 0.5f;     
    float afterburnTime = 2.0f;    

    void Bind(FS::PropertyBinder& binder, const std::string& prefix, const BurnConfig& defaults);
    void Draw(FS::PropertyBinder& binder, const std::string& prefix);
};

// 燃えている状態
class BurnStatus
{
public:
    // 足元の炎を調べて燃え方を進め、このフレームに入れるダメージを返す
    int Update(const FS::RendererManager* rendererManager, const FS::Vector3& footPosition, const BurnConfig& config, float deltaTime);
    void Extinguish();

    bool IsBurning() const { return burnTimer_ > 0.0f; }
    // 燃えている強さ
    float GetStrength() const;

private:
    bool isInFire_ = false;
    float burnTimer_ = 0.0f;       
    float afterburnTime_ = 0.0f;
    float fireStrength_ = 0.0f;     
    float tickTimer_ = 0.0f;
    float damageRemainder_ = 0.0f;  
};
