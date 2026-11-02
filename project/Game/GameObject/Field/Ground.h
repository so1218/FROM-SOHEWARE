#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "Skydome.h"
#include "PropertyBinder.h"
#include "Model.h"
#include "Terrain.h"
#include "WorldInteractionSystem.h"

class Ground : public FS::GameObject
{
public:
    Ground(FS::Engine* engine);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

    FS::Terrain* GetTerrain() const { return terrain_.get(); }

private:
    FS::Engine* engine_;

    std::unique_ptr<FS::Skydome> skydome_;
    std::unique_ptr<FS::PropertyBinder> binder_;
    std::unique_ptr<FS::Terrain> terrain_;
    std::unique_ptr<FS::WorldInteractionSystem> interactionSystem_;
};

