#include "WeaponKnife.h"
#include "TimeManager.h"
#include "Player.h"
#include "GlobalVariables.h"
#include "imGuiManager.h"

WeaponKnife::WeaponKnife(Engine* engine, Player* owner) : Weapon(engine, owner)
{
    // ナイフの初期設定
    damage_ = 20.0f;
    cooldown_ = 1.5f;
    projectileCount_ = 1;

    Initialize();
}

void WeaponKnife::Initialize()
{
    auto* gv = GlobalVariables::GetInstance();

    // グループを登録して値をロード
    gv->CreateGroup(GetGlobalVariableGroupName());
    gv->LoadFiles();

    // 登録
    gv->AddItem(GetGlobalVariableGroupName(), "Damage", damage_);
    gv->AddItem(GetGlobalVariableGroupName(), "Cooldown", cooldown_);
    gv->AddItem(GetGlobalVariableGroupName(), "Projectile Speed", projectileSpeed_);
    gv->AddItem(GetGlobalVariableGroupName(), "Projectile Lifetime", projectileLifetime_);
    gv->AddItem(GetGlobalVariableGroupName(), "Projectile Count", (float)projectileCount_);

    ApplyGlobalVariables();
}

void WeaponKnife::ApplyGlobalVariables()
{
    auto* gv = GlobalVariables::GetInstance();

    damage_ = gv->GetFloatValue(GetGlobalVariableGroupName(), "Damage");
    cooldown_ = gv->GetFloatValue(GetGlobalVariableGroupName(), "Cooldown");
    projectileSpeed_ = gv->GetFloatValue(GetGlobalVariableGroupName(), "Projectile Speed");
    projectileLifetime_ = gv->GetFloatValue(GetGlobalVariableGroupName(), "Projectile Lifetime");
    projectileCount_ = static_cast<int>(gv->GetFloatValue(GetGlobalVariableGroupName(), "Projectile Count"));
}

void WeaponKnife::Update(float deltaTime)
{
    // クールダウン処理
    cooldownTimer_ -= deltaTime;
    if (cooldownTimer_ <= 0.0f)
    {
        cooldownTimer_ = cooldown_; 
        Fire();                     
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
    // すべての弾（ナイフ）を描画
    for (auto& projectile : projectiles_)
    {
        projectile->Draw();
    }
}

void WeaponKnife::DebugDraw()
{
    ImGui::Begin("武器 - ナイフ");
    ImGui::Separator();
    ImGui::Text("パラメータ調整");

    bool changed = false;

    if (ImGui::DragFloat("ダメージ", &damage_, 0.1f, 0.0f, 0.0f))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "Damage", damage_);
        changed = true;
    }

    if (ImGui::DragFloat("クールダウン時間", &cooldown_, 0.01f, 0.0f, 0.0f))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "Cooldown", cooldown_);
        changed = true;
    }


    if (ImGui::DragFloat("弾の速度", &projectileSpeed_, 0.1f, 0.0f, 0.0f))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "Projectile Speed", projectileSpeed_);
        changed = true;
    }

    if (ImGui::DragFloat("弾の寿命", &projectileLifetime_, 0.1f, 0.0f, 0.0f))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "Projectile Lifetime", projectileLifetime_);
        changed = true;
    }

    if (ImGui::DragInt("同時発射数", &projectileCount_, 1, 0, 0))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "Projectile Count", (float)projectileCount_);
        changed = true;
    }

    if (changed)
    {
        ApplyGlobalVariables();
    }

    ImGui::End();
}

void WeaponKnife::Fire()
{
    // 弾の生成に必要な情報を取得
    Vector3 dir = owner_->GetLastMoveDirection(); // 発射方向
    Vector3 startPos = owner_->GetWorldPosition(); // 発射位置
    Camera* camera = owner_->GetCamera();          // カメラ参照

    // 複数発射（レベルアップによる増加に対応）
    for (int i = 0; i < projectileCount_; ++i)
    {
        auto projectile = std::make_unique<KnifeProjectile>(engine_, camera, startPos, dir);

        projectile->SetSpeed(projectileSpeed_);
        projectile->SetLifetime(projectileLifetime_);

        projectiles_.push_back(std::move(projectile));
    }
}

void WeaponKnife::LevelUp()
{
    // レベルアップ処理
    level_++;

    if (level_ == 2) { projectileCount_++; }   
    if (level_ == 3) { damage_ *= 1.5f; }      
    if (level_ == 4) { projectileCount_++; }   
    if (level_ == 5) { cooldown_ *= 0.8f; }    
}