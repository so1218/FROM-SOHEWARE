#include "pch.h"
#include "Player.h"
#include "CollisionConfig.h"
#include "AnimationModel.h"
#include "Input.h"
#include "MathUtils.h"  
#include "Collision.h"   
#include "TimeManager.h"
#include "AudioPlayer.h"
#include "GameDefine.h"
#include "PlayerStateNormal.h"
#include "CollisionManager.h"
#include "GameObjectManager.h"
#include "PlayerWeapon.h"
#include "PlayerReticle.h"
#include "Enemy.h"

using namespace FE;

Player::Player(Engine* engine, Camera* camera) : 
engine_(engine), camera_(camera)
{
	SetTag(ObjectTag::Player);

	animationModel_ = std::make_unique<AnimationModel>(engine_, "humanMesh", "humanRun");

	// 武器生成 & 初期化
	weapon_ = std::make_unique<PlayerWeapon>(engine_);
	reticle_ = std::make_unique<PlayerReticle>(engine_);

	binder_ = std::make_unique<PropertyBinder>(engine, "Player");
	collider_ = std::make_unique<Collider>(this);
}

void Player::Initialize()
{
	moveDirection_ = { 0.0f, 0.0f, 0.0f };

	collider_->SetType(CollisionShapeType::AABB);
	collider_->SetCollisionAttribute(kCollisionAttributePlayer);
	collider_->SetCollisionMask(kCollisionAttributeEnemy | kCollisionAttributeProp);

	animationModel_->Play("humanRun");

	// PropertyBinder への登録
	binder_->BindAnimationModel("PlayerModel", animationModel_.get());
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

	binder_->Bind("AimMoveSpeed", &config.aimMoveSpeed, 0.005f);
	binder_->Bind("AimToIdleBlendTime", &config.aimToIdleBlendTime, 0.01f);
	binder_->Bind("ShootRecoilTime", &config.shootRecoilTime, 0.01f);

	binder_->Bind("ColliderOffset", &colliderOffset_, { 0.0f, 1.0f, 0.0f });
	binder_->Bind("ColliderSize", &colliderSize_, { 0.5f, 1.0f, 0.5f });

	binder_->Bind("InteractionRadius", &interactionRadius_, 0.1f);
	binder_->Bind("InteractionForce", &interactionForce_, 0.1f);
	binder_->Bind("MaxVerticalDist", &maxVerticalDist_, 0.1f);

	binder_->Bind("MaxDamageMultiplier", &config.maxDamageMultiplier, 0.05f, 0.1f, 1.0f, 3.0f);
	binder_->Bind("MaxBulletSpread", &config.maxBulletSpread, 0.002f, 0.1f, 0.0f, 0.2f);

	stateMachine_ = std::make_unique<StateMachine<Player>>(this);
	stateMachine_->ChangeState(PlayerStateNormal::GetInstance());

	collider_->RegisterToManager();

	auraEmitter_ = engine_->GetParticleSystem()->CreateEmitter("playerAura");
	auraEmitter_->SetTargetToFollow(&animationModel_->GetTransform());
	engine_->GetParticleSystem()->AddEmitter(std::move(auraEmitter_));

	weapon_->Initialize();
	reticle_->Initialize();
}

