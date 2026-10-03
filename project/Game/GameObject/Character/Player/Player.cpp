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
#include "CollisionManager.h"
#include "GameObjectManager.h"
#include "PlayerWeapon.h"
#include "Enemy.h"

using namespace FE;

Player::Player(Engine* engine, Camera* camera) : GameObject(),
engine_(engine), camera_(camera)
{
	SetTag(ObjectTag::Player);

	animationModel_ = std::make_unique<AnimationModel>(engine_, "humanMesh", "humanRun");

	// 武器生成 & 初期化
	weapon_ = std::make_unique<PlayerWeapon>(engine_);
	weapon_->Initialize();

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

	reticleSprite_ = std::make_unique<Sprite>(engine_);
	reticleSprite_->SetTexture("white1x1");
	reticleSprite_->SetAnchorPoint({ 0.5f, 0.5f });
	reticleSprite_->SetIsVisible(true);

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

	binder_->Bind("ReticleLineThickness", &config.reticleLineThickness, 0.5f, 0.1f, 1.0f, 10.0f);
	binder_->Bind("ReticleLineLength", &config.reticleLineLength, 1.0f, 0.1f, 2.0f, 50.0f);
	binder_->Bind("ReticleMaxGap", &config.reticleMaxGap, 1.0f, 0.1f, 10.0f, 100.0f);
	binder_->Bind("ReticleMinGap", &config.reticleMinGap, 0.5f, 0.1f, 0.0f, 30.0f);
	binder_->Bind("ReticleCenterDotSize", &config.reticleCenterDotSize, 0.5f, 0.1f, 1.0f, 20.0f);

	binder_->Bind("ReticleFocusTime", &config.reticleFocusTime, 0.05f, 0.1f, 0.1f, 3.0f);
	binder_->Bind("MaxDamageMultiplier", &config.maxDamageMultiplier, 0.05f, 0.1f, 1.0f, 3.0f);
	binder_->Bind("MaxBulletSpread", &config.maxBulletSpread, 0.002f, 0.1f, 0.0f, 0.2f);

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

	// 画面揺れ（反動）
	if (followCamera_)
	{
		followCamera_->AddRecoil(0.08f, 0.03f);
	}

	CollisionManager* colManager = GetManager() ? GetManager()->GetCollisionManager() : nullptr;

	// 射撃処理を武器に委譲（成功時にレティクル拡散）
	if (weapon_->Fire(camera_, focusRatio_, config.maxDamageMultiplier, config.maxBulletSpread, colManager))
	{
		OnShootRecoil();
	}
}

void Player::UpdateReticle(float deltaTime, bool isMoving, bool isAiming)
{
	// ★ エイム中でない場合は即座に完全リセットして非表示にする
	if (!isAiming)
	{
		ResetReticle();
		return;
	}

	// エイム中のアルファ値フェードイン補間
	reticleAlpha_ = Math::Lerp(reticleAlpha_, 1.0f, 15.0f * deltaTime);

	// 移動していない（静止状態）場合にフォーカスを進める
	if (!isMoving)
	{
		focusTimer_ += deltaTime;
	}
	else
	{
		// 移動中は素早く拡散
		focusTimer_ -= deltaTime * config.reticleExpandSpeed;
	}

	// フォーカス時間を [0, reticleFocusTime] にクランプして正規化（0.0 ~ 1.0）
	focusTimer_ = std::clamp(focusTimer_, 0.0f, config.reticleFocusTime);
	focusRatio_ = focusTimer_ / config.reticleFocusTime;
}

