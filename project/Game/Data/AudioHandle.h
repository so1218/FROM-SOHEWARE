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
            { AudioID::title, L"Assets/Audio/title.mp3" },
            { AudioID::playScene, L"Assets/Audio/playScene.mp3" },
            { AudioID::clear, L"Assets/Audio/clear.mp3" },
            { AudioID::exp, L"Assets/Audio/exp.mp3" },
            { AudioID::levelUp, L"Assets/Audio/levelUp.mp3" },
			{ AudioID::enemyHit, L"Assets/Audio/enemyHit.mp3" },
			{ AudioID::playerHit, L"Assets/Audio/playerHit.mp3" },
			{ AudioID::throwKnife, L"Assets/Audio/throwKnife.mp3" },
			{ AudioID::throwAxe, L"Assets/Audio/axe.mp3" },
			{ AudioID::clearSE, L"Assets/Audio/clearSE.mp3" },
            { AudioID::dicision, L"Assets/Audio/dicision.mp3" },
            { AudioID::cursolSE, L"Assets/Audio/cursolSE.mp3" },
        }
    };

    static std::array<int, static_cast<size_t>(AudioID::count)> audioIndices_; // AudioPlayer内のID
    static bool initialized_;
};
