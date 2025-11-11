#include "WeaponKnife.h"
#include "TimeManager.h"
#include "Player.h"
#include "GlobalVariables.h"
#include "imGuiManager.h"

WeaponKnife::WeaponKnife(Engine* engine, Player* player, Camera* camera)
    : Weapon(engine, player), camera_(camera)
{
    // ナイフの初期設定
    damageBase_ = 20.0f;
    cooldownBase_ = 1.5f;
    projectileCountBase_ = 1;
    level_ = 1;

    Initialize();
}

void WeaponKnife::Initialize()
{
    auto* gv = GlobalVariables::GetInstance();

    // グローバル変数グループを登録して読み込み
    gv->CreateGroup(GetGlobalVariableGroupName());
    gv->LoadFiles();

    // パラメータを登録
    gv->AddItem(GetGlobalVariableGroupName(), "DamageBase", damageBase_);
    gv->AddItem(GetGlobalVariableGroupName(), "CooldownBase", cooldownBase_);
    gv->AddItem(GetGlobalVariableGroupName(), "Projectile CountBase", static_cast<float>(projectileCountBase_));
    gv->AddItem(GetGlobalVariableGroupName(), "Level", static_cast<float>(level_));
    gv->AddItem(GetGlobalVariableGroupName(), "Projectile Speed", projectileSpeed_);
    gv->AddItem(GetGlobalVariableGroupName(), "Projectile Lifetime", projectileLifetime_);
    gv->AddItem(GetGlobalVariableGroupName(), "CollisionSize", collisionSize_);
    gv->AddItem(GetGlobalVariableGroupName(), "Time Between Projectiles", timeBetweenProjectiles_);

    ApplyGlobalVariables();
}

void WeaponKnife::ApplyGlobalVariables()
{
    auto* gv = GlobalVariables::GetInstance();

    damageBase_ = gv->GetFloatValue(GetGlobalVariableGroupName(), "DamageBase");
    cooldownBase_ = gv->GetFloatValue(GetGlobalVariableGroupName(), "CooldownBase");
    projectileCountBase_ = static_cast<int>(gv->GetFloatValue(GetGlobalVariableGroupName(), "Projectile CountBase"));
    level_ = static_cast<int>(gv->GetFloatValue(GetGlobalVariableGroupName(), "Level"));
    projectileSpeed_ = gv->GetFloatValue(GetGlobalVariableGroupName(), "Projectile Speed");
    projectileLifetime_ = gv->GetFloatValue(GetGlobalVariableGroupName(), "Projectile Lifetime");
    collisionSize_ = gv->GetVector3Value(GetGlobalVariableGroupName(), "CollisionSize");
    timeBetweenProjectiles_ = gv->GetFloatValue(GetGlobalVariableGroupName(), "Time Between Projectiles");

    ApplyLevelEffects();
}

void WeaponKnife::Update(float deltaTime)
{
    // クールダウン処理
    cooldownTimer_ -= deltaTime;

    if (projectilesToFire_ == 0 && cooldownTimer_ <= 0.0f)
    {
        // バースト(連射)開始
        projectilesToFire_ = projectileCount_;  // 発射する総数をセット
        burstTimer_ = 0.0f;                     // 1発目はすぐ発射
        cooldownTimer_ = cooldown_;             // 次のバーストのためのクールダウンをリセット
    }

    // バースト発射中の処理
    if (projectilesToFire_ > 0)
    {
        // 連射間隔タイマーを減らす
        burstTimer_ -= deltaTime;

        // 連射間隔タイマーが0以下になったら
        if (burstTimer_ <= 0.0f)
        {
            FireOneProjectile(); // 1発発射
            projectilesToFire_--; // 残り弾数を減らす
            burstTimer_ = timeBetweenProjectiles_; // 次の弾までの間隔をセット
        }
    }

    // 弾の更新
    for (auto& projectile : projectiles_)
    {
        projectile->Update(deltaTime);
    }

    // 寿命が尽きた弾を削除
    std::erase_if(projectiles_, [](const std::unique_ptr<KnifeProjectile>& p)
        {
            return p->IsDead();
        });
}

