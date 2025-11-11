#include "WeaponAxe.h"
#include "GlobalVariables.h"
#include "ImGuiManager.h"
#include "CollisionManager.h"
#include "Player.h"
#include "AxeProjectile.h" 

WeaponAxe::WeaponAxe(Engine* engine, Player* player, Camera* camera)
    : Weapon(engine, player), camera_(camera)
{
    damage_ = 40.0f;
    cooldown_ = 3.0f;
    projectileCount_ = 1;
    level_ = 2;

    Initialize();
}

void WeaponAxe::Initialize()
{
    auto* gv = GlobalVariables::GetInstance();
    gv->CreateGroup(GetGlobalVariableGroupName());
    gv->LoadFiles();

    gv->AddItem(GetGlobalVariableGroupName(), "Damage", damage_);
    gv->AddItem(GetGlobalVariableGroupName(), "Cooldown", cooldown_);
    gv->AddItem(GetGlobalVariableGroupName(), "Projectile InitialSpeedY", projectileInitialSpeedY_);
    gv->AddItem(GetGlobalVariableGroupName(), "Projectile Lifetime", projectileLifetime_);
    gv->AddItem(GetGlobalVariableGroupName(), "Projectile Count", static_cast<float>(projectileCount_));

    ApplyGlobalVariables();
}

void WeaponAxe::ApplyGlobalVariables()
{
    auto* gv = GlobalVariables::GetInstance();
    damage_ = gv->GetFloatValue(GetGlobalVariableGroupName(), "Damage");
    cooldown_ = gv->GetFloatValue(GetGlobalVariableGroupName(), "Cooldown");
    projectileInitialSpeedY_ = gv->GetFloatValue(GetGlobalVariableGroupName(), "Projectile InitialSpeedY");
    projectileLifetime_ = gv->GetFloatValue(GetGlobalVariableGroupName(), "Projectile Lifetime");
    projectileCount_ = static_cast<int>(gv->GetFloatValue(GetGlobalVariableGroupName(), "Projectile Count"));
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
    ImGui::Begin("Weapon Axe");
    ImGui::Separator();
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

        projectiles_.push_back(std::move(newProjectile));
    }
}

void WeaponAxe::LevelUp()
{
    level_++;
    if (level_ == 2) projectileCount_++;
    if (level_ == 3) damage_ *= 1.5f;
    if (level_ == 4) projectileCount_++;
    if (level_ == 5) cooldown_ *= 0.8f;
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