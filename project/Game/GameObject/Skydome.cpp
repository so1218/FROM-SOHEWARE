#include "Skydome.h"
#include "TextureHandle.h"
#include "ImGuiManager.h"
#include "ModelHandle.h"

// 初期化処理
void Skydome::Initialize(Engine* engine, Camera* camera)
{
	engine_ = engine;
	camera_ = camera;
	/*modelData_ = ModelHandle::Get(ModelID::skydome);*/
	// スケールと位置の調整
	worldTransform_.scale_ = { 1.0f, 1.0f, 1.0f };
	worldTransform_.translation_ = { 0.0f, 0.0f, 0.0f };

	uvTransform_.Initialize();
};

// 更新処理
void Skydome::Update()
{
	worldTransform_.UpdateMatrix();

	eulerAngles_ = { 0.0f, 0.0f, 0.0f }; // pitch, yaw, roll（XYZ順）

	// オイラー角からクォータニオンに変換して設定
	uvTransform_.rotationQuaternion_ = Quaternion::QuaternionFromEuler(eulerAngles_);

	uvTransform_.translation_.y += 0.001f; // 天球の高さを少しずつ上げる（例として） 

	// ワールド行列更新
	uvTransform_.UpdateMatrix();
};

// 描画処理
void Skydome::Draw()
{
	/*engine_->DrawModel(worldTransform_, *camera_, *modelData_, TextureHandle::Get(TextureID::skydome), 0xffffffff, uvTransform_);*/
};

// デバッグ描画処理
void Skydome::DebugDraw()
{
	// ImGui表示
	ImGui::Begin("天球");
	ImGui::DragFloat3("Translate", &uvTransform_.translation_.x, 0.1f, -100.0f, 100.0f);
	ImGui::DragFloat3("Rotate (Euler)", &eulerAngles_.x, 0.1f, -360.0f, 360.0f);
	ImGui::DragFloat3("Scale", &uvTransform_.scale_.x, 0.1f, -100.0f, 100.0f);
	ImGui::End();
	uvTransform_.UpdateMatrix();
}