void WeaponKnife::Draw()
{
    // すべての弾を描画
    for (auto& projectile : projectiles_)
    {
        projectile->Draw();
    }
}

void WeaponKnife::DebugDraw()
{
    ImGui::Begin("武器：ナイフ");
    ImGui::Separator();
    ImGui::Text("パラメータ調整");

    bool changed = false;
    bool levelChanged = false;

    if (ImGui::DragFloat("ダメージ(Base)", &damageBase_, 0.1f, 0.0f, 0.0f))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "DamageBase", damageBase_);
        changed = true;
    }

    if (ImGui::DragFloat("クールダウン時間(Base)", &cooldownBase_, 0.01f, 0.0f, 0.0f))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "CooldownBase", cooldownBase_);
        changed = true;
    }

    if (ImGui::DragFloat("弾の速度(Base)", &projectileSpeed_, 0.1f, 0.0f, 0.0f))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "Projectile Speed", projectileSpeed_);
        changed = true;
    }

    if (ImGui::DragFloat("弾の寿命", &projectileLifetime_, 0.1f, 0.0f, 0.0f))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "Projectile Lifetime", projectileLifetime_);
        changed = true;
    }

    if (ImGui::DragInt("同時発射数", &projectileCountBase_, 1, 0, 0))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "Projectile CountBase", static_cast<float>(projectileCountBase_));
        changed = true;
    }
    if (ImGui::DragInt("レベル", &level_, 1, 0, 0)) 
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "Level", static_cast<float>(level_));
        levelChanged = true; 
    }

    if (ImGui::DragFloat("弾の連射間隔", &timeBetweenProjectiles_, 0.01f, 0.0f, 1.0f))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "Time Between Projectiles", timeBetweenProjectiles_);
    }

    if (ImGui::DragFloat3("弾の当たり判定サイズ", &collisionSize_.x, 0.01f, 0.01f, 10.0f))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "CollisionSize", collisionSize_);
    }

    if (changed)
    {
        ApplyGlobalVariables();
    }
    else if (levelChanged)
    {
        // レベルだけが変更された場合
        ApplyLevelEffects();
    }

    ImGui::End();
}

void WeaponKnife::FireOneProjectile()
{
    // プレイヤーの位置と向きを取得
    Vector3 playerPos = player_->GetWorldPosition();
    Vector3 playerDir = player_->GetLastMoveDirection();

    // 向きがゼロベクトルの場合は正面方向を使用
    if (playerDir.Length() < 0.001f)
    {
        playerDir = { 0.0f, 0.0f, 1.0f };
    }

    // 弾の生成と初期設定
    auto newProjectile = std::make_unique<KnifeProjectile>(engine_, camera_, playerPos, playerDir, collisionSize_);
    newProjectile->SetDamage(damage_);
    newProjectile->SetSpeed(projectileSpeed_);
    newProjectile->SetLifetime(projectileLifetime_);

    projectiles_.push_back(std::move(newProjectile));
}

void WeaponKnife::ApplyLevelEffects()
{
    // ベース値(レベル1)をセット
    damage_ = damageBase_;
    cooldown_ = cooldownBase_;
    projectileCount_ = projectileCountBase_;

    // 現在のレベルに応じて効果を上乗せ
    if (level_ >= 2) projectileCount_++;
    if (level_ >= 3) damage_ *= 1.5f;
    if (level_ >= 4) projectileCount_++;
    if (level_ >= 5) cooldown_ *= 0.8f;
}

void WeaponKnife::LevelUp()
{
    level_++;

    // GlobalVariables に現在のレベルを保存
    GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "Level", static_cast<float>(level_));

    // ステータスを再計算
    ApplyLevelEffects();
}

void WeaponKnife::AddCollidersToManager(CollisionManager* manager)
{
    // 自分が管理しているすべての弾をCollisionManager に登録する
    for (auto& projectile : projectiles_)
    {
        if (projectile && !projectile->IsDead())
        {
            // KnifeProjectile は Collider を継承しているのでそのまま渡せる
            manager->AddCollider(projectile.get());
        }
    }
}