#include "ExperienceGem.h"
#include "Player.h"          
#include "CollisionConfig.h" 
#include "ModelHandle.h"     
#include "TimeManager.h"     
#include "TextureHandle.h"  

ExperienceGem::ExperienceGem(Engine* engine, Camera* camera, Player* player)
    : engine_(engine), camera_(camera), player_(player)
{
    // 経験値オーブ用のモデルをロード (例: ModelID::exp_orb)
    model_ = std::make_unique<Model>(engine_, camera_, ModelHandle::Get(ModelID::cube));
	model_->SetTextureHandle(TextureHandle::Get(TextureID::uvChecker));
}

void ExperienceGem::Initialize()
{
    SetRadius(0.3f); // オーブの当たり判定の半径

    // 衝突属性を設定 
    SetCollisionAttribute(kCollisionAttributeExpGem);
    // 衝突対象はプレイヤーのみ
    SetCollisionMask(kCollisionAttributePlayer);
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