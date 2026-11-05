#pragma once

namespace FS
{

// フォルダの中から、指定した拡張子のファイルをすべて集める。拡張子は小文字で "." 付きで渡す
std::vector<std::filesystem::path> CollectAssetFiles(
    const std::filesystem::path& rootDirectory, std::initializer_list<std::string_view> extensions);

// アセットの名前は、拡張子を除いたファイル名
std::string GetAssetName(const std::filesystem::path& path);

// メッシュを持たない glTF はアニメーションだけのファイル
bool ContainsMesh(const std::filesystem::path& path);

}
