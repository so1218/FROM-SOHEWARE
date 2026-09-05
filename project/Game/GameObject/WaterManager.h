#pragma once
#include "WaterObject.h"

class WaterManager : public FE::GameObject
{
public:
    WaterManager(FE::Engine* engine, const std::string& groupName = "Stage_Water");
    ~WaterManager() override = default;

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

    void AddWater();
    void RemoveWater(int id);

private:
    FE::Engine* engine_ = nullptr;
    std::string groupName_;
    std::vector<std::unique_ptr<WaterObject>> waters_;

    std::unique_ptr<FE::PropertyBinder> binder_;
    int waterCount_ = 0; // JSONに保存される全体の水の数
};
