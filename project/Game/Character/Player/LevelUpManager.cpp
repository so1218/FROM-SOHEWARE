#include "LevelUpManager.h"
#include "TextureHandle.h"
#include <algorithm>

LevelUpManager::LevelUpManager()
{
	uint32_t texKnife = TextureHandle::Get(TextureID::knifeLevelUp);
	uint32_t texAxe = TextureHandle::Get(TextureID::axeLevelUp);
	uint32_t texMaxHpUp = TextureHandle::Get(TextureID::hpUp);
	uint32_t texSpeedUp = TextureHandle::Get(TextureID::speedUp);
	uint32_t texHpHeal = TextureHandle::Get(TextureID::heal);

	// データを登録
	allNewWeapons_.push_back({ 100,UpgradeType::newWeapon,"Knife","Throws knives forward", (int)WeaponType::Knife,0, texKnife });
	allNewWeapons_.push_back({ 101,UpgradeType::newWeapon,"Axe","Throws axe", (int)WeaponType::Axe,0, texAxe });

	allPassives_.push_back({ 200,UpgradeType::passiveUp,"MaxHp Up","Increases Max HP", 0,20.0f, texMaxHpUp });
	allPassives_.push_back({ 201,UpgradeType::passiveUp,"Speed Up","Increases Speed", 0,0.05f, texSpeedUp });
	allPassives_.push_back({ 202,UpgradeType::heal,"Chicken","Recover HP", 0,30.0f, texHpHeal });
}

std::vector<UpgradeInfo> LevelUpManager::PickUpgrades(Player* player)
{
	std::vector<UpgradeInfo> candidates;

	// 新規武器
	// まだ持っていない武器を候補に入れる
	for (const auto& info : allNewWeapons_)
	{
		// プレイヤーがその武器を持っていない場合のみ候補へ
		if (!player->HasWeapon(static_cast<WeaponType>(info.weaponId)))
		{
			candidates.push_back(info);
		}
	}

	// 武器強化
	// 持っている武器で、まだMaxレベルじゃないものを候補に入れる
	const auto& currentWeapons = player->GetWeapons();
	for (const auto& weapon : currentWeapons)
	{
		if (!weapon->IsMaxLevel())
		{
			// 武器に対応した強化データを生成して追加
			UpgradeInfo info;
			info.type = UpgradeType::UpgradeWeapon;
			info.weaponId = (int)weapon->GetType();
			info.name = "Upgrade Weapon";
			info.description = "Level Up";
			bool found = false;
			for (const auto& weaponInfo : allNewWeapons_)
			{
				if (weaponInfo.weaponId == info.weaponId)
				{
					info.textureHandle = weaponInfo.textureHandle; 
					found = true;
					break;
				}
			}

			// 見つからなかった場合
			if (!found)
			{
				info.textureHandle = TextureHandle::Get(TextureID::white1x1);
			}

			candidates.push_back(info);
		}
	}

	// パッシブ、回復を候補に
	for (const auto& info : allPassives_)
	{
		candidates.push_back(info);
	}

	// シャッフルして先頭3つを返す
	// 候補なし
	if (candidates.empty())
	{
		return {};
	}

	int n = static_cast<int>(candidates.size());
	for (int i = n - 1; i > 0; --i)
	{
		// 0からiまでのランダムなインデックスを取得
		int j = rand() % (i + 1);

		// 要素を入れ替える
		std::swap(candidates[i], candidates[j]);
	}

	// 先頭から最大3つを取り出して返す
	std::vector<UpgradeInfo> result;
	int pickCount = 3;
	for (int i = 0; i < pickCount && i < candidates.size(); ++i)
	{
		result.push_back(candidates[i]);
	}

	return result;
}