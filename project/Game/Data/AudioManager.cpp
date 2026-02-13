#include "AudioManager.h"
#include "AudioPlayer.h"
#include "StringUtils.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include <iostream>

using namespace FromEngine;

std::unordered_map<std::string, int> AudioManager::audioMap_;
bool AudioManager::initialized_ = false;

void AudioManager::Initialize()
{
    if (initialized_) return;

    const std::string csvPath = "Assets/Data/AudioList.csv";
    std::ifstream file(csvPath);

    if (!file.is_open()) 
    {
        assert(false && "AudioList.csv not found.");
        return;
    }

    // オーディオのルートフォルダ
    const std::string kAudioRootPath = "Assets/Audio/";

    std::string line;
    std::getline(file, line);

    while (std::getline(file, line))
    {
        if (line.empty()) continue;

        size_t firstChar = line.find_first_not_of(" \t");
        if (firstChar == std::string::npos) continue;
        if (line[firstChar] == '#' || (line[firstChar] == '/' && line[firstChar + 1] == '/')) {
            continue;
        }

        std::istringstream stream(line);
        std::string name, path;

        if (std::getline(stream, name, ',') && std::getline(stream, path))
        {
            name = StringUtils::Trim(name);
            path = StringUtils::Trim(path);

            if (!name.empty() && !path.empty())
            {
                // パスを結合
                std::string fullPath = kAudioRootPath + path;

                // 結合してからConvertStringに渡す
                AudioPlayer::GetInstance().Load(name, StringUtils::ConvertString(fullPath));
            }
        }
    }

    initialized_ = true;
}

int AudioManager::Get(const std::string& name)
{
    assert(initialized_);

    // 見つからない場合にエラーを出す
    auto it = audioMap_.find(name);
    if (it == audioMap_.end()) 
    {
        // 開発中に気づけるようにログを出す
        std::cout << "Warning: Audio ID '" << name << "' not found in CSV." << std::endl;
        return -1;
    }

    return it->second;
}