// 更新処理
void Player::Update()
{
	float deltaTime = TimeManager::GetInstance()->GetDeltaTime();

	// 1. ステートマシン更新
	stateMachine_->Update();
	isGroundedOnObject_ = false;

	// 2. 衝突判定・境界チェック
	if (treeField_)
	{
		Vector3 pos = GetTransform().translation_;
		float playerRadius = colliderSize_.x * 0.5f;

		if (treeField_->ResolveCollision(pos, playerRadius))
		{
			GetTransform().translation_ = pos;
		}
	}

	{
		Vector3& pos = GetTransform().translation_;
		pos.x = std::clamp(pos.x, -511.0f, 511.0f);
		pos.z = std::clamp(pos.z, -511.0f, 511.0f);
	}

	collider_->SetCenterOffset(colliderOffset_);
	collider_->SetSize(colliderSize_);

	// 3. 基本アニメーションの更新
	animationModel_->Update();
	animationModel_->GetTransform().translation_ = GetTransform().translation_;
	animationModel_->GetTransform().rotationQuaternion_ = GetTransform().rotationQuaternion_;

	// 4. エイム時の姿勢（上半身ボーン）更新
	float cameraPitch = 0.0f;
	bool isAiming = followCamera_ ? followCamera_->IsAiming() : false;

	if (isAiming && camera_)
	{
		Vector3 camForward = camera_->GetWorldTransform().rotationQuaternion_.RotateVector({ 0.0f, 0.0f, 1.0f });
		cameraPitch = std::asin(std::clamp(camForward.y, -1.0f, 1.0f));
	}

	if (isAiming && std::abs(cameraPitch) > 0.001f)
	{
		Quaternion pitchOffset = Quaternion::QuaternionFromEuler({ -cameraPitch * 0.5f, 0.0f, 0.0f });
		Quaternion neckOffset = Quaternion::QuaternionFromEuler({ -cameraPitch * 0.3f, 0.0f, 0.0f });

		animationModel_->AddJointRotationOffset("mixamorig1:Spine1", pitchOffset);
		animationModel_->AddJointRotationOffset("mixamorig1:Spine2", pitchOffset);
		animationModel_->AddJointRotationOffset("mixamorig1:Neck", neckOffset);

		animationModel_->PostUpdateSkeleton();
	}

	// 5. 武器の更新（右手ボーン行列を渡す）
	Matrix4x4 rightHandMatrix = animationModel_->GetJointWorldMatrix("mixamorig1:RightHand");
	if (weapon_)
	{
		weapon_->Update(rightHandMatrix, camera_);
	}

	// レティクルの更新
	bool isMoving = moveDirection_.LengthSq() > 0.001f;
	if (reticle_)
	{
		reticle_->Update(isMoving, isAiming);
	}

	// 6. インタラクションデータの送信
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
	auto& input = Input::GetInstance();
	const int controllerId = 0; // 1P想定

	// 1. スティックの生入力を取得 (-32768 ~ 32767)
	float stickX = static_cast<float>(input.GetLeftStickX(controllerId));
	float stickY = static_cast<float>(input.GetLeftStickY(controllerId));

	// キーボード入力（WASD）の加算
	if (input.IsKeyPressed(DIK_W)) stickY += 32768.0f;
	if (input.IsKeyPressed(DIK_S)) stickY -= 32768.0f;
	if (input.IsKeyPressed(DIK_D)) stickX += 32768.0f;
	if (input.IsKeyPressed(DIK_A)) stickX -= 32768.0f;

	// 2. -1.0f ~ +1.0f に正規化
	float inputX = stickX / 32768.0f;
	float inputY = stickY / 32768.0f;

	// 3. 入力ベクトルの長さを算出（倒し具合）
	float inputLength = std::sqrt(inputX * inputX + inputY * inputY);
	float deadZone = static_cast<float>(STICK_THRESHOLD) / 32768.0f;

	Vector3 dir = { 0.0f, 0.0f, 0.0f };

	// デッドゾーンを超えている場合のみ処理
	if (inputLength > deadZone)
	{
		// デッドゾーンを超えた分を 0.0f ~ 1.0f にスケーリング（入力強度の算出）
		float inputMagnitude = (std::min)(1.0f, (inputLength - deadZone) / (1.0f - deadZone));

		// 入力方向の単位ベクトル
		float dirX = inputX / inputLength;
		float dirY = inputY / inputLength;

		// 4. カメラの向きに合わせてワールド移動方向を決定
		Vector3 cameraForward = camera_->GetWorldTransform().rotationQuaternion_.RotateVector({ 0.0f, 0.0f, 1.0f });
		Vector3 cameraRight = camera_->GetWorldTransform().rotationQuaternion_.RotateVector({ 1.0f, 0.0f, 0.0f });

		cameraForward.y = 0.0f;
		cameraRight.y = 0.0f;
		cameraForward = cameraForward.Normalize();
		cameraRight = cameraRight.Normalize();

		// 方向 × 入力強度（倒し具合）を掛け合わせて360度アナログ移動ベクトルを作成
		dir = (cameraForward * dirY + cameraRight * dirX).Normalize() * inputMagnitude;
	}

	return dir;
}

