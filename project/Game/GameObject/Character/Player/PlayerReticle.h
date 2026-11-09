#pragma once
#include "PropertyBinder.h"
#include "Sprite.h"

class PlayerReticle
{
public:
    struct Config
    {
        float lineThickness = 2.0f; 
        float lineLength = 12.0f;   
        float maxGap = 35.0f;       
        float minGap = 5.0f;        
        float centerDotSize = 3.0f; 

        float focusTime = 1.0f;     
        float expandSpeed = 8.0f;   

        float threshold = 0.8f;     
        float midRatio = 0.35f;     
        float finalExponent = 3.0f; 
    };

    PlayerReticle(FS::Engine* engine);
    ~PlayerReticle() = default;

    void Initialize();
    void Update(bool isMoving, bool isAiming);
    void Draw();
    void DebugGui();

    void OnShootRecoil();
    void Reset();

    // フォーカス率を取得
    float GetFocusRatio() const { return focusRatio_; }

private:
    FS::Engine* engine_ = nullptr;
    std::unique_ptr<FS::Sprite> sprite_;
    std::unique_ptr<FS::PropertyBinder> binder_;

    Config config_;

    float focusTimer_ = 0.0f;
    float focusRatio_ = 0.0f;
    float reticleAlpha_ = 0.0f;
};