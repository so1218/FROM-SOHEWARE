#pragma once
#include <array>
#include <string>
#include <memory>

// 音声ID
enum class AudioID
{
    fanfare,

    count
};

// AudioIDとパスを紐付け
struct AudioDefinition
{
    AudioID id;
    const wchar_t* path;
};

class AudioHandle
{
public:
    static void Initialize();
    static int Get(AudioID id);  // AudioPlayerが管理する配列インデックスを返す

private:
    static constexpr std::array<AudioDefinition, static_cast<size_t>(AudioID::count)> audioDefinitions_ =
    { 
        {
            { AudioID::fanfare, L"Resources/audios/fanfare.wav" },
        }
    };

    static std::array<int, static_cast<size_t>(AudioID::count)> audioIndices_; // AudioPlayer内のID
    static bool initialized_;
};
