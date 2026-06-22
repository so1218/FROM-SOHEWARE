#include "pch.h"
#include "AssetBrowserWindow.h"
#include "ModelManager.h"

namespace FE
{

void AssetBrowserWindow::Draw()
{
    ImGui::Begin("Assets");

    // ModelManagerからロード済みのモデル名一覧を取得
    std::vector<std::string> modelNames = ModelManager::GetInstance().GetLoadedModelNames();

    for (const auto& name : modelNames)
    {
        // 選択可能なテキストとして表示
        ImGui::Selectable(name.c_str());

        // ドラッグを開始したときの処理
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
        {
            // データとして文字列を送る
            ImGui::SetDragDropPayload("DND_MODEL", name.c_str(), name.size() + 1);

            // ドラッグ中にマウスカーソルに追従するテキスト
            ImGui::Text("Drop %s to Scene", name.c_str());

            ImGui::EndDragDropSource();
        }
    }

    ImGui::End();
}

}