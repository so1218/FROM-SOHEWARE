#pragma once

namespace FS
{

class AudioManager
{
public:
    // フォルダ内の音をすべて AudioPlayer に読み込む。名前は拡張子を除いたファイル名
    static void Initialize(const std::filesystem::path& rootDirectory = "Assets/Audio");

private:
    static bool initialized_;
};

}
