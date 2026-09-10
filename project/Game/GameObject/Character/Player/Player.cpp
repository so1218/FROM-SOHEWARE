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
	weaponModel_ = std::make_unique<Model>(engine_, "rock1");

	binder_ = std::make_unique<PropertyBinder>(engine, "Player");
	collider_ = std::make_unique<FE::Collider>(this);
}

void Player::Initialize()
{
	// プレイヤーの基本情報を設定
	moveDirection_ = { 0.0f, 0.0f, 0.0f };
	weaponModel_->MakeMaterialUnique();

	collider_->SetType(CollisionShapeType::AABB);

	animationModel_->Play("humanRun");

	// 衝突判定の属性設定
	collider_->SetCollisionAttribute(kCollisionAttributePlayer);
	collider_->SetCollisionMask(kCollisionAttributeEnemy | kCollisionAttributeProp);

	binder_->BindAnimationModel("PlayerModel", animationModel_.get());
	binder_->BindModel("WeaponModel", weaponModel_.get());
	binder_->Bind("RunSpeed", &config.runSpeed, 0.01f);
	binder_->Bind("RotationSpeed", &config.rotationSpeed, 0.1f);
	binder_->Bind("IdleAnimSpeed", &config.idleAnimSpeed, 0.05f);
	binder_->Bind("RunAnimSpeed", &config.runAnimSpeed, 0.05f);
	binder_->Bind("JumpAnimSpeed", &config.jumpAnimSpeed, 0.05f);
	binder_->Bind("IdleToRunBlendTime", &config.idleToRunBlendTime, 0.01f);
	binder_->Bind("RunToIdleBlendTime", &config.runToIdleBlendTime, 0.01f);
	binder_->Bind("JumpBlendTime", &config.jumpBlendTime, 0.01f);
	binder_->Bind("JumpInitialVelocity", &config.jumpInitialVelocity, 0.1f);
	binder_->Bind("Gravity", &config.gravity, 0.1f);
	binder_->Bind("AirControlRate", &config.airControlRate, 0.05f);

	binder_->Bind("ColliderOffset", &colliderOffset_, { 0.0f, 1.0f, 0.0f });
	binder_->Bind("ColliderSize", &colliderSize_, { 0.5f, 1.0f, 0.5f });

	binder_->Bind("InteractionRadius", &interactionRadius_, 0.1f);
	binder_->Bind("InteractionForce", &interactionForce_, 0.1f);
	binder_->Bind("MaxVerticalDist", &maxVerticalDist_, 0.1f);

	stateMachine_ = std::make_unique<StateMachine<Player>>(this);
	stateMachine_->ChangeState(PlayerStateNormal::GetInstance());

	collider_->RegisterToManager();

	auraEmitter_ = engine_->GetParticleSystem()->CreateEmitter("playerAura");
	auraEmitter_->SetTargetToFollow(&animationModel_->GetTransform());
	engine_->GetParticleSystem()->AddEmitter(std::move(auraEmitter_));

	weaponModel_->GetTransform().SetParent(&rightHandTransform_);
}

