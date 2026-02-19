#include "AnimationManager.h"
#include "StringUtils.h"
#include <fstream>
#include <sstream>
#include <cassert>

AnimationManager* AnimationManager::GetInstance()
{
    static AnimationManager instance;
    return &instance;
}

void AnimationManager::LoadFromCSV(const std::string& csvPath)
{
    std::ifstream file(csvPath);
    if (!file.is_open())
    {
        assert(false && "AnimationList.csv not found.");
        return;
    }

    // CSV上のパスの基準ディレクトリ
    const std::string kDirectoryPath = "Assets/Models/";
    std::string line;

    // ヘッダー行をスキップするので一度getlineする
    std::getline(file, line);

    // CSV読み込みループ
    while (std::getline(file, line))
    {
        // 空行やコメントスキップ
        if (line.empty()) continue;
        size_t firstChar = line.find_first_not_of(" \t");
        if (firstChar == std::string::npos) continue;
        if (line[firstChar] == '#' || (line[firstChar] == '/' && line[firstChar + 1] == '/'))
        {
            continue;
        }

        std::istringstream stream(line);
        std::string name, path;

        // カンマ区切りで取得
        if (std::getline(stream, name, ',') && std::getline(stream, path))
        {
            // 空白除去
            name = StringUtils::Trim(name);
            path = StringUtils::Trim(path);

            if (!name.empty() && !path.empty())
            {
                // Load関数を呼び出す
                std::string fullPath = kDirectoryPath + path;
                Load(name, fullPath);
            }
        }
    }
}

void AnimationManager::Load(const std::string& name, const std::string& path)
{
    // 重複チェック
    if (animations_.find(name) != animations_.end())
    {
        return;
    }

    // 実データのロード
    animations_[name] = LoadAnimationFile(path.c_str());
}

const Animation* AnimationManager::Get(const std::string& name)
{
    auto it = animations_.find(name);
    if (it == animations_.end())
    {
        return nullptr;
    }
    return &it->second;
}