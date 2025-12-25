#pragma once
#include "UpgradeInfo.h"
#include "Player.h"
#include <vector>


class LevelUpManager
{
public:
	LevelUpManager();

	// プレイヤーの状態から、ランダムな選択肢を返す
	std::vector<UpgradeInfo> PickUpgrades(Player* player);

private:
	// ゲーム内に存在する全ての新気噴データのリスト
	std::vector<UpgradeInfo> allNewWeapons_;
	// ゲーム内に存在する全てのパッシブ強化データのリスト
	std::vector<UpgradeInfo> allPassives_;
};