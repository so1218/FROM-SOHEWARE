#include "AnimationHandle.h"
#include "AnimationManager.h"
#include "StringUtils.h"
#include <fstream>
#include <sstream>
#include <cassert>

bool AnimationHandle::initialized_ = false;

void AnimationHandle::Initialize()
{
    if (initialized_) return;

    const std::string csvPath = "Assets/Data/AnimationList.csv";
    std::ifstream file(csvPath);

    if (!file.is_open()) 
    {
        assert(false && "AnimationList.csv not found.");
        return;
    }

    // ルートパスを指定
    const std::string kDirectoryPath = "Assets/Models/";

    std::string line;
    std::getline(file, line);

    while (std::getline(file, line))
    {
        if (line.empty()) continue;

        size_t firstChar = line.find_first_not_of(" \t");
        if (firstChar == std::string::npos) continue;
        if (line[firstChar] == '#' || (line[firstChar] == '/' && line[firstChar + 1] == '/'))
        {
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
                std::string fullPath = kDirectoryPath + path;

                AnimationManager::GetInstance().Load(name, fullPath);
            }
        }
    }

    initialized_ = true;
}