#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "PropertyBinder.h"

class Bubble : public GameObject
{
public:
    Bubble(Engine* engine);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

private:
    std::unique_ptr<Model> model_;
    std::unique_ptr<PropertyBinder> binder_;
};

