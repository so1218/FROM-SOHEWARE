#include "pch.h"
#include "Player.h"
#include "CollisionConfig.h"
#include "AnimationModel.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "MathUtils.h"  
#include "Collision.h"   
#include "TimeManager.h"
#include "AudioPlayer.h"
#include "GameDefine.h"
#include "PlayerStateNormal.h"

using namespace FE;

Player::Player(Engine* engine, Camera* camera) : GameObject(),
	camera_(camera)
{
	SetTag(ObjectTag::Player);

	engine_ = engine;

	// アニメーションモデルを生成
	animationModel_ = std::make_unique<AnimationModel>(engine_, "humanMesh", "humanRun");

	binder_ = std::make_unique<PropertyBinder>(engine, "Player");
	collider_ = std::make_unique<FE::Collider>(this);
}

void Player::Initialize()
{
	// プレイヤーの基本情報を設定
	moveDirection_ = { 0.0f, 0.0f, 0.0f };
	moveSpeed_ = 0.2f;

	collider_->SetType(CollisionShapeType::AABB);

	animationModel_->Play("humanRun");

	// 衝突判定の属性設定
	collider_->SetCollisionAttribute(kCollisionAttributePlayer);
	collider_->SetCollisionMask(kCollisionAttributeEnemy);

	binder_->BindAnimationModel("PlayerModel", animationModel_.get());
	binder_->Bind("RunSpeed", &runSpeed_, 0.01f);
	binder_->Bind("RotationSpeed", &rotationSpeed_, 0.1f);
	binder_->Bind("ColliderOffset", &colliderOffset_, { 0.0f, 1.0f, 0.0f });
	binder_->Bind("ColliderSize", &colliderSize_, { 0.5f, 1.0f, 0.5f });

	stateMachine_ = std::make_unique<StateMachine<Player>>(this);
	stateMachine_->ChangeState(PlayerStateNormal::GetInstance());

	collider_->RegisterToManager();

	auraEmitter_ = engine_->GetParticleSystem()->CreateEmitter("playerAura");
	auraEmitter_->SetTargetToFollow(&animationModel_->GetTransform());
	engine_->GetParticleSystem()->AddEmitter(std::move(auraEmitter_));
}

// 更新処理
void Player::Update()
{
	float deltaTime = TimeManager::GetInstance()->GetDeltaTime();

	// 毎フレーム、現在のステートのUpdateが呼ばれる
	stateMachine_->Update();
	
	{
		// 0番目のディレクショナルライトを取得
		auto* dirLights = engine_->GetLightManager()->GetDirectionalLightData();
		if (dirLights[0].enable)
		{
			// ライト方向を正規化
			Vector3 lightDir = dirLights[0].direction;
			lightDir = lightDir.Normalize();

			// 影を落とす対象の中心座標
			Vector3 shadowTarget = GetTransform().translation_;

			// ライト位置を決定
			float distance = 100.0f;
			Vector3 lightPos = shadowTarget - (lightDir * distance);

			// 上方向ベクトル
			Vector3 up = { 0.0f, 1.0f, 0.0f };
			if (fabs(lightDir.y) > 0.99f) up = { 1.0f, 0.0f, 0.0f };

			// ライトのビュー行列を作成
			Matrix4x4 lightView = Matrix4x4::MakeLookAt(lightPos, shadowTarget, up);

			// 平行光源用の正射影行列を作成
			float size = 100.0f;
			float nearZ = -100.0f;
			float farZ = 200.0f;
			Matrix4x4 lightProj = Matrix4x4::MakeOrthographic(size, size, nearZ, farZ);

			// ビュー行列と射影行列を合成
			Matrix4x4 lightViewProj = lightView * lightProj;

			// シャドウ行列をライトマネージャに更新
			engine_->GetLightManager()->UpdateDirectionalLightShadowMatrix(0, lightViewProj);
		}
	}

	collider_->SetCenterOffset(colliderOffset_);
	collider_->SetSize(colliderSize_);

	// 最終的な行列更新
	animationModel_->Update();
	animationModel_->GetTransform().translation_ = GetTransform().translation_;
	animationModel_->GetTransform().rotationQuaternion_ = GetTransform().rotationQuaternion_;
	GetTransform().UpdateMatrix();
}

