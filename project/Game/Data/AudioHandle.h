#pragma once
#include <array>
#include <string>
#include <memory>

// 音声ID
enum class AudioID
{
    fanfare,
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
            { AudioID::fanfare, L"Resources/audios/fanfare.wav" },

            { AudioID::title, L"Resources/audios/title.mp3" },
            { AudioID::playScene, L"Resources/audios/playScene.mp3" },
            { AudioID::clear, L"Resources/audios/clear.mp3" },
            { AudioID::exp, L"Resources/audios/exp.mp3" },
            { AudioID::levelUp, L"Resources/audios/levelUp.mp3" },
			{ AudioID::enemyHit, L"Resources/audios/enemyHit.mp3" },
			{ AudioID::playerHit, L"Resources/audios/playerHit.mp3" },
			{ AudioID::throwKnife, L"Resources/audios/throwKnife.mp3" },
			{ AudioID::throwAxe, L"Resources/audios/axe.mp3" },
			{ AudioID::clearSE, L"Resources/audios/clearSE.mp3" },
            { AudioID::dicision, L"Resources/audios/dicision.mp3" },
            { AudioID::cursolSE, L"Resources/audios/cursolSE.mp3" },
        }
    };

    static std::array<int, static_cast<size_t>(AudioID::count)> audioIndices_; // AudioPlayer内のID
    static bool initialized_;
};
