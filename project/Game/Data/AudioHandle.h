#pragma once
#include <array>
#include <string>
#include <memory>

// 音声ID
enum class AudioID
{
    // BGM
    titleSceneBGM,
    playSceneBGM,
    clearSceneBGM,

    // SE
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
            // BGM
            { AudioID::titleSceneBGM, L"Assets/Audio/BGM/titleSceneBGM.mp3" },
            { AudioID::playSceneBGM, L"Assets/Audio/BGM/playSceneBGM.mp3" },
            { AudioID::clearSceneBGM, L"Assets/Audio/BGM/clearSceneBGM.mp3" },

            // SE
            { AudioID::exp, L"Assets/Audio/SE/exp.mp3" },
            { AudioID::levelUp, L"Assets/Audio/SE/levelUp.mp3" },
			{ AudioID::enemyHit, L"Assets/Audio/SE/enemyHit.mp3" },
			{ AudioID::playerHit, L"Assets/Audio/SE/playerHit.mp3" },
			{ AudioID::throwKnife, L"Assets/Audio/SE/throwKnife.mp3" },
			{ AudioID::throwAxe, L"Assets/Audio/SE/axe.mp3" },
			{ AudioID::clearSE, L"Assets/Audio/SE/clearSE.mp3" },
            { AudioID::dicision, L"Assets/Audio/SE/dicision.mp3" },
            { AudioID::cursolSE, L"Assets/Audio/SE/cursolSE.mp3" },
        }
    };

    static std::array<int, static_cast<size_t>(AudioID::count)> audioIndices_; // AudioPlayer内のID
    static bool initialized_;
};
