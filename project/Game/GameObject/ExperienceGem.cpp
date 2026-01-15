#include "ExperienceGem.h"
#include "Player.h"          
#include "CollisionConfig.h" 
#include "ModelHandle.h"     
#include "TimeManager.h"     
#include "TextureHandle.h"  
#include "AudioHandle.h"
#include "AudioPlayer.h"

using namespace FromEngine;

ExperienceGem::ExperienceGem(Engine* engine, Camera* camera, Player* player)
    : engine_(engine), camera_(camera), player_(player)
{
    // 経験値オーブ用のモデルをロード
    model_ = std::make_unique<Model>(engine_, camera_, ModelHandle::Get(ModelID::cube));
	model_->SetTextureHandle(TextureHandle::Get(TextureID::white1x1));

    model_->SetEnableOutline(true);
    model_->SetColor(0xFFFF00FF);
}

void ExperienceGem::Initialize()
{
    SetRadius(0.3f); // オーブの当たり判定の半径

    // 衝突属性を設定 
    SetCollisionAttribute(kCollisionAttributeExpGem);
    // 衝突対象はプレイヤーのみ
    SetCollisionMask(kCollisionAttributePlayer);

    model_->GetTransform().translation_.y = 0.5f;
}

void ExperienceGem::Update()
{
    WorldTransform& transform = model_->GetTransform();

    // プレイヤーへの引き寄せ処理
    Vector3 playerPos = player_->GetWorldPosition();
    Vector3 selfPos = GetWorldPosition();

    Vector3 direction = playerPos - selfPos;
    float distance = direction.Length();

    // 磁石（引き寄せ）範囲内に入ったら、プレイヤーに向かって移動
    if (distance < magnetRadius_)
    {
        float deltaTime = TimeManager::GetInstance()->GetDeltaTime();
        direction = direction.Normalize();

        transform.translation_ += direction * moveSpeed_ * deltaTime;
    }

    transform.UpdateMatrix();

    model_->SetEmissiveIntensity(4.0f);
    model_->materialHandle_.materialData->enableRim = true;
    model_->materialHandle_.materialData->rimColor = { 255.0f / 255.0f,137.0f / 255.0f,51.0f / 255.0f };
    model_->materialHandle_.materialData->rimPower = 3.8f;
    model_->materialHandle_.materialData->rimIntensity = 1.7f;
}

void ExperienceGem::Draw()
{
    if (!isCollected_) {
        model_->Draw();
    }
}

void ExperienceGem::OnCollision(Collider* other)
{
    // プレイヤーと衝突したら
    if (other->GetCollisionAttribute() & kCollisionAttributePlayer)
    {
        // 収集フラグを立てる
        isCollected_ = true;

        AudioPlayer::GetInstance().Play(AudioHandle::Get(AudioID::exp), false, 100);
    }
}

Vector3 ExperienceGem::GetWorldPosition()
{
    // ワールド座標を入れる変数
    Vector3 worldPos;
    // ワールド行列の平行移動成分を取得(ワールド座標)
    worldPos.x = model_->GetTransform().matWorld_.m[3][0];
    worldPos.y = model_->GetTransform().matWorld_.m[3][1];
    worldPos.z = model_->GetTransform().matWorld_.m[3][2];
    return worldPos;
}