#pragma once

namespace FE
{


// すべての操作（コマンド）の基底クラス
class IEditorCommand
{
public:
    virtual ~IEditorCommand() = default;
    virtual void Execute() = 0; // 実行 / Redo
    virtual void Undo() = 0;    // 取り消し
};

// Undo/Redo の履歴を管理するクラス
class EditorHistoryManager
{
public:
    static EditorHistoryManager& GetInstance()
    {
        static EditorHistoryManager instance;
        return instance;
    }

    // 新しい操作を実行して履歴に積む
    void AddAndExecute(std::unique_ptr<IEditorCommand> command);

    // Ctrl+Z / Ctrl+Y の入力をチェックして実行
    void Update();

    void Undo();

    void Redo();

    void Clear();

private:
    EditorHistoryManager() = default;
    std::vector<std::unique_ptr<IEditorCommand>> undoStack_;
    std::vector<std::unique_ptr<IEditorCommand>> redoStack_;
};

}