void Player::UpdateRotation(const Vector3& moveDir)
{
	float deltaTime = TimeManager::GetInstance()->GetDeltaTime();
	if (moveDir.Length() > 0.0f) 
	{
		lastMoveDirection_ = moveDir;
	}

	if (lastMoveDirection_.Length() > 0.001f)
	{
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

void Player::UpdateAimRotation()
{
	float deltaTime = TimeManager::GetInstance()->GetDeltaTime();

	// カメラの前方ベクトル
	Vector3 cameraForward = camera_->GetWorldTransform().rotationQuaternion_.RotateVector({ 0.0f, 0.0f, 1.0f });
	cameraForward.y = 0.0f;

	if (cameraForward.LengthSq() > 0.001f)
	{
		cameraForward = cameraForward.Normalize();
		float targetAngleY = std::atan2(cameraForward.x, cameraForward.z);
		Quaternion targetRotation = Quaternion::QuaternionFromEuler({ 0.0f, targetAngleY, 0.0f });

		// カメラの向きに補間
		Quaternion currentRotation = GetTransform().rotationQuaternion_;
		GetTransform().rotationQuaternion_ = Quaternion::Slerp(currentRotation, targetRotation, 15.0f * deltaTime);
	}
}

void Player::FireWeapon()
{
	if (!weapon_ || !camera_) return;

	if (followCamera_)
	{
		followCamera_->AddRecoil(0.08f, 0.03f);
	}

	CollisionManager* colManager = GetManager() ? GetManager()->GetCollisionManager() : nullptr;

	// レティクルから現在のフォーカス率を取得して射撃に渡す
	float focusRatio = reticle_->GetFocusRatio();

	if (weapon_->Fire(camera_, focusRatio, config.maxDamageMultiplier, config.maxBulletSpread, colManager))
	{
		if (reticle_)
		{
			reticle_->OnShootRecoil(); // 射撃時の反動拡散
		}
	}
}

void Player::UpdateFootstepEvents()
{
	if (!animationModel_) return;

	// 現在のアニメーション進捗率
	float currentNormalizedTime = animationModel_->GetNormalizedTime();

	// humanRun アニメーションの足がつくタイミング（割合）
	const float leftFootStepTime = 0.20f;
	const float rightFootStepTime = 0.70f;

	// アニメーションが1周して0.0に戻った場合の補正
	if (currentNormalizedTime < prevAnimNormalizedTime_)
	{
		prevAnimNormalizedTime_ -= 1.0f;
	}

	// 前フレームと現在フレームの間で接地タイミングを跨いだか判定
	bool isLeftFoot = (prevAnimNormalizedTime_ < leftFootStepTime && currentNormalizedTime >= leftFootStepTime);
	bool isRightFoot = (prevAnimNormalizedTime_ < rightFootStepTime && currentNormalizedTime >= rightFootStepTime);

	if (isLeftFoot || isRightFoot)
	{
		// 単発の足音SEを再生（PlayUnique ではなく単発再生用関数を使用）
		// ※ 音声再生クラスの単発再生関数（Play や PlayOneShot 等）に合わせて変更してください
		AudioPlayer::GetInstance().Play("playerRunning", false, 10);
	}

	// 次フレームのために正規化した進捗率（0.0～1.0の範囲）を保持
	prevAnimNormalizedTime_ = std::fmod(currentNormalizedTime, 1.0f);
	if (prevAnimNormalizedTime_ < 0.0f) prevAnimNormalizedTime_ += 1.0f;
}

void Player::ResetFootstepState()
{
	if (animationModel_)
	{
		prevAnimNormalizedTime_ = animationModel_->GetNormalizedTime();
	}
	else
	{
		prevAnimNormalizedTime_ = 0.0f;
	}
}

void Player::OnCollisionEnter(Collider* mine, Collider* other)
{

}

void Player::Draw()
{
	animationModel_->GetTransform().translation_ = GetTransform().translation_;

	GetTransform().UpdateMatrix();
	collider_->DrawCollider();

	animationModel_->Draw();

	weapon_->Draw();
	reticle_->Draw();
}

void Player::DebugDraw()
{
#ifdef ENABLE_IMGUI
	ImGui::Begin("プレイヤー");

	binder_->DrawAnimationModel("PlayerModel", "プレイヤーインスペクター");

	ImGui::Separator();

	weapon_->DebugDraw();
	reticle_->DebugDraw();

	if (ImGui::CollapsingHeader("動き"))
	{
		binder_->Draw("RunSpeed", "走り速度");
		binder_->Draw("RotationSpeed", "回転の速さ");
	}

	if (ImGui::CollapsingHeader("エイム設定"))
	{
		binder_->Draw("AimMoveSpeed", "エイム時移動速度");
		binder_->Draw("AimToIdleBlendTime", "エイム補間時間(秒)");
		binder_->Draw("ShootRecoilTime", "射撃反動時間(秒)");
		binder_->Draw("MaxDamageMultiplier", "フォーカス時威力倍率");
		binder_->Draw("MaxBulletSpread", "最大弾道ブレ角");
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