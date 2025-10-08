#include "StageManager.h"

void StageManager::SetStage(int stageId)
{
    currentStage_ = stageId;
    // もし configs_ に stageId がないなら読み込む／初期化する
    // GlobalVariables から取得したり
}

float StageManager::GetSlopeAngle() const
{
    auto it = configs_.find(currentStage_);
    if (it != configs_.end()) return it->second.slopeAngle;
    // デフォルト値を返す
    return 0.0f;
}

int StageManager::GetSlopeLength() const
{
    auto it = configs_.find(currentStage_);
    if (it != configs_.end()) return it->second.slopeLength;
    return 10;
}