// 更新処理
void Player::Update()
{
	float deltaTime = TimeManager::GetInstance()->GetDeltaTime();

	// 毎フレーム、現在のステートのUpdateが呼ばれる
	stateMachine_->Update();

	isGroundedOnObject_ = false;

	// 木との衝突判定
	if (treeField_)
	{
		Vector3 pos = GetTransform().translation_;

		// AABBコライダーの横幅の半分をプレイヤーの半径
		float playerRadius = colliderSize_.x * 0.5f;

		// 押し戻し実行
		if (treeField_->ResolveCollision(pos, playerRadius))
		{
			// 押し戻された位置をプレイヤーに再設定
			GetTransform().translation_ = pos;
		}
	}

	{
		Vector3& pos = GetTransform().translation_;
		pos.x = std::clamp(pos.x, -511.0f, 511.0f);
		pos.z = std::clamp(pos.z, -511.0f, 511.0f);
	}
	
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

	// 右手のワールド行列を取得
	Matrix4x4 rightHandWorldMatrix = animationModel_->GetJointWorldMatrix("mixamorig1:RightHand");

	// 武器に右手の行列をそのままセットする
	rightHandTransform_.matWorld_ = rightHandWorldMatrix;

	// ---------------------------------------------------------
	// ワールドインタラクション用データの作成と送信
	// ---------------------------------------------------------
	Vector3 currentPos = GetTransform().translation_;

	Vector3 velocity = { 0.0f, 0.0f, 0.0f };
	if (deltaTime > 0.0001f) {
		velocity = (currentPos - prevPosition_) / deltaTime;
	}
	prevPosition_ = currentPos;

	InteractionEntity entity{};
	entity.position = currentPos;
	entity.radius = interactionRadius_;         
	entity.velocity = velocity;
	entity.maxVerticalDist = maxVerticalDist_;  
	entity.entityType = 0;
	entity.forceMultiplier = interactionForce_; 

	engine_->GetRendererManager()->SubmitInteractionEntity(entity);
	engine_->GetRendererManager()->SetWorldInteractionCenter({ currentPos.x, currentPos.z });
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

void Player::UpdateRotation(const Vector3& moveDir)
{
	float deltaTime = TimeManager::GetInstance()->GetDeltaTime();
	if (moveDir.Length() > 0.0f) {
		lastMoveDirection_ = moveDir;
	}

	if (lastMoveDirection_.Length() > 0.001f) {
		float targetAngleY = std::atan2(lastMoveDirection_.x, lastMoveDirection_.z);
		Quaternion targetRotation = Quaternion::QuaternionFromEuler({ 0.0f, targetAngleY, 0.0f });

		Quaternion currentRotation = GetTransform().rotationQuaternion_;
		float slerpFactor = Math::Clamp(config.rotationSpeed * deltaTime, 0.0f, 1.0f);
		GetTransform().rotationQuaternion_ = Quaternion::Slerp(currentRotation, targetRotation, slerpFactor);
	}
}

void Player::ApplyHorizontalMovement(const Vector3& moveDir, float speed)
{
	GetTransform().translation_ += moveDir * speed;
}

void Player::ApplyGravity(float deltaTime)
{
	velocityY_ -= config.gravity * deltaTime;
	GetTransform().translation_.y += velocityY_ * deltaTime;
}

void Player::SnapToGround()
{
	// オブジェクトに乗っている場合
	if (isGroundedOnObject_) return;

	if (!terrain_) return;

	float groundHeight = 0.0f;
	if (terrain_->GetHeightAt(GetTransform().translation_.x, GetTransform().translation_.z, groundHeight))
	{
		// 地形より十分高い位置にいる場合は地面へ落とさない
		if (GetTransform().translation_.y <= groundHeight + 0.5f)
		{
			GetTransform().translation_.y = groundHeight;
			velocityY_ = 0.0f;
		}
	}
}

bool Player::IsGrounded() const
{
	// プロップの上に乗っている場合
	if (isGroundedOnObject_) return true;

	// 地形の上に乗っている場合
	if (!terrain_) return true;

	float groundHeight = 0.0f;
	if (terrain_->GetHeightAt(GetTransform().translation_.x, GetTransform().translation_.z, groundHeight))
	{
		return GetTransform().translation_.y <= groundHeight + 0.1f;
	}

	return false;
}

void Player::OnCollisionEnter(FE::Collider* mine, FE::Collider* other)
{

}

void Player::Draw()
{
	animationModel_->GetTransform().translation_ = GetTransform().translation_;

	GetTransform().UpdateMatrix();
	collider_->DrawCollider();

	animationModel_->Draw();
	weaponModel_->Draw();
}

void Player::DebugDraw()
{
#ifdef ENABLE_IMGUI
	ImGui::Begin("プレイヤー");

	binder_->DrawAnimationModel("PlayerModel", "プレイヤーインスペクター");
	binder_->DrawModel("WeaponModel", "武器インスペクター");

	ImGui::Separator();

	if (ImGui::CollapsingHeader("動き"))
	{
		binder_->Draw("RunSpeed", "走り速度");
		binder_->Draw("RotationSpeed", "回転の速さ");
	}

	if (ImGui::CollapsingHeader("ジャンプ"))
	{
		binder_->Draw("JumpInitialVelocity", "ジャンプ初速");
		binder_->Draw("Gravity", "重力加速度");
		binder_->Draw("AirControlRate", "空中移動制御率(0~1)");
	}

	if (ImGui::CollapsingHeader("アニメーション調整"))
	{
		binder_->Draw("IdleAnimSpeed", "待機アニメ速度");
		binder_->Draw("RunAnimSpeed", "走りアニメ速度");
		binder_->Draw("JumpAnimSpeed", "ジャンプアニメ速度");
		binder_->Draw("IdleToRunBlendTime", "待機→走り 補間時間(秒)");
		binder_->Draw("RunToIdleBlendTime", "走り→待機 補間時間(秒)");
		binder_->Draw("JumpBlendTime", "ジャンプ 補間時間(秒)");
	}

	if (ImGui::CollapsingHeader("コライダー"))
	{
		binder_->Draw("ColliderOffset", "位置オフセット");
		binder_->Draw("ColliderSize", "ハーフサイズ");
	}

	if (ImGui::CollapsingHeader("環境インタラクション"))
	{
		binder_->Draw("InteractionRadius", "干渉半径");
		binder_->Draw("InteractionForce", "押し出し強度");
		binder_->Draw("MaxVerticalDist", "有効高低差");
	}

	ImGui::Separator();

	ImGui::End();
#endif
}
