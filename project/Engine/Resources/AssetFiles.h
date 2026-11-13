#pragma once

namespace FS
{

// アセットを置くフォルダ
inline constexpr std::string_view kEngineAssetDirectory = "Engine/Assets";
inline constexpr std::string_view kGameAssetDirectory = "Assets";

// Engine/Assets/<subDirectory> と Assets/<subDirectory> から、指定した拡張子のファイルをすべて集める
std::vector<std::filesystem::path> CollectAssetFiles(
    std::string_view subDirectory, std::initializer_list<std::string_view> extensions);

// アセットの名前は、拡張子を除いたファイル名
std::string GetAssetName(const std::filesystem::path& path);

// メッシュを持たない glTF はアニメーションだけのファイル
bool ContainsMesh(const std::filesystem::path& path);

}
