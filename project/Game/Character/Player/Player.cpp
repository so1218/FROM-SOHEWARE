#include "Player.h"
#include "MapChipField.h"
#include "CollisionConfig.h"
#include "PlayScene.h"
#include "GlobalVariables.h"
#include "TextureHandle.h"
#include "ModelHandle.h"
#include "AnimationHandle.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "MathUtils.h"  
#include "Collision.h"   
#include "TimeManager.h"
#include "WeaponKnife.h"
#include "WeaponAxe.h"
#include "AudioHandle.h"
#include "AudioPlayer.h"

#include <numbers>
#include <algorithm>

Player::Player(Engine* engine, Camera* camera)
{
	engine_ = engine;
	camera_ = camera;

	// 通常モデルを生成
	modelPlayer_ = std::make_unique<Model>(engine_, camera_, std::move(ModelHandle::Get(ModelID::cube)));

	modelTamesi_ = std::make_unique<Model>(engine_, camera_, std::move(ModelHandle::Get(ModelID::cube)));
	modelTamesi_->GetTransform().scale_.x = 500;
	modelTamesi_->GetTransform().scale_.z = 500;
	modelTamesi_->GetTransform().translation_.y = -0.5f;

	// アニメーションモデルを生成
	animationPlayer_ = std::make_unique<AnimationModel>(engine_,camera_,*ModelHandle::Get(ModelID::player),AnimationHandle::Get(AnimationID::player));
}

void Player::Initialize()
{
	// プレイヤーの基本情報を設定
	size_ = { 1.0f, 1.0f, 1.0f };
	moveDirection_ = { 0.0f, 0.0f, 0.0f };
	moveSpeed_ = 0.2f;

	// ステータス初期化
	level_ = 1;
	hp_ = maxHp_;
	experience_ = 0;
	isInvincible_ = false;
	invincibilityTimer_ = 0.0f;
	isEnd_ = false;

	modelPlayer_->SetEnableOutline(true);
	animationPlayer_->SetEnableOutline(true);

	animationPlayer_->SetColor(0x86FF30ff);

	modelTamesi_->SetColor(0x333333ff);

	// 衝突判定の属性設定
	SetCollisionAttribute(kCollisionAttributePlayer);
	SetCollisionMask(kCollisionAttributeEnemy | kCollisionAttributeExpGem);

	// デバッグ用のグローバル変数登録
	auto* gv = GlobalVariables::GetInstance();
	auto groupName = GetGlobalVariableGroupName(); 
	gv->CreateGroup(groupName);

	gv->AddItem(groupName, "Translation", modelPlayer_->GetTransform().translation_);
	gv->AddItem(groupName, "Scale", modelPlayer_->GetTransform().scale_);
	gv->AddItem(groupName, "Move Speed", moveSpeed_);
	gv->AddItem(groupName, "HP", hp_);
	gv->AddItem(groupName, "MaxHP", maxHp_);
	gv->AddItem(groupName, "Invincibility Duration", invincibilityDuration_);
	gv->AddItem(groupName, "Level", level_);
	gv->AddItem(groupName, "Experience", experience_);
	gv->AddItem(groupName, "XP to Next Level", xpToNextLevel_);

	ApplyGlobalVariables();
}

void Player::ApplyGlobalVariables()
{
	auto* gv = GlobalVariables::GetInstance();
	auto groupName = GetGlobalVariableGroupName();

	modelPlayer_->GetTransform().translation_ = gv->GetVector3Value(groupName, "Translation");
	modelPlayer_->GetTransform().scale_ = gv->GetVector3Value(groupName, "Scale");
	moveSpeed_ = gv->GetFloatValue(groupName, "Move Speed");
	hp_ = gv->GetFloatValue(groupName, "HP");
	maxHp_ = gv->GetFloatValue(groupName, "MaxHP");
	invincibilityDuration_ = gv->GetFloatValue(groupName, "Invincibility Duration");
	level_ = gv->GetIntValue(groupName, "Level");
	experience_ = gv->GetIntValue(groupName, "Experience");
	xpToNextLevel_ = gv->GetIntValue(groupName, "XP to Next Level");
}

// 武器を追加する処理
void Player::AddWeapon(WeaponType type)
{
	switch (type)
	{
	case WeaponType::Knife:
		// ナイフ武器を追加
		weapons_.push_back(std::make_unique<WeaponKnife>(engine_, this, camera_));
		break;

	case WeaponType::Axe:
		weapons_.push_back(std::make_unique<WeaponAxe>(engine_, this, camera_));
		break;

	default:
		break;
	}
}

