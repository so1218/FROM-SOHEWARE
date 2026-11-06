#pragma once

namespace FS
{

// 空や天候などの設定は環境設定ウィンドウ1つにタブで並べる
inline bool BeginEnvironmentTab(const char* label)
{
    if (ImGui::Begin("環境設定") && ImGui::BeginTabBar("EnvironmentTabs"))
    {
        if (ImGui::BeginTabItem(label))
        {
            return true;
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
    return false;
}

inline void EndEnvironmentTab()
{
    ImGui::EndTabItem();
    ImGui::EndTabBar();
    ImGui::End();
}

}
