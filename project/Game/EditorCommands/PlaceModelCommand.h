#pragma once
#include "IEditorCommand.h"
#include "Model.h"
#include "TestSceneHori.h"

class PlaceModelCommand : public FE::IEditorCommand
{
private:
    TestSceneHori* scene_;
    std::unique_ptr<FE::Model> model_; // Undo時はここに退避、Redo時はここからシーンへ
    FE::Model* modelRawPtr_ = nullptr; // シーン側で検索するための目印

public:
    PlaceModelCommand(TestSceneHori* scene, std::unique_ptr<FE::Model> model);

    void Execute() override;

    void Undo() override;
};