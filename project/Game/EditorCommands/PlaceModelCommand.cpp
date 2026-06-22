#include "pch.h"
#include "PlaceModelCommand.h"
#include "TestSceneHori.h"

PlaceModelCommand::PlaceModelCommand(TestSceneHori* scene, std::unique_ptr<FE::Model> model)
    : scene_(scene), model_(std::move(model))
{
    modelRawPtr_ = model_.get();
}

void PlaceModelCommand::Execute()
{
    // 所有権をシーンに渡す
    scene_->AddPlacedModel(std::move(model_));
}

void PlaceModelCommand::Undo()
{
    // 所有権をシーンから引き抜いて自分の中に退避（画面から消えるがメモリは生きている）
    model_ = scene_->RemovePlacedModel(modelRawPtr_);
}