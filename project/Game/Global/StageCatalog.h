#pragma once
#include <string>
#include <vector>
#include "Vector4.h"

// ステージ
struct StageDefinition
{
    std::string settingsName; 
    std::string displayName;   

    std::string GetGroupName(const std::string& name) const { return "Stages/" + settingsName + "/" + name; }
};

// 画面に出すステージの名前と色
struct StageAppearance
{
    std::string name;
    FS::Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
};

// ステージの一覧と、タイトルで選んでいるステージ
namespace StageCatalog
{

const std::vector<StageDefinition>& GetStages();
int GetSelectedIndex();
const StageDefinition& GetSelectedStage();

// ステージの並びと同じ順
StageAppearance& GetAppearance(int index);
const StageAppearance& GetSelectedAppearance();

// 端を越えたら反対の端へ回る
void Select(int index);

// タイトルでステージを切り替えてシーンを作り直した直後か
bool TakeJustSwitched();

}
