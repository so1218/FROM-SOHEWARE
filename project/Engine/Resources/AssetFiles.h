#pragma once

namespace FS
{

// アセットを置くフォルダ。エンジンが使うものは Engine/Assets、ゲームのものは Assets に置き、中の構成はそろえる
inline constexpr std::string_view kEngineAssetDirectory = "Engine/Assets";
inline constexpr std::string_view kGameAssetDirectory = "Assets";

// Engine/Assets/<subDirectory> と Assets/<subDirectory> から、指定した拡張子のファイルをすべて集める。
// 拡張子は小文字で "." 付きで渡す。エンジンのものが先に並ぶので、同じ名前があればエンジンのものが読まれる
std::vector<std::filesystem::path> CollectAssetFiles(
    std::string_view subDirectory, std::initializer_list<std::string_view> extensions);

// アセットの名前は、拡張子を除いたファイル名
std::string GetAssetName(const std::filesystem::path& path);

// メッシュを持たない glTF はアニメーションだけのファイル
bool ContainsMesh(const std::filesystem::path& path);

}
