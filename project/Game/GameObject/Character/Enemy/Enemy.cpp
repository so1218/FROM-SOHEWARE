#include "pch.h"
#include "Enemy.h"
#include "GameDefine.h"
#include "TimeManager.h"

using namespace FE;

Enemy::Enemy(Engine* engine, int id, const std::string& parentGroupName)
    : engine_(engine), id_(id) // ★親クラスのコンストラクタを忘れずに
{
    model_ = std::make_unique<FE::Model>(engine_, "enemy"); // モデル名は任意

    // 初期配置が重ならないようにズラす
    basePosition_ = { static_cast<float>(id) * 5.0f, 2.0f, 10.0f };

    std::string childGroupName = "Enemy_" + std::to_string(id_);
    binder_ = std::make_unique<FE::PropertyBinder>(engine_, parentGroupName, childGroupName);
    collider_ = std::make_unique<FE::Collider>(this);
}

Enemy::~Enemy()
{
    if (spotLightIndex_ != -1)
    {
        engine_->GetLightManager()->ReturnSpotLight(spotLightIndex_);
        spotLightIndex_ = -1;
    }
}

void Enemy::Initialize()
{
    collider_->RegisterToManager();
    SetTag(ObjectTag::Enemy);
    binder_->BindModel("enemyModel", model_.get());

    binder_->Bind("BasePosition", &basePosition_, basePosition_);

    // 振幅と速さもエディタで調整できるようにする
    // デフォルト: Y軸(上下)に2m, 速さ1.0。X軸(左右)に3m, 速さ0.5
    binder_->Bind("Amplitude", &amplitude_, { 3.0f, 2.0f, 0.0f });
    binder_->Bind("Frequency", &frequency_, { 0.5f, 1.0f, 0.0f });
    binder_->Bind("Phase", &phase_, { 0.0f, 0.0f, 0.0f });

    binder_->BindColor("SpotColor", &spotColor_, { 1.0f, 1.0f, 0.8f, 1.0f });
    binder_->Bind("SpotIntensity", &spotIntensity_, 8.0f);
    binder_->Bind("SpotDistance", &spotDistance_, 20.0f);
    binder_->Bind("SpotAngle", &spotAngleDeg_, 30.0f);
    binder_->Bind("SpotVolumetric", &spotVolumetric_, 4.0f);
    binder_->Bind("SpotDirection", &spotDirection_, { 0.0f, -1.0f, 0.0f });

    binder_->Bind("ColliderRadius", &colliderRadius_, 1.0f);
    binder_->Bind("ColliderOffset", &colliderOffset_, { 0.0f, 0.0f, 0.0f });

    // スポットライトの空きスロットを要求
    spotLightIndex_ = engine_->GetLightManager()->RequestSpotLight();

    collider_->SetApplyRotation(false);
    collider_->SetRadius(colliderRadius_);
    collider_->SetCenterOffset(colliderOffset_);

    auraEmitter_ = engine_->GetParticleSystem()->CreateEmitter("enemyAura");
    auraEmitter_->SetTargetToFollow(&model_->GetTransform());
    engine_->GetParticleSystem()->AddEmitter(std::move(auraEmitter_));
}

