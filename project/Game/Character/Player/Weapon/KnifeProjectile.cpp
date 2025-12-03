#include "KnifeProjectile.h"
#include "ModelHandle.h" 
#include "TimeManager.h" 
#include "CollisionConfig.h" 
#include "Enemy.h" 
#include "TextureHandle.h" 

KnifeProjectile::KnifeProjectile(Engine* engine, Camera* camera, const Vector3& startPos, const Vector3& direction, const Vector3& collisionSize)
{
    model_ = std::make_unique<Model>(engine, camera, std::move(ModelHandle::Get(ModelID::knife)));
    model_->SetTextureHandle(TextureHandle::Get(TextureID::knife));
    model_->GetTransform().translation_ = startPos;
    model_->GetTransform().scale_ = { 0.5f, 0.5f, 0.5f };
    direction_ = direction.Normalize();

    // 進行方向ベクトルからY軸回転(Yaw)を計算
    float initialYaw = atan2(direction_.x, direction_.z);
    model_->GetTransform().rotation_.y = initialYaw;
    model_->GetTransform().rotationQuaternion_ = Quaternion::QuaternionFromEuler(model_->GetTransform().rotation_);

    // 衝突判定の初期化
    collisionSize_ = collisionSize;
    SetRadius(collisionSize_.x);
    UpdateAABB();       

    // 衝突属性設定
    SetCollisionAttribute(kCollisionAttributePlayerWeaponKnife);
    SetCollisionMask(kCollisionAttributeEnemy);            
}

KnifeProjectile::~KnifeProjectile()
{
    
}

void KnifeProjectile::Update(float deltaTime)
{
    if (IsDead()) return;

    // 移動処理
    model_->GetTransform().translation_ += direction_ * speed_ * deltaTime;
    lifetime_ -= deltaTime;

    // 行列と当たり判定を更新
    model_->GetTransform().UpdateMatrix();
    UpdateAABB();

    model_->SetEmissiveIntensity(4.0f);
    model_->materialHandle_.materialData->enableRim = true;
    model_->materialHandle_.materialData->rimColor = { 255.0f / 255.0f,137.0f / 255.0f,51.0f / 255.0f };
    model_->materialHandle_.materialData->rimPower = 3.8f;
    model_->materialHandle_.materialData->rimIntensity = 1.7f;
    model_->SetEnableOutline(true);
}

void KnifeProjectile::Draw()
{
    if (IsDead()) return;
    model_->Draw();
}

void KnifeProjectile::OnCollision(Collider* other)
{
    // 敵と衝突した場合の処理
    if (other->GetCollisionAttribute() & kCollisionAttributeEnemy)
    {
        Enemy* enemy = static_cast<Enemy*>(other);
        enemy->TakeDamage(damage_, GetWorldPosition());
        isHit_ = true;
    }
}

Vector3 KnifeProjectile::GetWorldPosition()
{
    Vector3 worldPos;
    worldPos.x = model_->GetTransform().matWorld_.m[3][0];
    worldPos.y = model_->GetTransform().matWorld_.m[3][1];
    worldPos.z = model_->GetTransform().matWorld_.m[3][2];
    return worldPos;
}

void KnifeProjectile::UpdateAABB()
{
    // 当たり判定ボックスを更新
    Vector3 center = model_->GetTransform().GetWorldPosition();

    float halfW = collisionSize_.x / 2.0f;
    float halfH = collisionSize_.y / 2.0f;
    float halfD = collisionSize_.z / 2.0f;

    aabb_.min = { center.x - halfW, center.y - halfH, center.z - halfD };
    aabb_.max = { center.x + halfW, center.y + halfH, center.z + halfD };
}