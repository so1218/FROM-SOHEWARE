#include "pch.h"
#include "MoveModelCommand.h"

MoveModelCommand::MoveModelCommand(FE::Model* model, const FE::Vector3& oldPos, const FE::Vector3& newPos)
    : targetModel_(model), oldTranslation_(oldPos), newTranslation_(newPos) {}

void MoveModelCommand::Execute()
{
    targetModel_->GetTransform().translation_ = newTranslation_;
}

void MoveModelCommand::Undo()
{
    targetModel_->GetTransform().translation_ = oldTranslation_;
}