// 更新処理
void Player::Update()
{
	animationPlayer_->SetDissolveTextureHandle(TextureHandle::Get(TextureID::noise1));
	//animationPlayer_->materialHandle_.materialData->edgeColor = { 1.0f, 0.2f, 0.1f };
	//animationPlayer_->materialHandle_.materialData->edgeIntensity = 5.0f;
	//animationPlayer_->materialHandle_.materialData->edgeWidth = 0.1f;
	//animationPlayer_->SetEnableDissolve(true);
	//animationPlayer_->materialHandle_.materialData->dissolveThreshold = 0.5f;

	float deltaTime = TimeManager::GetInstance()->GetDeltaTime();

	// 無敵時間の更新処理
	if (isInvincible_)
	{
		invincibilityTimer_ -= deltaTime;
		if (invincibilityTimer_ <= 0.0f)
		{
			isInvincible_ = false;
			// (モデルの色を元に戻す処理)
		}
	}

	// 移動処理
	Move();

	// モデルの行列更新
	modelPlayer_->GetTransform().UpdateMatrix();

	// アニメーション更新
	animationPlayer_->Update(1.5f, true);
	animationPlayer_->SetTransform(modelPlayer_->GetTransform());

	// 所持武器の更新
	for (auto& weapon : weapons_)
	{
		weapon->Update(TimeManager::GetInstance()->GetDeltaTime());
	}


	{
		// 0番目のディレクショナルライトを取得
		auto* dirLights = engine_->lightManager_->GetDirectionalLightData();
		if (dirLights[0].enable)
		{
			// ライト方向を正規化
			Vector3 lightDir = dirLights[0].direction;
			lightDir = lightDir.Normalize();

			// 影を落とす対象の中心座標
			Vector3 shadowTarget = modelPlayer_->GetTransform().translation_;

			// ライト位置を決定
			float distance = 100.0f;
			Vector3 lightPos = shadowTarget - (lightDir * distance);

			// 上方向ベクトル（真上/真下はX軸に変更）
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
			engine_->lightManager_->UpdateDirectionalLightShadowMatrix(0, lightViewProj);
		}
	}

	//animationPlayer_->SetEmissiveIntensity(3.5f);
	//animationPlayer_->materialHandle_.materialData->enableRim = true;
	//animationPlayer_->materialHandle_.materialData->rimUseLightDir = true;
	//animationPlayer_->materialHandle_.materialData->rimColor = { 255.0f / 255.0f,237.0f / 255.0f,51.0f / 255.0f };
	//animationPlayer_->materialHandle_.materialData->rimPower = 5.2f;
	//animationPlayer_->materialHandle_.materialData->rimIntensity = 5.2f;

}

void Player::AddWeaponColliders(CollisionManager* manager)
{
	for (auto& weapon : weapons_)
	{
		weapon->AddCollidersToManager(manager);
	}
}

