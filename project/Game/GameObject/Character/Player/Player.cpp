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
#include "Enemy.h"

using namespace FE;

Player::Player(Engine* engine, Camera* camera) : GameObject(),
	camera_(camera)
{
	SetTag(ObjectTag::Player);

	engine_ = engine;

	// アニメーションモデルを生成
	animationModel_ = std::make_unique<AnimationModel>(engine_, "humanMesh", "humanRun");
	weaponModel_ = std::make_unique<Model>(engine_, "handGun_01");

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

	reticleSprite_ = std::make_unique<FE::Sprite>(engine_);
	reticleSprite_->SetTexture("white1x1"); 
	reticleSprite_->SetAnchorPoint({ 0.5f, 0.5f }); 
	reticleSprite_->SetIsVisible(true);

	// マズルフラッシュ用のポイントライトを1つ確保
	muzzleLightIndex_ = engine_->GetLightManager()->RequestPointLight();
	if (muzzleLightIndex_ >= 0)
	{
		// 初期状態は消灯（Intensity = 0）
		engine_->GetLightManager()->UpdatePointLightProperties(
			muzzleLightIndex_, config.muzzleFlashColor, 0.0f, config.muzzleFlashRadius, 0.0f
		);
	}

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

	binder_->Bind("Muzzle Flash Color", &config.muzzleFlashColor, { 1.0f, 0.75f, 0.3f, 1.0f });
	binder_->Bind("Muzzle Flash Intensity", &config.muzzleFlashIntensity, 25.0f, 0.5f, 0.0f, 100.0f);
	binder_->Bind("Muzzle Flash Radius", &config.muzzleFlashRadius, 8.0f, 0.1f, 0.5f, 30.0f);
	binder_->Bind("Muzzle Flash Duration", &config.muzzleFlashDuration, 0.05f, 0.005f, 0.01f, 0.2f);
	binder_->Bind("Muzzle Offset", &config.muzzleOffset, { 0.0f, 0.05f, 0.35f });

	stateMachine_ = std::make_unique<StateMachine<Player>>(this);
	stateMachine_->ChangeState(PlayerStateNormal::GetInstance());

	collider_->RegisterToManager();

	auraEmitter_ = engine_->GetParticleSystem()->CreateEmitter("playerAura");
	auraEmitter_->SetTargetToFollow(&animationModel_->GetTransform());
	engine_->GetParticleSystem()->AddEmitter(std::move(auraEmitter_));

	auto muzzleParticle = engine_->GetParticleSystem()->CreateEmitter("muzzleFlash");
	if (muzzleParticle)
	{
		muzzleFlashEmitterPtr_ = muzzleParticle.get(); // 生ポインタを保持しておく
		engine_->GetParticleSystem()->AddEmitter(std::move(muzzleParticle)); // 所有権を渡す
	}

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

	if (weaponModel_)
	{
		weaponModel_->GetTransform().UpdateMatrix();
	}

	if (muzzleFlashEmitterPtr_)
	{
		// 銃口の最新ワールド座標をセット
		muzzleFlashEmitterPtr_->SetPosition(GetMuzzleWorldPosition());

		// 射撃方向（カメラの前方）に合わせて回転をセット
		if (camera_)
		{
			Vector3 forward = camera_->GetWorldTransform().rotationQuaternion_.RotateVector({ 0.0f, 0.0f, 1.0f }).Normalize();
			Quaternion rot = Quaternion::LookRotation(forward, { 0.0f, 1.0f, 0.0f });
			muzzleFlashEmitterPtr_->SetRotation(rot);
		}
	}

	// ---------------------------------------------------------
	// ★ マズルフラッシュ（ポイントライト）の更新
	// ---------------------------------------------------------
	if (muzzleLightIndex_ >= 0)
	{
		// 毎フレーム最新の銃口位置を計算
		Vector3 muzzlePos = GetMuzzleWorldPosition();

		if (muzzleFlashTimer_ > 0.0f)
		{
			muzzleFlashTimer_ -= deltaTime;

			// ライト位置を更新
			engine_->GetLightManager()->UpdatePointLightPosition(muzzleLightIndex_, muzzlePos);

			float alpha = std::clamp(muzzleFlashTimer_ / config.muzzleFlashDuration, 0.0f, 1.0f);
			float currentIntensity = config.muzzleFlashIntensity * alpha;

			engine_->GetLightManager()->UpdatePointLightProperties(
				muzzleLightIndex_,
				config.muzzleFlashColor,
				currentIntensity,
				config.muzzleFlashRadius,
				1.0f
			);
		}
		else
		{
			// 常時位置だけ更新しておき、消灯状態にする
			engine_->GetLightManager()->UpdatePointLightPosition(muzzleLightIndex_, muzzlePos);
			engine_->GetLightManager()->UpdatePointLightProperties(
				muzzleLightIndex_,
				config.muzzleFlashColor,
				0.0f,
				config.muzzleFlashRadius,
				0.0f
			);
		}
	}

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
	if (!camera_) return;

	// 1. 発光タイマーのセット（ライト）
	muzzleFlashTimer_ = config.muzzleFlashDuration;

	if (muzzleFlashEmitterPtr_)
	{
		muzzleFlashEmitterPtr_->Play();
	}

	// 2. 銃口のワールド座標を取得
	Vector3 muzzlePos = GetMuzzleWorldPosition();

	// ---------------------------------------------------------
	// レイキャスト・ダメージ・反動処理（既存コード）
	// ---------------------------------------------------------
	Vector3 rayStart = camera_->GetWorldTransform().translation_;
	Vector3 baseForward = camera_->GetWorldTransform().rotationQuaternion_.RotateVector({ 0.0f, 0.0f, 1.0f }).Normalize();

	float currentSpread = config.maxBulletSpread * (1.0f - focusRatio_);
	float randPitch = (((float)rand() / RAND_MAX) * 2.0f - 1.0f) * currentSpread;
	float randYaw = (((float)rand() / RAND_MAX) * 2.0f - 1.0f) * currentSpread;

	Quaternion spreadRot = Quaternion::QuaternionFromEuler({ randPitch, randYaw, 0.0f });
	Vector3 finalRayDir = spreadRot.RotateVector(baseForward).Normalize();

	int baseDamage = 20;
	float damageMult = 1.0f + (config.maxDamageMultiplier - 1.0f) * focusRatio_;
	int finalDamage = static_cast<int>(baseDamage * damageMult);

	CollisionManager* colManager = GetManager() ? GetManager()->GetCollisionManager() : nullptr;
	if (!colManager) return;

	RaycastHit hitInfo;
	float maxDistance = 150.0f;
	uint32_t targetMask = kCollisionAttributeEnemy | kCollisionAttributeProp;

	if (followCamera_)
	{
		followCamera_->AddRecoil(0.03f, 0.01f);
	}

	OnShootRecoil();

	if (colManager->Raycast(rayStart, finalRayDir, maxDistance, &hitInfo, targetMask))
	{
		if (hitInfo.hitObject)
		{
			if (hitInfo.hitObject->CompareTag(ObjectTag::Enemy))
			{
				Enemy* enemy = static_cast<Enemy*>(hitInfo.hitObject);
				if (enemy)
				{
					enemy->TakeDamage(finalDamage, hitInfo.point, hitInfo.normal);
				}
			}
		}
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

Vector3 Player::GetMuzzleWorldPosition() const
{
	// 右手ではなく、銃モデル本体のワールド行列を基準にオフセットを適用する
	if (weaponModel_)
	{
		const Matrix4x4& weaponMat = weaponModel_->GetTransform().matWorld_;
		return weaponMat.TransformPoint(config.muzzleOffset);
	}

	const Matrix4x4& handMat = rightHandTransform_.matWorld_;
	return handMat.TransformPoint(config.muzzleOffset);
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

	DrawReticle();
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

	if (ImGui::CollapsingHeader("エイム・射撃・レティクル"))
	{
		binder_->Draw("AimMoveSpeed", "エイム時移動速度");
		binder_->Draw("AimToIdleBlendTime", "エイム補間時間(秒)");
		binder_->Draw("ShootRecoilTime", "射撃反動時間(秒)");

		ImGui::Separator();
		ImGui::Text("マズルフラッシュ設定");
		binder_->Draw("Muzzle Flash Color", "発光色");
		binder_->Draw("Muzzle Flash Intensity", "発光強度");
		binder_->Draw("Muzzle Flash Radius", "照射半径");
		binder_->Draw("Muzzle Flash Duration", "発光時間(秒)");
		binder_->Draw("Muzzle Offset", "銃口位置オフセット");

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

		// 現在のフォーカス率の可視化
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
