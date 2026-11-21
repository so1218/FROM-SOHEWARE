#pragma once
#include <string>
#include <vector>

// 遊べるステージ
// フィールド (地形・草・木・植物・遠景・置き物・蛍)、時間と天気、空、ライトとポストエフェクト、タイトルのカメラの設定は、
// ステージごとに GlobalVariables の Stages/<settingsName>/ に保存する。プレイヤーや敵、UI の設定はステージに関係なく共通
struct StageDefinition
{
    std::string settingsName;  // 設定を保存するフォルダの名前
    std::string displayName;   // タイトルに出す名前

    // このステージの設定のグループ名。"Ground" なら "Stages/<settingsName>/Ground"
    std::string GetGroupName(const std::string& name) const { return "Stages/" + settingsName + "/" + name; }
};

// ステージの一覧と、タイトルで選んでいるステージ。選んだステージはシーンを作り直しても残る
namespace StageCatalog
{

const std::vector<StageDefinition>& GetStages();
int GetSelectedIndex();
const StageDefinition& GetSelectedStage();

// 端を越えたら反対の端へ回る
void Select(int index);

// タイトルでステージを切り替えてシーンを作り直した直後か。一度読むと false に戻る
bool TakeJustSwitched();

}
