#include "pch.h"
#include "AssetFiles.h"

namespace FS
{

namespace
{

std::string ToLower(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(),
        [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return text;
}

}

std::vector<std::filesystem::path> CollectAssetFiles(
    const std::filesystem::path& rootDirectory, std::initializer_list<std::string_view> extensions)
{
    std::vector<std::filesystem::path> files;
    if (!std::filesystem::exists(rootDirectory))
    {
        std::cerr << "[AssetFiles] Directory not found: " << rootDirectory.generic_string() << std::endl;
        return files;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(rootDirectory))
    {
        if (!entry.is_regular_file()) continue;

        const std::string extension = ToLower(entry.path().extension().string());
        if (std::find(extensions.begin(), extensions.end(), extension) != extensions.end())
        {
            files.push_back(entry.path());
        }
    }

    // 名前が重複したときにどちらが読まれるかを、毎回同じにする
    std::sort(files.begin(), files.end());
    return files;
}

std::string GetAssetName(const std::filesystem::path& path)
{
    return path.stem().string();
}

bool ContainsMesh(const std::filesystem::path& path)
{
    if (ToLower(path.extension().string()) != ".gltf")
    {
        return true;
    }

    std::ifstream file(path);
    const nlohmann::json gltf = nlohmann::json::parse(file, nullptr, false);

    // 読めないファイルはモデルとして扱い、ModelLoader にエラーを出させる
    if (gltf.is_discarded())
    {
        return true;
    }
    return gltf.contains("meshes") && !gltf["meshes"].empty();
}

}