void Enemy::Update()
{
    if (IsDead()) return;

    // 1フレームあたりの経過時間を足す
    time_ += TimeManager::GetInstance()->GetDeltaTime();

    // サイン波を使ってオフセット（ズレ）を計算
    constexpr float radian = Math::PI / 180.0f;

    // サイン波に Phase（位相）を足してオフセットを計算
    FE::Vector3 offset = {
        std::sin(time_ * frequency_.x + phase_.x * radian) * amplitude_.x,
        std::sin(time_ * frequency_.y + phase_.y * radian) * amplitude_.y,
        std::sin(time_ * frequency_.z + phase_.z * radian) * amplitude_.z
    };

    FE::Vector3 currentPos = basePosition_ + offset;

    // -----------------------------------------------------------------
    // 【追加】進行方向の計算と回転の適用
    // -----------------------------------------------------------------
    // 新しい座標を代入する前の translation_ は「1フレーム前の座標」
    FE::Vector3 prevPos = model_->GetTransform().translation_;
    FE::Vector3 velocity = {
        currentPos.x - prevPos.x,
        currentPos.y - prevPos.y,
        currentPos.z - prevPos.z
    };

    // 移動量がゼロの時（最初のフレームなど）に計算がバグるのを防ぐ
    float speedSq = velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z;
    if (speedSq > 0.000001f)
    {
        // ベクトルを正規化（長さを1にする）して Forward（前方向）を作る
        float speed = std::sqrt(speedSq);
        FE::Vector3 forward = { velocity.x / speed, velocity.y / speed, velocity.z / speed };

        // 上方向を定義（Yアップ）
        FE::Vector3 up = { 0.0f, 1.0f, 0.0f };

        // LookRotation を使って「指定した方向を向くクォータニオン」を生成
        FE::Quaternion targetRotation = FE::Quaternion::LookRotation(-forward, up);

        // 回転を適用
        model_->GetTransform().SetRotation(targetRotation);
    }
    // -----------------------------------------------------------------

    // 座標の更新
    model_->GetTransform().translation_ = currentPos;
    SetTransform(model_->GetTransform());

    // スポットライトの追従と更新
    if (spotLightIndex_ != -1)
    {
        // 略 (元のコードのまま)
        engine_->GetLightManager()->UpdateSpotLightTransform(spotLightIndex_, currentPos, spotDirection_);

        float cosAngle = std::cos(spotAngleDeg_ * radian);

        engine_->GetLightManager()->UpdateSpotLightProperties(
            spotLightIndex_,
            spotColor_,
            spotIntensity_,
            spotDistance_,
            cosAngle,
            spotVolumetric_
        );
    }

    collider_->SetRadius(colliderRadius_);
    collider_->SetCenterOffset(colliderOffset_);
}

void Enemy::Draw()
{
    if (model_) model_->Draw();
    collider_->DrawCollider();
}

void Enemy::DebugDraw()
{
#ifdef IS_DEVELOPMENT
    ImGui::PushID(id_);
    std::string headerName = "敵" + std::to_string(id_) + " の設定";
    std::string label = "敵 " + std::to_string(id_) + " のインスペクター";

    binder_->DrawModel("enemyModel", label);
    if (ImGui::CollapsingHeader(headerName.c_str()))
    {
        ImGui::Text("移動設定");
        binder_->Draw("BasePosition", "基準座標 (中心)");
        binder_->Draw("Amplitude", "移動幅 (X, Y, Z)");
        binder_->Draw("Frequency", "移動スピード (X, Y, Z)");
        binder_->Draw("Phase", "波のズレ");

        ImGui::Text("スポットライト設定");
        if (spotLightIndex_ == -1) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "ライトの空きがない");
        }
        else {
            binder_->Draw("SpotColor", "色");
            binder_->Draw("SpotIntensity", "ライト輝度");
            binder_->Draw("SpotDistance", "届く距離");
            binder_->Draw("SpotAngle", "照射角 (度数)");
            binder_->Draw("SpotVolumetric", "ボリュームフォグ輝度");
            binder_->Draw("SpotDirection", "照射方向 (X, Y, Z)");
        }

        ImGui::Text("当たり判定設定");
        binder_->Draw("ColliderRadius", "半径");
        binder_->Draw("ColliderOffset", "オフセット");
    }
    ImGui::PopID();
#endif
}

void Enemy::OnCollisionEnter(FE::Collider* mine, FE::Collider* other)
{
    // プレイヤーに当たったらダメージ処理などを行う
    FE::GameObject* hitObject = other->GetOwner();
    if (hitObject && hitObject->CompareTag(ObjectTag::Player))
    {
        // プレイヤーにダメージを与えたり、爆発エフェクトを出したりする
    }
}