#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "Skybox.h"
#include "Skydome.h"
#include "PropertyBinder.h"
#include "Model.h"
#include "Terrain.h"

class Ground : public FE::GameObject
{
public:
    Ground(FE::Engine* engine);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

    FE::Terrain* GetTerrain() const { return terrain_.get(); }

private:
    FE::Engine* engine_;

    std::unique_ptr<FE::Model> model_;
    std::unique_ptr<FE::Model> modelBuilding_;
    std::unique_ptr<FE::Skydome> skydome_;
    std::unique_ptr<FE::PropertyBinder> binder_;

    std::unique_ptr<FE::Terrain> terrain_;
};

