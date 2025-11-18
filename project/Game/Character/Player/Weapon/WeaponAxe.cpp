#include "WeaponAxe.h"
#include "GlobalVariables.h"
#include "ImGuiManager.h"
#include "CollisionManager.h"
#include "Player.h"
#include "AxeProjectile.h" 

WeaponAxe::WeaponAxe(Engine* engine, Player* player, Camera* camera)
    : Weapon(engine, player), camera_(camera)
{
    // ベース値を初期化
    damageBase_ = 40.0f;
    cooldownBase_ = 3.0f;
    projectileCountBase_ = 1;
    level_ = 1;

    Initialize();
}

void WeaponAxe::Initialize()
{
    auto* gv = GlobalVariables::GetInstance();
    gv->CreateGroup(GetGlobalVariableGroupName());
    gv->LoadFiles();

    gv->AddItem(GetGlobalVariableGroupName(), "DamageBase", damageBase_);
    gv->AddItem(GetGlobalVariableGroupName(), "CooldownBase", cooldownBase_);
    gv->AddItem(GetGlobalVariableGroupName(), "Projectile CountBase", static_cast<float>(projectileCountBase_));
    gv->AddItem(GetGlobalVariableGroupName(), "Level", static_cast<float>(level_));
    gv->AddItem(GetGlobalVariableGroupName(), "Projectile InitialSpeedY", projectileInitialSpeedY_);
    gv->AddItem(GetGlobalVariableGroupName(), "Projectile Lifetime", projectileLifetime_);
    gv->AddItem(GetGlobalVariableGroupName(), "CollisionSize", collisionSize_);

    ApplyGlobalVariables();
}

void WeaponAxe::ApplyGlobalVariables()
{
    auto* gv = GlobalVariables::GetInstance();
    damageBase_ = gv->GetFloatValue(GetGlobalVariableGroupName(), "DamageBase");
    cooldownBase_ = gv->GetFloatValue(GetGlobalVariableGroupName(), "CooldownBase");
    projectileCountBase_ = static_cast<int>(gv->GetFloatValue(GetGlobalVariableGroupName(), "Projectile CountBase"));
    level_ = static_cast<int>(gv->GetFloatValue(GetGlobalVariableGroupName(), "Level"));
    projectileInitialSpeedY_ = gv->GetFloatValue(GetGlobalVariableGroupName(), "Projectile InitialSpeedY");
    projectileLifetime_ = gv->GetFloatValue(GetGlobalVariableGroupName(), "Projectile Lifetime");
    collisionSize_ = gv->GetVector3Value(GetGlobalVariableGroupName(), "CollisionSize");

    ApplyLevelEffects();
}

void WeaponAxe::Update(float deltaTime)
{
    cooldownTimer_ -= deltaTime;
    if (cooldownTimer_ <= 0.0f)
    {
        cooldownTimer_ = cooldown_;
        Fire();
    }

    for (auto& projectile : projectiles_)
    {
        projectile->Update();
    }

    std::erase_if(projectiles_, [](const std::unique_ptr<AxeProjectile>& p)
        {
            return p->IsDead();
        });
}

void WeaponAxe::Draw()
{
    for (auto& projectile : projectiles_)
    {
        projectile->Draw();
    }
}

void WeaponAxe::DebugDraw()
{
    ImGui::Begin("武器：斧");
    ImGui::Separator();

    bool changed = false;
    bool levelChanged = false;

    if (ImGui::DragFloat("ダメージ(Base)", &damageBase_, 0.1f, 0.0f, 0.0f))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "DamageBase", damageBase_);
        changed = true;
    }

    if (ImGui::DragFloat("クールダウン(Base)", &cooldownBase_, 0.01f, 0.0f, 0.0f))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "CooldownBase", cooldownBase_);
        changed = true;
    }

    if (ImGui::DragInt("発射数(Base)", &projectileCountBase_, 1, 1, 10))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "Projectile CountBase", static_cast<float>(projectileCountBase_));
        changed = true;
    }

    if (ImGui::DragFloat("Y初速", &projectileInitialSpeedY_, 0.1f, 0.0f, 0.0f))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "Projectile InitialSpeedY", projectileInitialSpeedY_);
        changed = true;
    }

    if (ImGui::DragFloat("弾の寿命", &projectileLifetime_, 0.1f, 0.0f, 0.0f))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "Projectile Lifetime", projectileLifetime_);
        changed = true;
    }

    if (ImGui::DragInt("レベル", &level_, 1, 1, 99))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "Level", static_cast<float>(level_));
        levelChanged = true;
    }

    if (ImGui::DragFloat3("弾の当たり判定サイズ", &collisionSize_.x, 0.01f, 0.01f, 10.0f))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "CollisionSize", collisionSize_);
        changed = true;
    }


    if (changed)
    {
        ApplyGlobalVariables();
    }
    else if (levelChanged)
    {
        ApplyLevelEffects();
    }

    ImGui::End();
}

void WeaponAxe::Fire()
{
    Vector3 playerPos = player_->GetWorldPosition();

    for (int i = 0; i < projectileCount_; ++i)
    {
        float initialSpeedX = Math::RandomFloat(-4.0f, 4.0f);
        float initialSpeedZ = Math::RandomFloat(-3.0f, 3.0f);
        Vector3 initialVelocity = { initialSpeedX, projectileInitialSpeedY_, initialSpeedZ };

        float initialYaw = atan2(initialVelocity.x, initialVelocity.z);

        auto newProjectile = std::make_unique<AxeProjectile>(engine_, camera_, playerPos, initialVelocity, initialYaw);

        newProjectile->SetDamage(damage_);
        newProjectile->SetLifetime(projectileLifetime_);

        newProjectile->SetSize(currentCollisionSize_);

        projectiles_.push_back(std::move(newProjectile));
    }
}

void WeaponAxe::LevelUp()
{
    level_++;

    // ステータスを再計算
    ApplyLevelEffects();
}

void WeaponAxe::ApplyLevelEffects()
{
    // ベース値(レベル1)をセット
    damage_ = damageBase_;
    cooldown_ = cooldownBase_;
    projectileCount_ = projectileCountBase_;
    Vector3 currentSize = collisionSize_;

    if (level_ >= 2)  projectileCount_ += 1;        
    if (level_ >= 3)  damage_ *= 1.2f;              
    if (level_ >= 4)  projectileCount_ += 1;        
    if (level_ >= 5)  currentSize *= 1.2f;          

    if (level_ >= 6)  projectileCount_ += 1;        
    if (level_ >= 7)  damage_ *= 1.2f;              
    if (level_ >= 8)  cooldown_ *= 0.9f;            
    if (level_ >= 9)  projectileCount_ += 1;        
    if (level_ >= 10) 
    {
        damage_ *= 1.5f;
        currentSize *= 1.3f;
    }

    if (level_ >= 11) cooldown_ *= 0.9f;           
    if (level_ >= 12) projectileCount_ += 1;       
    if (level_ >= 13) currentSize *= 1.2f;         
    if (level_ >= 14) damage_ *= 1.5f;             
    if (level_ >= 15)
    {
        projectileCount_ += 2;  
        cooldown_ *= 0.8f;      
        damage_ *= 1.5f;        
        currentSize *= 1.5f;    
    }

    currentCollisionSize_ = currentSize;
}

void WeaponAxe::AddCollidersToManager(CollisionManager* manager)
{
    for (auto& projectile : projectiles_)
    {
        if (projectile && !projectile->IsDead())
        {
            manager->AddCollider(projectile.get());
        }
    }
}