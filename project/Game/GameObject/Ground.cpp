#include "Ground.h"
#include "TextureHandle.h"
#include "ModelHandle.h"
#include "ImGuiManager.h"

// 初期化処理
void Ground::Initialize(Engine* engine, Camera* camera)
{
	engine_ = engine;
	camera_ = camera;
	modelData_ = ModelHandle::Get(ModelID::field);
	// スケールと位置の調整
	worldTransform_.scale_ = { 1000.0f, 1.0f,1000.0f };
	worldTransform_.translation_ = { 0.0f, 0.0f, 0.0f };
	worldTransform_.UpdateMatrix();
};

// 更新処理
void Ground::Update()
{
	modelData_->materialHandle.materialData->isArtGrid = true;

	// staticで毎フレーム値を保持
	eulerAngles_ = { 0.0f, 0.0f, 0.0f }; // pitch, yaw, roll（XYZ順）

	// オイラー角からクォータニオンに変換して設定
	worldTransform_.rotationQuaternion_ = Quaternion::QuaternionFromEuler(eulerAngles_);

	// ワールド行列更新
	worldTransform_.UpdateMatrix();
};

// 描画処理
void Ground::Draw()
{
	engine_->DrawModel(worldTransform_, *camera_, *modelData_, TextureHandle::Get(TextureID::white1x1),0x777777ff);
};

// デバッグ描画処理
void Ground::DebugDraw()
{
	// ImGui表示
	ImGui::Begin("Ground");
	ImGui::DragFloat3("Translate", &worldTransform_.translation_.x, 0.1f, -100.0f, 100.0f);
	ImGui::DragFloat3("Rotate (Euler)", &eulerAngles_.x, 0.1f, -360.0f, 360.0f);
	ImGui::DragFloat3("Scale", &worldTransform_.scale_.x, 0.1f, -100.0f, 100.0f);
	ImGui::End();
}