void Player::Move()
{
	float deltaTime = TimeManager::GetInstance()->GetDeltaTime();

	moveDirection_ = GetMoveDirection();

	// 移動方向がある場合、最後の移動方向を更新
	if (moveDirection_.Length() > 0.0f)
	{
		lastMoveDirection_ = moveDirection_;
		if (currentAnimState_ != PlayerAnimState::Walk)
		{
			animationPlayer_->SetAnimation(AnimationHandle::Get(AnimationID::player));
			currentAnimState_ = PlayerAnimState::Walk; 
		}
	}
	else
	{
		if (walkEmitterPtr_)
		{
			walkEmitterPtr_->Play();
		}
		if (currentAnimState_ != PlayerAnimState::Idle)
		{
			animationPlayer_->SetAnimation(AnimationHandle::Get(AnimationID::playerIdle));
			currentAnimState_ = PlayerAnimState::Idle; 
		}
	}

	// 向きを補間して滑らかに回転させる
	if (lastMoveDirection_.Length() > 0.001f)
	{
		float targetAngleY = std::atan2(lastMoveDirection_.x, lastMoveDirection_.z);
		Quaternion targetRotation = Quaternion::QuaternionFromEuler({ 0.0f, targetAngleY, 0.0f });

		Quaternion currentRotation = modelPlayer_->GetTransform().rotationQuaternion_;
		float slerpFactor = Math::Clamp(rotationSpeed_ * deltaTime, 0.0f, 1.0f);
		Quaternion newRotation = Quaternion::Slerp(currentRotation, targetRotation, slerpFactor);

		modelPlayer_->GetTransform().rotationQuaternion_ = newRotation;
	}

	// 実際の位置更新
	modelPlayer_->GetTransform().translation_ += moveDirection_ * moveSpeed_;
	modelPlayer_->GetTransform().translation_.y = 0.1f; // 地面の高さを固定
	modelTamesi_->GetTransform().translation_ = modelPlayer_->GetTransform().translation_;
	modelTamesi_->GetTransform().translation_.y = -0.5f;
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

void Player::OnCollision(Collider* other)
{
	if (other->GetCollisionAttribute() & kCollisionAttributeEnemy)
	{
		TakeDamage(10.0f);
		followCamera_->StartShake(0.2f, 0.5f);
		Input::GetInstance().StartVibration(0, 0.3f, 0.3f, 0.3f);
		if (damagedEmitterPtr_)
		{
			damagedEmitterPtr_->Play();
		}
		AudioPlayer::GetInstance().Play(AudioHandle::Get(AudioID::playerHit), false, 100);
	}
	if (other->GetCollisionAttribute() & kCollisionAttributeExpGem)
	{
		GainExperience(5);
		followCamera_->StartShake(0.1f, 0.2f);
		Input::GetInstance().StartVibration(0, 0.15f, 0.15f, 0.15f);
		if (getExpEmitterPtr_)
		{
			getExpEmitterPtr_->Play();
		}
	}
}

Vector3 Player::GetWorldPosition()
{
	Vector3 worldPos;
	worldPos.x = modelPlayer_->GetTransform().matWorld_.m[3][0];
	worldPos.y = modelPlayer_->GetTransform().matWorld_.m[3][1];
	worldPos.z = modelPlayer_->GetTransform().matWorld_.m[3][2];
	return worldPos;
}

void Player::Draw()
{
	/*modelPlayer_->Draw();*/
	modelTamesi_->Draw();
	animationPlayer_->Draw();
	DrawCollider();

	for (auto& weapon : weapons_)
	{
		weapon->Draw();
	}
}

void Player::DebugDraw()
{
	ImGui::Begin("プレイヤー");

	auto* gv = GlobalVariables::GetInstance();
	auto groupName = GetGlobalVariableGroupName();
	bool changed = false;

	ImGui::Text("トランスフォーム");
	if (ImGui::DragFloat3("位置（Translation）", &modelPlayer_->GetTransform().translation_.x, 0.1f, -100.0f, 100.0f))
	{
		gv->SetValue(groupName, "Translation", modelPlayer_->GetTransform().translation_);
		changed = true;
	}
	if (ImGui::DragFloat3("スケール（Scale）", &modelPlayer_->GetTransform().scale_.x, 0.1f, 0.1f, 100.0f))
	{
		gv->SetValue(groupName, "Scale", modelPlayer_->GetTransform().scale_);
		changed = true;
	}
	ImGui::DragFloat3("tamesi位置（Translation）", &modelTamesi_->GetTransform().translation_.x, 0.1f, -100.0f, 100.0f);
	
	ImGui::DragFloat3("tamesiスケール（Scale）", &modelTamesi_->GetTransform().scale_.x, 0.1f, 0.1f, 100.0f);
	
	ImGui::Separator();

	ImGui::Text("ステータス");
	if (ImGui::DragFloat("移動速度", &moveSpeed_, 0.01f, 0.0f, 10.0f))
	{
		gv->SetValue(groupName, "Move Speed", moveSpeed_);
		changed = true;
	}
	if (ImGui::DragFloat("現在HP", &hp_, 1.0f, 0.0f, maxHp_))
	{
		gv->SetValue(groupName, "HP", hp_);
		changed = true;
	}
	if (ImGui::DragFloat("最大HP", &maxHp_, 1.0f, 1.0f, 1000.0f))
	{
		gv->SetValue(groupName, "MaxHP", maxHp_);
		changed = true;
	}

	ImGui::Separator();

	ImGui::Text("レベルと経験値");
	if (ImGui::DragInt("レベル", &level_, 1, 1, 99))
	{
		gv->SetValue(groupName, "Level", level_);
		changed = true;
	}
	if (ImGui::DragInt("経験値", &experience_, 1, 0, 10000))
	{
		gv->SetValue(groupName, "Experience", experience_);
		changed = true;
	}
	if (ImGui::DragInt("次のレベルまでの経験値", &xpToNextLevel_, 1, 1, 10000))
	{
		gv->SetValue(groupName, "XP to Next Level", xpToNextLevel_);
		changed = true;
	}

	ImGui::Separator();

	ImGui::Text("戦闘設定");
	if (ImGui::DragFloat("無敵時間（秒）", &invincibilityDuration_, 0.05f, 0.0f, 5.0f))
	{
		gv->SetValue(groupName, "Invincibility Duration", invincibilityDuration_);
		changed = true;
	}

	ImGui::Text("ディゾルブ設定 (Dissolve)");

	// データへのポインタを取得して記述を短くする
	auto* matData = animationPlayer_->materialHandle_.materialData;

	if (matData)
	{
		// 1. 有効/無効の切り替え (int <-> bool 変換)
		// シェーダー側が int なので、ImGui用の bool を噛ませる
		bool isDissolve = (matData->enableDissolve != 0);
		if (ImGui::Checkbox("有効化 (Enable)", &isDissolve))
		{
			animationPlayer_->SetEnableDissolve(isDissolve);
		}

		// 2. 閾値 (Threshold) - 0.0～1.0 でスライドさせる
		// これを動かしてモデルが消えたり現れたりするか確認してください
		ImGui::DragFloat("閾値 (Threshold)", &matData->dissolveThreshold, 0.01f, 0.0f, 1.0f);

		// 3. エッジの幅
		ImGui::DragFloat("エッジ幅 (Width)", &matData->edgeWidth, 0.001f, 0.0f, 0.5f);

		// 4. エッジの発光強度
		ImGui::DragFloat("発光強度 (Intensity)", &matData->edgeIntensity, 0.1f, 0.0f, 50.0f);

		// 5. エッジの色 (ColorEdit3 を使うとカラーピッカーが出る)
		ImGui::ColorEdit3("エッジ色 (Color)", &matData->edgeColor.x);
	}

	if (changed)
	{
		ApplyGlobalVariables();
	}

	ImGui::End();

	// 所持武器のDebugDraw
	for (auto& weapon : weapons_)
	{
		weapon->DebugDraw();
	}
}

void Player::TakeDamage(float damage)
{
	// 無敵時間中はダメージを受けない
	if (isInvincible_) 
	{
		return;
	}

	hp_ -= damage;
	if (hp_ <= 0.0f)
	{
		hp_ = 0.0f;
		isEnd_ = true;
	}

	// ダメージを受けたら無敵時間を開始
	isInvincible_ = true;
	invincibilityTimer_ = invincibilityDuration_;

}

void Player::GainExperience(int amount)
{
	experience_ += amount;

	// 経験値が次のレベルに達したら、達しなくなるまでレベルアップ処理を繰り返す
	while (experience_ >= xpToNextLevel_)
	{
		LevelUp();
	}
}

bool Player::HasWeapon(WeaponType type)const
{
	for (const auto& weapon : weapons_)
	{
		if (weapon->GetType() == type)
		{
			return true;
		}
	}
	return false;
}

void Player::LevelUp()
{
	// レベル自体の数値を上げる
	level_++;
	experience_ -= xpToNextLevel_; 
	xpToNextLevel_ = static_cast<int>(xpToNextLevel_ * 1.2f);

	Input::GetInstance().StartVibration(0, 0.3f, 0.3f, 0.3f);
	if (levelUpEmitterPtr_)
	{
		levelUpEmitterPtr_->Play();
	}
	AudioPlayer::GetInstance().PlayUnique(AudioHandle::Get(AudioID::levelUp), false, 100);
	// ここでレベルアップ選択画面を開く
	isWaitingForUpgrade_ = true;

	/*if (!weapons_.empty())
	{
		weapons_[0]->LevelUp();
		weapons_[1]->LevelUp();
	}*/
}

void Player::ApplyUpgrade(const UpgradeInfo& upgrade)
{
	switch (upgrade.type)
	{
	case UpgradeType::newWeapon:
		AddWeapon(static_cast<WeaponType>(upgrade.weaponId));
		break;

	case UpgradeType::UpgradeWeapon:
		// 所持している該当武器をレベルアップ
		for (auto& weapon : weapons_)
		{
			if (weapon->GetType() == static_cast<WeaponType>(upgrade.weaponId))
			{
				weapon->LevelUp();
				break;
			}
		}
		break;

	case UpgradeType::passiveUp:
		// ステータス強化
		if (upgrade.name == "MaxHp Up")
		{
			maxHp_ += upgrade.value;
		}
		else if (upgrade.name == "Speed Up")
		{
			moveSpeed_ += upgrade.value;
		}
		break;

	case UpgradeType::heal:
		hp_ = Math::MyMin(hp_ + upgrade.value, maxHp_);
		break;
	}
}

float Player::GetHpRatio() const { return hp_ / maxHp_; }
float Player::GetXpRatio() const
{
	if (xpToNextLevel_ <= 0) return 0.0f;
	return static_cast<float>(experience_) / static_cast<float>(xpToNextLevel_);
}