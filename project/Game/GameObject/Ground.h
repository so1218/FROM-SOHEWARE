#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "Skybox.h"
#include "PropertyBinder.h"
#include "Model.h"

class Ground : public FE::GameObject
{
public:
    Ground(FE::Engine* engine);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

private:
    FE::Engine* engine_;

    std::unique_ptr<FE::Model> model_;
    std::unique_ptr<FE::Model> modelTree_;
    std::unique_ptr<FE::Model> modelRock_;
    std::unique_ptr<FE::Skybox> skybox_;
    std::unique_ptr<FE::PropertyBinder> binder_;

    std::vector<FE::Vector3> treePositions_;
};

