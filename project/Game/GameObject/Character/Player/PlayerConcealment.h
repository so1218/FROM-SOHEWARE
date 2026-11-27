#pragma once
#include "Engine.h"
#include "PropertyBinder.h"

class GrassField;

class PlayerConcealment
{
public:
    explicit PlayerConcealment(FS::Engine* engine);

    void BindProperties(FS::PropertyBinder& binder);
    void DrawProperties(FS::PropertyBinder& binder);

    void SetGrassField(const GrassField* grassField) { grassField_ = grassField; }

    // 這って草に入っているか、置いた霧の中にいれば隠れている
    void Update(const FS::Vector3& footPosition, const FS::Vector3& bodyCenter, bool isCrawling, bool isControllable);
    // 隠れていないことにして、画面の演出も消す
    void Reset();

    bool IsHidden() const { return isHidden_; }
    // 焼けていない草の中にいるか
    bool IsInGrass() const { return isInGrass_; }
    bool IsInFogVolume() const { return isInFogVolume_; }

private:
    bool IsInsideFogVolume(const FS::Vector3& position) const;

    FS::Engine* engine_ = nullptr;
    const GrassField* grassField_ = nullptr;

    bool isInGrass_ = false;
    bool isOnBurntGrass_ = false;
    bool isInFogVolume_ = false;
    bool isHidden_ = false;

    // 画面の演出の強さ
    float effectRatio_ = 0.0f;

    // 燃え進み具合がこれを超えた草には隠れられない
    float burntHideThreshold_ = 0.3f;

    float vignetteAmount_ = 0.6f;
    FS::Vector3 vignetteColor_ = { 0.01f, 0.02f, 0.03f };
    float vignetteStart_ = 0.25f;
    float vignetteEnd_ = 0.85f;
    float fadeInTime_ = 0.25f;
    float fadeOutTime_ = 0.5f;
    // 隠れている間の、世界の音の大きさとこもり具合
    float hiddenWorldVolume_ = 0.7f;
    float hiddenWorldMuffle_ = 0.6f;
};
