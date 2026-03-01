#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "Skybox.h"
#include "PropertyBinder.h"

class Ground : public GameObject
{
public:
    Ground(Engine* engine);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

private:
    std::unique_ptr<Model> model_;
    std::unique_ptr<Model> modelTree_;
    std::unique_ptr<Skybox> skybox_;
    std::unique_ptr<PropertyBinder> binder_;
};

