#pragma once
#include <unordered_map>

// StageConfig はステージ別設定を保持する構造体
struct StageConfig
{
    float slopeAngle;
    int slopeLength;
};

class StageManager
{
public:
    void SetStage(int stageId);
    int GetStage() const { return currentStage_; }

    float GetSlopeAngle() const;
    int GetSlopeLength() const;

private:
    int currentStage_ = 1;
    // キャッシュされたステージ設定
    std::unordered_map<int, StageConfig> configs_;
};
