#pragma once

namespace FS::DevError
{

// 開発中に直すべき間違い (キーの綴り違い、見つからないアセットなど) を知らせる。
// ログに残し、開発用の構成 (Debug・Development) では画面の隅の一覧にも出す。ゲームは止めない。
// 毎フレーム呼ばれても困らないように、同じ内容は回数だけ数える。
// location には間違いを書いた場所を渡す。アセットを探す関数は、呼び出し元の場所を引数で受け取ってここに渡す
void Report(const std::string& message, std::source_location location = std::source_location::current());

// 報告されたエラーの一覧を ImGui で表示する。1 件もなければ何も出さない。毎フレーム 1 回呼ぶ
void DrawGui();

// これがある間に Report されたものには、呼び出し元の代わりに context を「場所」として書く。
// JSON から読んだ名前を使うときに置いて、どのファイルのどの項目が間違っているかを出す
class ScopedContext
{
public:
    explicit ScopedContext(std::string context);
    ~ScopedContext();

    ScopedContext(const ScopedContext&) = delete;
    ScopedContext& operator=(const ScopedContext&) = delete;
};

}
