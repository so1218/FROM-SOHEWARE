#pragma once
#include <unordered_map>


class StageManager
{
public:
	// 最大ステージ数(4はタイトルシーン用)
	static constexpr int kMaxStages = 4;

	StageManager();

	// 全ステージの設定を最初に読み込む
	void Initialize();

	// ステージIDをセットする
	void SetCurrentStage(int stageId) { currentStage_ = stageId; }
	int GetCurrentStage() const { return currentStage_; }

private:

	int currentStage_ = 1;
};