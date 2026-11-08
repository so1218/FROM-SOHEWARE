#include "pch.h"
#include "DevError.h"
#include "Logger.h"

namespace FS::DevError
{

namespace
{

struct Entry
{
    std::string message;
    std::string place;
    int count = 0;
};

// 画面に出している一覧。クリアすると空になる
std::vector<Entry> entries;
// これまでに報告されたもの。ログには初めてのときだけ書く
std::unordered_set<std::string> reportedKeys;
// ScopedContext で積まれた場所
std::vector<std::string> contexts;

std::string MakePlace(const std::source_location& location)
{
    if (!contexts.empty())
    {
        return contexts.back();
    }

    std::string_view path = location.file_name();

    // std::make_unique などを通して呼ぶと、標準ライブラリの中が呼び出し元になってしまう
    const bool isProjectFile =
        path.find("\\Engine\\") != std::string_view::npos || path.find("/Engine/") != std::string_view::npos ||
        path.find("\\Game\\") != std::string_view::npos || path.find("/Game/") != std::string_view::npos;
    if (!isProjectFile)
    {
        return "行は分からない (std::make_unique などを通して呼んでいる)。上の名前でコードを検索すると見つかる";
    }

    const size_t slash = path.find_last_of("/\\");
    if (slash != std::string_view::npos)
    {
        path = path.substr(slash + 1);
    }
    return std::format("{} の {} 行目", path, location.line());
}

}

void Report(const std::string& message, std::source_location location)
{
    const std::string place = MakePlace(location);
    const std::string key = message + "\n" + place;

    for (Entry& entry : entries)
    {
        if (entry.message == message && entry.place == place)
        {
            ++entry.count;
            return;
        }
    }

    entries.push_back({ message, place, 1 });

    // クリアしたあとにまた起きたものは、一覧に戻すだけにする
    if (!reportedKeys.insert(key).second) return;

    LOG_ERROR("{} (場所: {})", message, place);
}

void DrawGui()
{
#if defined(ENABLE_IMGUI) && defined(ENABLE_DEV_TOOLS)
    if (entries.empty()) return;

    const ImVec4 titleColor = { 0.6f, 0.1f, 0.1f, 1.0f };
    ImGui::PushStyleColor(ImGuiCol_TitleBg, titleColor);
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, titleColor);
    ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, titleColor);

    // 件数が変わってもウィンドウの位置と大きさを保つよう、### の後ろを ID にする
    const std::string title = std::format("開発用のエラー ({} 件)###DevError", entries.size());
    if (ImGui::Begin(title.c_str()))
    {
        if (ImGui::Button("クリア"))
        {
            entries.clear();
        }

        for (const Entry& entry : entries)
        {
            ImGui::Separator();
            ImGui::PushTextWrapPos(0.0f);
            ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.4f, 1.0f), "%s", entry.message.c_str());
            ImGui::TextDisabled("場所: %s (%d 回)", entry.place.c_str(), entry.count);
            ImGui::PopTextWrapPos();
        }
    }
    ImGui::End();

    ImGui::PopStyleColor(3);
#endif
}

ScopedContext::ScopedContext(std::string context)
{
    contexts.push_back(std::move(context));
}

ScopedContext::~ScopedContext()
{
    contexts.pop_back();
}

}
