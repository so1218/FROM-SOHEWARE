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
	animationPlayer_ = std::make_unique<AnimationModel>(engine_,camera_,*ModelHandle::Get(ModelID::walk),AnimationHandle::Get(AnimationID::walk));

	debugLine1_ = std::make_unique<Line>(engine_, camera_);
	debugLine1_->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f }); 

	debugLine2_ = std::make_unique<Line>(engine_, camera_);
	debugLine2_->SetColor({ 0.0f, 1.0f, 0.0f, 1.0f });
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

	animationPlayer_->SetColor(0xff0000ff);

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
	UpdateAABB();

	// アニメーション更新
	animationPlayer_->Update(1, true);
	animationPlayer_->SetTransform(modelPlayer_->GetTransform());

	// 所持武器の更新
	for (auto& weapon : weapons_)
	{
		weapon->Update(TimeManager::GetInstance()->GetDeltaTime());
	}

	animationPlayer_->SetEmissiveIntensity(3.5f);
	animationPlayer_->materialHandle_.materialData->enableRim = true;
	animationPlayer_->materialHandle_.materialData->rimUseLightDir = true;
	animationPlayer_->materialHandle_.materialData->rimColor = { 255.0f / 255.0f,237.0f / 255.0f,51.0f / 255.0f };
	animationPlayer_->materialHandle_.materialData->rimPower = 5.2f;
	animationPlayer_->materialHandle_.materialData->rimIntensity = 5.2f;

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

	}
	else
	{
		if (walkEmitterPtr_)
		{
			walkEmitterPtr_->Play();
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
	modelPlayer_->GetTransform().translation_.y = 0.5f; // 地面の高さを固定
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

void Player::UpdateAABB()
{
	Vector3 center = modelPlayer_->GetTransform().GetWorldPosition();
	float halfW = size_.x / 2.0f;
	float halfH = size_.y / 2.0f;
	float halfD = size_.z / 2.0f;

	aabb_.min = { center.x - halfW, center.y - halfH, center.z - halfD };
	aabb_.max = { center.x + halfW, center.y + halfH, center.z + halfD };
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
	modelPlayer_->Draw();
	modelTamesi_->Draw();
	animationPlayer_->Draw();

	debugLine1_->SetStart(modelPlayer_->GetTransform().translation_);
	debugLine1_->SetEnd({ 0.0f, 5.0f, 0.0f });
	debugLine1_->Draw();

	debugLine2_->SetStart(modelPlayer_->GetTransform().translation_);
	debugLine2_->SetEnd({ 0.0f, 10.0f, 0.0f });
	debugLine2_->Draw();

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