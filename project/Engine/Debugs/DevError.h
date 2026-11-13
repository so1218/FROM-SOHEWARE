#pragma once

namespace FS::DevError
{

// 開発中に直すべき間違いを知らせる
void Report(const std::string& message, std::source_location location = std::source_location::current());

// 報告されたエラーの一覧を ImGui で表示
void DrawGui();

// これがある間に Report されたものには、呼び出し元の代わりに context を場所として書く
class ScopedContext
{
public:
    explicit ScopedContext(std::string context);
    ~ScopedContext();

    ScopedContext(const ScopedContext&) = delete;
    ScopedContext& operator=(const ScopedContext&) = delete;
};

}
