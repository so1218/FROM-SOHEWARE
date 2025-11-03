#include "KnifeProjectile.h"
#include "ModelHandle.h" // ModelID を使うため
#include "TimeManager.h" // deltaTime を使うため（あれ、Updateの引数でもらってるから不要かも）

KnifeProjectile::KnifeProjectile(Engine* engine, Camera* camera, const Vector3& startPos, const Vector3& direction)
{
    // プレイヤーと同じように、自分のモデルを生成する
    // ここでは仮に 'cube' を使いますが、'knife' のModelIDがあるならそれを使ってください
    model_ = std::make_unique<Model>(engine, camera, std::move(ModelHandle::Get(ModelID::sphere)));

    // 初期位置を設定
    model_->GetTransform().translation_ = startPos;
    // 発射された方向
    direction_ = direction.Normalize();

    // TODO: このオブジェクトの衝突属性を設定する (PlayerのInitializeを参考に)
    // SetCollisionAttribute(kCollisionAttributePlayerProjectile);
    // SetCollisionMask(kCollisionAttributeEnemy);
}

KnifeProjectile::~KnifeProjectile()
{
    // model_ の unique_ptr がここで自動的に破棄される
    // エンジン側で描画リストなどから自動で除去されることを期待
}

void KnifeProjectile::Update(float deltaTime)
{
    // 設定された方向へ、まっすぐ飛んでいく
    model_->GetTransform().translation_ += direction_ * speed_ * deltaTime;

    // 生存時間を減らす
    lifetime_ -= deltaTime;
}

void KnifeProjectile::Draw()
{
    // 自分のモデルに描画を命令する
    model_->Draw();

}