#include "pch.h"
#include "AudioManager.h"
#include "AudioPlayer.h"
#include "AssetFiles.h"
#include "StringUtils.h"

namespace FS
{

bool AudioManager::initialized_ = false;

void AudioManager::Initialize()
{
    if (initialized_) return;

    std::unordered_set<std::string> loadedNames;
    for (const std::filesystem::path& path : CollectAssetFiles("Audio", { ".mp3", ".wav" }))
    {
        const std::string name = GetAssetName(path);
        if (!loadedNames.insert(name).second)
        {
            std::cerr << "[AudioManager] Duplicate audio name, skipped: " << path.generic_string() << std::endl;
            continue;
        }

        AudioPlayer::GetInstance().Load(name, StringUtils::ConvertString(path.generic_string()));
    }

    initialized_ = true;
}

}
