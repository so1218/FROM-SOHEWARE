#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "Skydome.h"

namespace FS
{

// 空と太陽の光
class Environment : public GameObject
{
public:
    // environmentGroupName は時間・天候・霧の設定、skydomeGroupName は空の設定を保存するグループの名前
    Environment(Engine* engine, const std::string& environmentGroupName = "Environment", const std::string& skydomeGroupName = "Skydome");

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugGui() override;

private:
    Engine* engine_;
    std::unique_ptr<Skydome> skydome_;
};

}
