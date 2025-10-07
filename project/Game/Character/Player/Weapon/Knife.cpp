#include "Knife.h"
#include "TimeManager.h"
#include "BulletManager.h"
#include "BulletFactory.h"

Knife::Knife() 
{
    SetLevel(1); 
}

void Knife::Update(Player* player)
{
    cooldownTimer_ -= TimeManager::GetInstance()->GetDeltaTime(); // 自作エンジンでのフレーム時間取得

    if (cooldownTimer_ <= 0.0f) 
    {
        Shoot(player);
        cooldownTimer_ = cooldownMax_;
    }
}

void Knife::Shoot(Player* player)
{
    Vector3 spawnPos = player->GetWorldPosition();
    Vector3 direction = player->GetMoveDirection(); // プレイヤーの向いてる方向

    // レベルに応じて発射本数・角度を変える
    int numKnives = 1 + (level_ - 1); // 例：Lv1=1発, Lv2=2発, Lv3=3発...
    float spreadAngle = 15.0f; // 弾の拡がり角度（度）

    for (int i = 0; i < numKnives; ++i) 
    {
        float angleOffset = ((i - (numKnives - 1) / 2.0f) * spreadAngle);

        // 回転させた方向ベクトルを作る（Y軸周りの回転）
        float radians = ToRadians(angleOffset); // 角度をラジアンに変換
        Quaternion rot = Quaternion::FromAxisAngle({ 0.0f, 1.0f, 0.0f }, radians);
        Vector3 dirRotated = rot.RotateVector(direction);

        // 弾を生成
        BulletManager::GetInstance()->SpawnBullet(BulletType::Knife, spawnPos, dirRotated, level_);
    }
}

void Knife::SetLevel(int level)
{
    level_ = std::clamp(level, 1, 5); // Lv1～5に制限

    // レベルに応じてクールダウンや性能を変える（例）
    cooldownMax_ = 1.0f - (level_ - 1) * 0.15f; // レベルごとに速く
}