void Player::ResetReticle()
{
	focusTimer_ = 0.0f;
	focusRatio_ = 0.0f;
	reticleAlpha_ = 0.0f; 
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

// 描画処理
void Player::DrawReticle()
{
	if (!reticleSprite_ || reticleAlpha_ <= 0.001f) return;

	// 画面中央座標
	float centerX = static_cast<float>(Engine::GetClientWidth()) * 0.5f;
	float centerY = static_cast<float>(Engine::GetClientHeight()) * 0.5f;

	// ★ RE2風：後半に向かって収束速度が加速する EaseInCubic を適用
	float easedRatio = Easing::Evaluate(EasingType::EaseInQuart, focusRatio_);

	// イージング適用後の割合でギャップ（中心からの距離）を計算
	float currentGap = Math::Lerp(config.reticleMaxGap, config.reticleMinGap, easedRatio);

	// 完全収束時の判定
	bool isFullyFocused = (focusRatio_ >= 0.98f);

	float thickness = config.reticleLineThickness;
	float length = config.reticleLineLength;

	// ---------------------------------------------------------
	// 1. 上下左右 4本のレティクル線の描画
	// ---------------------------------------------------------

	// 【上線】
	reticleSprite_->SetPosition({ centerX, centerY - currentGap - length * 0.5f });
	reticleSprite_->SetSize({ thickness, length });
	reticleSprite_->Draw();

	// 【下線】
	reticleSprite_->SetPosition({ centerX, centerY + currentGap + length * 0.5f });
	reticleSprite_->SetSize({ thickness, length });
	reticleSprite_->Draw();

	// 【左線】
	reticleSprite_->SetPosition({ centerX - currentGap - length * 0.5f, centerY });
	reticleSprite_->SetSize({ length, thickness });
	reticleSprite_->Draw();

	// 【右線】
	reticleSprite_->SetPosition({ centerX + currentGap + length * 0.5f, centerY });
	reticleSprite_->SetSize({ length, thickness });
	reticleSprite_->Draw();

	// ---------------------------------------------------------
	// 2. 完全収束時（Max）に中心に描画される四角い照準
	// ---------------------------------------------------------
	if (isFullyFocused && config.reticleCenterDotSize > 0.0f)
	{
		float dotSize = config.reticleCenterDotSize;
		reticleSprite_->SetPosition({ centerX, centerY });
		reticleSprite_->SetSize({ dotSize, dotSize });
		reticleSprite_->Draw();
	}
}

void Player::OnShootRecoil()
{
	// 射撃した瞬間にフォーカスタイマーを大幅に下げる
	focusTimer_ *= 0.2f;
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

	// 武器描画
	if (weapon_)
	{
		weapon_->Draw();
	}

	DrawReticle();
}

void Player::DebugDraw()
{
#ifdef ENABLE_IMGUI
	ImGui::Begin("プレイヤー");

	binder_->DrawAnimationModel("PlayerModel", "プレイヤーインスペクター");

	ImGui::Separator();

	// 武器設定
	if (weapon_)
	{
		weapon_->DebugDraw();
	}

	if (ImGui::CollapsingHeader("動き"))
	{
		binder_->Draw("RunSpeed", "走り速度");
		binder_->Draw("RotationSpeed", "回転の速さ");
	}

	if (ImGui::CollapsingHeader("エイム・レティクル"))
	{
		binder_->Draw("AimMoveSpeed", "エイム時移動速度");
		binder_->Draw("AimToIdleBlendTime", "エイム補間時間(秒)");
		binder_->Draw("ShootRecoilTime", "射撃反動時間(秒)");

		ImGui::Separator();
		ImGui::Text("レティクル調整");

		binder_->Draw("ReticleLineThickness", "線の太さ");
		binder_->Draw("ReticleLineLength", "線の長さ");
		binder_->Draw("ReticleMaxGap", "最大広がり距離");
		binder_->Draw("ReticleMinGap", "最小収束距離");
		binder_->Draw("ReticleCenterDotSize", "完全収束時の中心四角サイズ");

		ImGui::Spacing();
		binder_->Draw("ReticleFocusTime", "フォーカス完了時間(秒)");
		binder_->Draw("MaxDamageMultiplier", "フォーカス時威力倍率");
		binder_->Draw("MaxBulletSpread", "最大弾道ブレ角");

		ImGui::Spacing();
		ImGui::ProgressBar(focusRatio_, ImVec2(-1, 0), "フォーカス率");
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