void Player::Move()
{
	float deltaTime = TimeManager::GetInstance()->GetDeltaTime();

	moveDirection_ = GetMoveDirection();

	// 移動方向がある場合、最後の移動方向を更新
	if (moveDirection_.Length() > 0.0f)
	{
		lastMoveDirection_ = moveDirection_;
	}

	// 向きを補間して回転
	if (lastMoveDirection_.Length() > 0.001f)
	{
		float targetAngleY = std::atan2(lastMoveDirection_.x, lastMoveDirection_.z);
		Quaternion targetRotation = Quaternion::QuaternionFromEuler({ 0.0f, targetAngleY, 0.0f });

		Quaternion currentRotation = GetTransform().rotationQuaternion_;
		float slerpFactor = Math::Clamp(rotationSpeed_ * deltaTime, 0.0f, 1.0f);
		Quaternion newRotation = Quaternion::Slerp(currentRotation, targetRotation, slerpFactor);

		GetTransform().rotationQuaternion_ = newRotation;
	}

	// 実際の位置更新
	GetTransform().translation_ += moveDirection_ * moveSpeed_;

	engine_->GetFluidSimulationPass()->GetSettings()->objectPos = GetTransform().translation_;
	engine_->GetFluidSimulationPass()->GetSettings()->objectVelocity = moveDirection_ * moveSpeed_;
}

// 入力から移動方向を取得
Vector3 Player::GetMoveDirection()
{
	Vector3 dir = { 0.0f, 0.0f, 0.0f };
	auto& input = Input::GetInstance();

	const int controllerId = 0; // 1P想定
	SHORT stickX = input.GetLeftStickX(controllerId);
	SHORT stickY = input.GetLeftStickY(controllerId);
	const int DEAD_ZONE = STICK_THRESHOLD;

	float normalizedX = 0.0f;
	float normalizedY = 0.0f;

	// スティックの入力を正規化
	if (abs(stickX) > DEAD_ZONE)
		normalizedX = static_cast<float>(stickX) / 32768.0f;
	if (abs(stickY) > DEAD_ZONE)
		normalizedY = static_cast<float>(stickY) / 32768.0f;

	// キーボード入力対応
	if (input.IsKeyPressed(DIK_W)) normalizedY += 1.0f;
	if (input.IsKeyPressed(DIK_S)) normalizedY -= 1.0f;
	if (input.IsKeyPressed(DIK_D)) normalizedX += 1.0f;
	if (input.IsKeyPressed(DIK_A)) normalizedX -= 1.0f;

	dir = { normalizedX, 0.0f, normalizedY };

	// カメラ方向に基づいて移動ベクトルを変換
	if (dir.Length() > 0.0f)
	{
		dir = dir.Normalize();

		Vector3 cameraForward = camera_->GetWorldTransform().rotationQuaternion_.RotateVector({ 0.0f, 0.0f, 1.0f });
		Vector3 cameraRight = camera_->GetWorldTransform().rotationQuaternion_.RotateVector({ 1.0f, 0.0f, 0.0f });

		cameraForward.y = 0.0f;
		cameraRight.y = 0.0f;
		cameraForward = cameraForward.Normalize();
		cameraRight = cameraRight.Normalize();

		dir = cameraForward * dir.z + cameraRight * dir.x;
		dir = dir.Normalize();
	}

	return dir;
}

void Player::OnCollisionEnter(FE::Collider* mine, FE::Collider* other)
{

}

void Player::Draw()
{
	animationModel_->Draw();
}

void Player::DebugDraw()
{
#ifdef IS_DEVELOPMENT
	ImGui::Begin("プレイヤー");

	binder_->DrawAnimationModel("PlayerModel", "プレイヤーインスペクター");

	ImGui::Separator();

	if (ImGui::CollapsingHeader("動き"))
	{
		binder_->Draw("RunSpeed", "走り速度");
		binder_->Draw("RotationSpeed", "回転の速さ");
	}

	if (ImGui::CollapsingHeader("コライダー"))
	{
		binder_->Draw("ColliderOffset", "位置オフセット");
		binder_->Draw("ColliderSize", "ハーフサイズ");
	}

	ImGui::Separator();

	ImGui::End();

	/*ImGuiManager::DrawGizmo(transform_);*/
	collider_->DrawCollider();
#endif
}
