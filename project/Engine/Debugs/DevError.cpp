#include "pch.h"
#include "DevError.h"
#include "Logger.h"
#include "StringUtils.h"

namespace FS::DevError
{

namespace
{

thread_local std::vector<std::string> contexts;

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
    const std::string text = std::format("{}\n\n場所: {}", message, MakePlace(location));

    // アセットの読み込みは別のスレッドから呼ばれることがある
    static std::mutex mutex;
    static std::unordered_set<std::string> reportedTexts;
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (!reportedTexts.insert(text).second) return;
    }

    LOG_ERROR("{}", text);

#ifdef ENABLE_DEV_TOOLS
    const std::wstring wideText = StringUtils::ConvertString(text);
    MessageBoxW(nullptr, wideText.c_str(), L"開発用のエラー", MB_OK | MB_ICONWARNING);
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
