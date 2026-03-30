#pragma once
#include "Engine.h"
#include "GameObject.h"

class Grid : public FE::GameObject
{
public:
    Grid(FE::Engine* engine);
    ~Grid() override = default;

    void Initialize() override {};
    void Update() override {}
    void Draw() override;

private:
    std::unique_ptr<FE::Model> model_;

};