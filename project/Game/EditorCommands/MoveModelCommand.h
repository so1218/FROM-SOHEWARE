#pragma once
#include "IEditorCommand.h"
#include "Model.h"

class MoveModelCommand : public FE::IEditorCommand
{
private:
    FE::Model* targetModel_;
    FE::Vector3 oldTranslation_;
    FE::Vector3 newTranslation_;

public:
    MoveModelCommand(FE::Model* model, const FE::Vector3& oldPos, const FE::Vector3& newPos);

    void Execute() override;

    void Undo() override;
};