#pragma once
#include <array>
#include <string>
#include <memory>

// 音声ID
enum class AudioID
{
    title,
    playScene,
    clear,

    exp,
    levelUp,
    enemyHit,
    playerHit,
    throwKnife,
    throwAxe,
    clearSE,
    dicision,
    cursolSE,

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
            { AudioID::title, L"Assets/audios/title.mp3" },
            { AudioID::playScene, L"Assets/audios/playScene.mp3" },
            { AudioID::clear, L"Assets/audios/clear.mp3" },
            { AudioID::exp, L"Assets/audios/exp.mp3" },
            { AudioID::levelUp, L"Assets/audios/levelUp.mp3" },
			{ AudioID::enemyHit, L"Assets/audios/enemyHit.mp3" },
			{ AudioID::playerHit, L"Assets/audios/playerHit.mp3" },
			{ AudioID::throwKnife, L"Assets/audios/throwKnife.mp3" },
			{ AudioID::throwAxe, L"Assets/audios/axe.mp3" },
			{ AudioID::clearSE, L"Assets/audios/clearSE.mp3" },
            { AudioID::dicision, L"Assets/audios/dicision.mp3" },
            { AudioID::cursolSE, L"Assets/audios/cursolSE.mp3" },
        }
    };

    static std::array<int, static_cast<size_t>(AudioID::count)> audioIndices_; // AudioPlayer内のID
    static bool initialized_;
};
