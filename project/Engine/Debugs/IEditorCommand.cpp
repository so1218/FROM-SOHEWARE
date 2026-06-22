#include "pch.h"
#include "IEditorCommand.h"
#include "Input.h"

namespace FE
{

// 新しい操作を実行して履歴に積む
void EditorHistoryManager::AddAndExecute(std::unique_ptr<IEditorCommand> command)
{
    command->Execute();
    undoStack_.push_back(std::move(command));
    redoStack_.clear();

    std::string log = "[History] Command Executed. Current Undo Stack Size: " + std::to_string(undoStack_.size()) + "\n";
    OutputDebugStringA(log.c_str());
}

// Ctrl+Z / Ctrl+Y の入力をチェックして実行
void EditorHistoryManager::Update()
{
    bool isCtrlDown = (Input::GetInstance().IsKeyPressed(DIK_LCONTROL) || Input::GetInstance().IsKeyPressed(DIK_RCONTROL));

    if (isCtrlDown)
    {
        if (Input::GetInstance().IsKeyTriggered(DIK_Z)) { Undo(); }
        if (Input::GetInstance().IsKeyTriggered(DIK_Y)) { Redo(); }
    }
}

void EditorHistoryManager::Undo()
{
    if (undoStack_.empty()) return;

    auto command = std::move(undoStack_.back());
    undoStack_.pop_back();

    command->Undo();
    redoStack_.push_back(std::move(command)); // Redo用に移動
}

void EditorHistoryManager::Redo()
{
    if (redoStack_.empty()) return;

    auto command = std::move(redoStack_.back());
    redoStack_.pop_back();

    command->Execute();
    undoStack_.push_back(std::move(command)); // Undo用に移動
}

void EditorHistoryManager::Clear()
{
    undoStack_.clear();
    redoStack_.clear();
}

}