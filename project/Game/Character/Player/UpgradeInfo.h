#pragma once
#include <string>

enum class UpgradeType
{
	newWeapon,
	UpgradeWeapon,
	passiveUp,
	heal
};

struct UpgradeInfo
{
	int id; // 識別ID
	UpgradeType type; // 種類
	std::string name; // 表示名
	std::string description; // 説明文
	int weaponId; // 武器の場合のID
	float value; // ステータスアップなどの数値
	uint32_t textureID;
};