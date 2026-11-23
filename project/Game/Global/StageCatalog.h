#pragma once
#include <string>
#include <vector>

// ステージ
struct StageDefinition
{
    std::string settingsName; 
    std::string displayName;   

    std::string GetGroupName(const std::string& name) const { return "Stages/" + settingsName + "/" + name; }
};

// ステージの一覧と、タイトルで選んでいるステージ
namespace StageCatalog
{

const std::vector<StageDefinition>& GetStages();
int GetSelectedIndex();
const StageDefinition& GetSelectedStage();

// 端を越えたら反対の端へ回る
void Select(int index);

// タイトルでステージを切り替えてシーンを作り直した直後か
bool TakeJustSwitched();

}
