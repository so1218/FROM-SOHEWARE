#pragma once
#include "WaterObject.h"

class WaterManager : public FS::GameObject
{
public:
    WaterManager(FS::Engine* engine, const std::string& groupName = "Stage_Water");
    ~WaterManager() override = default;

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

    void AddWater();
    void RemoveWater(int id);

private:
    FS::Engine* engine_ = nullptr;
    std::string groupName_;
    std::vector<std::unique_ptr<WaterObject>> waters_;

    std::unique_ptr<FS::PropertyBinder> binder_;
    int waterCount_ = 0; // JSONに保存される全体の水の数
};
