#pragma once

namespace FE
{

class AudioManager
{
public:
    // CSVから全アセットをロード
    static void Initialize();

    // 文字列IDからAudioPlayerのインデックスを取得
    static int Get(const std::string& name);

private:
    // 文字列IDとAudioPlayerのインデックスを紐付け
    static std::unordered_map<std::string, int> audioMap_;
    static bool initialized_;
};

}