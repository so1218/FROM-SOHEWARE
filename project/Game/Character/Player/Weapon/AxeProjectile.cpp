#include "AxeProjectile.h"
#include "ModelHandle.h" 
#include "TimeManager.h" 
#include "CollisionConfig.h" 
#include "TextureHandle.h" 
#include "AudioHandle.h"
#include "AudioPlayer.h"

AxeProjectile::AxeProjectile(Engine* engine, Camera* camera, const Vector3& startPos, const Vector3& initialVelocity, float initialYaw)
{
    // モデルを作って開始位置に置く
    model_ = std::make_unique<Model>(engine, camera, std::move(ModelHandle::Get(ModelID::axe)));
	model_->SetTextureHandle(TextureHandle::Get(TextureID::axe));
    model_->GetTransform().translation_ = startPos;
	model_->GetTransform().scale_ = { 0.5f, 0.5f, 0.5f };

    velocity_ = initialVelocity; 
    model_->GetTransform().rotation_.y = initialYaw;

    model_->GetTransform().rotationQuaternion_ = Quaternion::QuaternionFromEuler(model_->GetTransform().rotation_);

    // 衝突判定を初期化
    Vector3 size = { 0.4f, 0.4f, 0.4f };
    SetRadius(size.x);
    UpdateAABB();

    SetCollisionAttribute(kCollisionAttributePlayerWeaponAxe);
    SetCollisionMask(kCollisionAttributeEnemy);
}

AxeProjectile::~AxeProjectile() {}

void AxeProjectile::Update()
{
    if (IsDead()) return;

    float deltaTime = TimeManager::GetInstance()->GetDeltaTime();

    // 重力を加算
    velocity_.y += gravity_ * deltaTime;

    // 速度を位置に反映
    model_->GetTransform().translation_ += velocity_ * deltaTime;

    // 斧を回転させる
    model_->GetTransform().rotation_.x += 10.0f * deltaTime;
	model_->GetTransform().rotationQuaternion_ = Quaternion::QuaternionFromEuler(model_->GetTransform().rotation_);

    lifetime_ -= deltaTime;

    // 行列と当たり判定を更新
    model_->GetTransform().UpdateMatrix();
    UpdateAABB();

    model_->SetEmissiveIntensity(2.0f);
    model_->materialHandle_.materialData->enableRim = true;
    model_->materialHandle_.materialData->rimColor = { 255.0f / 255.0f,137.0f / 255.0f,51.0f / 255.0f };
    model_->materialHandle_.materialData->rimPower = 3.8f;
    model_->materialHandle_.materialData->rimIntensity = 1.7f;
    model_->SetEnableOutline(true);
}

void AxeProjectile::Draw()
{
    if (IsDead()) return;
    model_->Draw();
    DrawCollider();
}

void AxeProjectile::OnCollision(Collider* other)
{
    if (other->GetCollisionAttribute() & kCollisionAttributeEnemy)
    {
        Enemy* enemy = static_cast<Enemy*>(other);
        enemy->TakeDamage(damage_, GetWorldPosition());
        isHit_ = true; // ヒットしたら消える
        AudioPlayer::GetInstance().Play(AudioHandle::Get(AudioID::enemyHit), false, 100);
    }
}

void AxeProjectile::SetSize(const Vector3& size)
{
 
    // 当たり判定の半径を変更
    SetRadius(size.x);

    UpdateAABB();
}

Vector3 AxeProjectile::GetWorldPosition()
{
    Vector3 worldPos;
    worldPos.x = model_->GetTransform().matWorld_.m[3][0];
    worldPos.y = model_->GetTransform().matWorld_.m[3][1];
    worldPos.z = model_->GetTransform().matWorld_.m[3][2];
    return worldPos;
}

void AxeProjectile::UpdateAABB()
{
    // 当たり判定の箱を更新
    float r = GetRadius();
    Vector3 size = { r * 2.0f, r * 2.0f, r * 2.0f };
    Vector3 center = model_->GetTransform().GetWorldPosition();

    float halfW = size.x / 2.0f;
    float halfH = size.y / 2.0f;
    float halfD = size.z / 2.0f;
    aabb_.min = { center.x - halfW, center.y - halfH, center.z - halfD };
    aabb_.max = { center.x + halfW, center.y + halfH, center.z + halfD };
}