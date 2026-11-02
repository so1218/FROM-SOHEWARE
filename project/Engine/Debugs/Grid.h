#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "Model.h"

class Grid : public FS::GameObject
{
public:
    Grid(FS::Engine* engine);
    ~Grid() override = default;

    void Initialize() override {};
    void Update() override {}
    void Draw() override;

private:
    FS::Engine* engine_;
    std::unique_ptr<FS::Model> model_;
};