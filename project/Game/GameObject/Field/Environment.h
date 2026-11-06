#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "Skydome.h"

// 空と、時間帯や天気による光の変化
class Environment : public FS::GameObject
{
public:
    Environment(FS::Engine* engine);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugGui() override;

private:
    FS::Engine* engine_;
    std::unique_ptr<FS::Skydome> skydome_;
};
