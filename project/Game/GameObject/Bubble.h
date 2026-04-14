#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "PropertyBinder.h"
#include "Model.h"

class Bubble : public FE::GameObject
{
public:
    Bubble(FE::Engine* engine);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

    std::unique_ptr<FE::Model> model_;
private:
    FE::Engine* engine_;

    std::unique_ptr<FE::PropertyBinder> binder_;
};

