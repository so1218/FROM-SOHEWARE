#pragma once

namespace FS
{

class AudioManager
{
public:
    // フォルダ内の音をすべて AudioPlayer に読み込む。名前は拡張子を除いたファイル名
    // Engine/Assets/Audio と Assets/Audio の音をすべて読む
    static void Initialize();

private:
    static bool initialized_;
};

}
