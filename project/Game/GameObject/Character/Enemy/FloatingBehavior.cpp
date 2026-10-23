#include "pch.h"
#include "FloatingBehavior.h"
#include "Enemy.h"
#include "TimeManager.h"
#include "AudioPlayer.h"

using namespace FE;

FloatingBehavior::~FloatingBehavior()
{
    if (soundHandleID_ >= 0)
    {
        AudioPlayer::GetInstance().Stop(soundHandleID_);
        soundHandleID_ = -1;
    }
}

void FloatingBehavior::Initialize(Enemy* owner)
{
    engine_ = owner->GetEngine();
    auto binder = owner->GetBinder();

    binder->Bind("BasePosition", &basePosition_, basePosition_);
    binder->Bind("Amplitude", &amplitude_, { 3.0f, 2.0f, 0.0f });
    binder->Bind("Frequency", &frequency_, { 0.5f, 1.0f, 0.0f });
    binder->Bind("Phase", &phase_, { 0.0f, 0.0f, 0.0f });

    binder->BindColor("SpotColor", &spotColor_, { 1.0f, 1.0f, 0.8f, 1.0f });
    binder->Bind("SpotIntensity", &spotIntensity_, 8.0f);
    binder->Bind("SpotDistance", &spotDistance_, 20.0f);
    binder->Bind("SpotAngle", &spotAngleDeg_, 30.0f);
    binder->Bind("SpotVolumetric", &spotVolumetric_, 4.0f);
    binder->Bind("SpotDirection", &spotDirection_, { 0.0f, -1.0f, 0.0f });

    auto moveFireEmitter = engine_->GetParticleSystem()->CreateEmitter("floatingEnemyMoveFire");
    if (moveFireEmitter)
    {
        moveFireEmitterPtr_ = moveFireEmitter.get();
        moveFireEmitterPtr_->SetTargetToFollow(&owner->GetModel()->GetTransform());
        engine_->GetParticleSystem()->AddEmitter(std::move(moveFireEmitter));
    }

    auto moveSmokeEmitter = engine_->GetParticleSystem()->CreateEmitter("floatingEnemyMoveSmoke");
    if (moveSmokeEmitter)
    {
        moveSmokeEmitterPtr_ = moveSmokeEmitter.get();
        moveSmokeEmitterPtr_->SetTargetToFollow(&owner->GetModel()->GetTransform());
        engine_->GetParticleSystem()->AddEmitter(std::move(moveSmokeEmitter));
    }

    auto explosion = engine_->GetParticleSystem()->CreateEmitter("floatingEnemyExplosion");
	explosionEmitterPtr_ = explosion.get();
    if (explosion)
    {
        engine_->GetParticleSystem()->AddEmitter(std::move(explosion));
    }

    soundHandleID_ = AudioPlayer::GetInstance().Play3D("floatingEnemy", owner->GetTransform().translation_, true, 100, 100);
}

void FloatingBehavior::Update(Enemy* owner)
{
    time_ += FE::TimeManager::GetInstance()->GetDeltaTime();
    constexpr float radian = FE::Math::PI / 180.0f;

    // サイン波による位置計算
    FE::Vector3 offset = {
        std::sin(time_ * frequency_.x + phase_.x * radian) * amplitude_.x,
        std::sin(time_ * frequency_.y + phase_.y * radian) * amplitude_.y,
        std::sin(time_ * frequency_.z + phase_.z * radian) * amplitude_.z
    };

    FE::Vector3 currentPos = basePosition_ + offset;
    FE::Vector3 prevPos = owner->GetTransform().translation_;

    // 進行方向を向く回転計算
    FE::Vector3 velocity = { currentPos.x - prevPos.x, currentPos.y - prevPos.y, currentPos.z - prevPos.z };
    float speedSq = velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z;

    if (speedSq > 0.000001f)
    {
        float speed = std::sqrt(speedSq);
        FE::Vector3 forward = { velocity.x / speed, velocity.y / speed, velocity.z / speed };
        FE::Vector3 up = { 0.0f, 1.0f, 0.0f };

        FE::Quaternion targetRotation = FE::Quaternion::LookRotation(-forward, up);
        owner->GetTransform().SetRotation(targetRotation);
    }

    owner->GetTransform().translation_ = currentPos;
    owner->SyncTransform(); 

    if (soundHandleID_ >= 0)
    {
        AudioPlayer::GetInstance().SetEmitterPosition(soundHandleID_, currentPos);
    }

    // スポットライトの追従と更新
    float cosAngle = std::cos(spotAngleDeg_ * radian);

    engine_->GetLightManager()->SubmitSpotLight(
        currentPos,
        spotColor_,
        spotIntensity_,
        spotDistance_,
        spotDirection_,
        cosAngle,
        spotVolumetric_
    );
}

void FloatingBehavior::OnTakeDamage(Enemy* owner, int damage, const Vector3& hitPoint, const Vector3& hitNormal)
{

}

void FloatingBehavior::OnDeath(Enemy* owner)
{
    if (!engine_ || !owner) return;

	AudioPlayer::GetInstance().Play("floatingEnemyExplosion", false, 40);

    if (moveFireEmitterPtr_)
    {
        moveFireEmitterPtr_->Stop();
        moveFireEmitterPtr_->Destroy(); 
        moveFireEmitterPtr_ = nullptr;
    }

    if (moveSmokeEmitterPtr_)
    {
        moveSmokeEmitterPtr_->Stop();
        moveSmokeEmitterPtr_->Destroy();
        moveSmokeEmitterPtr_ = nullptr;
    }

    if (explosionEmitterPtr_)
    {
        explosionEmitterPtr_->SetPosition(owner->GetTransform().translation_);

        explosionEmitterPtr_->Play();
    }
}

void FloatingBehavior::DebugDraw(Enemy* owner)
{
#ifdef ENABLE_IMGUI
    auto binder = owner->GetBinder();

    ImGui::Text("移動設定 (Floating)");
    binder->Draw("BasePosition", "基準座標 (中心)");
    binder->Draw("Amplitude", "移動幅");
    binder->Draw("Frequency", "移動スピード");
    binder->Draw("Phase", "波のズレ");

    ImGui::Text("スポットライト設定");

    binder->Draw("SpotColor", "色");
    binder->Draw("SpotIntensity", "ライト輝度");
    binder->Draw("SpotDistance", "届く距離");
    binder->Draw("SpotAngle", "照射角");
    binder->Draw("SpotVolumetric", "ボリュームフォグ輝度");
    binder->Draw("SpotDirection", "照射方向");
    
#endif
}