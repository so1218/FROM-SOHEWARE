#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "Skydome.h"

namespace FS
{

// 空と太陽の光。シーンに置くと、空が描かれ、時間帯と天気に合わせて光・空の色・霧が変わる
// 置かないと太陽の光がないので、モデルは暗く (ほぼ黒く) 映る
class Environment : public GameObject
{
public:
    Environment(Engine* engine);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugGui() override;

private:
    Engine* engine_;
    std::unique_ptr<Skydome> skydome_;
};

}
