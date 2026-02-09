#include "AudioHandle.h"
#include "AudioPlayer.h"
#include "StringUtils.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include <iostream>

using namespace FromEngine;

std::unordered_map<std::string, int> AudioHandle::audioMap_;
bool AudioHandle::initialized_ = false;

void AudioHandle::Initialize()
{
    if (initialized_) return;

    const std::string csvPath = "Assets/Data/AudioList.csv";
    std::ifstream file(csvPath);

    // ファイルがない場合
    if (!file.is_open())
    {
        assert(false && "AudioList.csvが見つからない");
        return;
    }

    std::string line;
    std::getline(file, line); // 1行目をスキップ

    while (std::getline(file, line)) {
        std::istringstream stream(line);
        std::string name, path;

        // カンマ区切りで読み込み
        if (std::getline(stream, name, ',') && std::getline(stream, path, ',')) 
        {
            AudioPlayer::GetInstance().Load(name, StringUtils::ConvertString(path));
        }
    }

    initialized_ = true;
}

int AudioHandle::Get(const std::string& name)
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