#include "pch.h"
#include "EnvironmentProp.h"
#include "GameDefine.h"

using namespace FE;

EnvironmentProp::EnvironmentProp(FE::Engine* engine, int id, const std::string& parentGroupName)
    : engine_(engine), id_(id), parentGroupName_(parentGroupName)
{
    // 初期状態として仮のモデル（cubeなど）を生成しておく（BindModelで上書きされます）
    model_ = std::make_unique<FE::Model>(engine_, "cube");
}

EnvironmentProp::~EnvironmentProp()
{
    // ★重要: シーン切り替えやオブジェクト破棄の際、ライトを借りていればLightManagerに確実に返却する
    if (pointLightIndex_ != -1)
    {
        engine_->GetLightManager()->ReturnPointLight(pointLightIndex_);
        pointLightIndex_ = -1;
    }

    // ★追加: 自身が破棄されるなら、パーティクルも安全に破棄する
    if (activeEmitter_)
    {
        activeEmitter_->Destroy();
        activeEmitter_ = nullptr;
    }
}

void EnvironmentProp::Initialize()
{
    std::string childGroupName = "Prop_" + std::to_string(id_);
    binder_ = std::make_unique<FE::PropertyBinder>(engine_, parentGroupName_, childGroupName);

    // 最初に「モデル名」だけをバインドする
    // モデル名が変わったら、即座に変えずに「次のフレームで再構築してね」とフラグを立てる
    binder_->BindModelName("ModelName", &modelName_, "cube", [this](const std::string& newName) {
        isNeedReconstruct_ = true;
        });

    // 初回のプロパティ構築
    SetupProperties();

    // ロードされた初期設定を適用（ライトの要求やコライダーの登録を行う）
    ApplySettings();
}

void EnvironmentProp::SetupProperties()
{
    // ★追加: 予期せぬ2重生成を防ぐため、モデル再構築の際も古いエミッターは確実に破棄する
    if (activeEmitter_)
    {
        activeEmitter_->Destroy();
        activeEmitter_ = nullptr;
    }

    // 1. 一度バインダーに登録された古いメモリ番地（ポインタ）をすべてリセット
    binder_->Clear();

    // ※モデル名選択のUIだけは消えてほしくないので再バインド
    binder_->BindModelName("ModelName", &modelName_, "cube", [this](const std::string& newName) {
        isNeedReconstruct_ = true;
        });

    auto* gv = FE::GlobalVariables::GetInstance();
    std::string loadedCustomName = gv->GetStringValue(binder_->GetGroupPath(), "CustomName");
    // もしJSONにデータがあればそれを使い、無ければ空文字をJSONにセット
    if (!loadedCustomName.empty()) {
        propCustomName_ = loadedCustomName;
    }
    else {
        gv->SetValue(binder_->GetGroupPath(), "CustomName", propCustomName_);
    }

    // 2. 新しいモデルを完全に作り直す
    model_ = std::make_unique<FE::Model>(engine_, modelName_);

    // 3. 新しく生成された正しいメモリ番地で再バインドする
    binder_->BindModel ("Model", model_.get());
    binder_->Bind("Behavior", &propBehavior_, 0);
    binder_->Bind("HasCollider", &hasCollider_, true);

    binder_->Bind("ColliderType", &colliderType_, 0);
    binder_->Bind("ColliderRadius", &colliderRadius_, 1.0f);
    binder_->Bind("ColliderSize", &colliderSize_, { 0.5f, 0.5f, 0.5f });
    binder_->Bind("ColliderOffset", &colliderOffset_, { 0.0f, 0.0f, 0.0f });

    binder_->Bind("HasLight", &hasLight_, false);
    binder_->BindColor("LightColor", &lightColor_, { 1.0f, 0.5f, 0.0f, 1.0f });
    binder_->Bind("LightIntensity", &lightIntensity_, 5.0f);
    binder_->Bind("LightRadius", &lightRadius_, 10.0f);
    binder_->Bind("LightVolumetricScatteringIntensity", &lightVolumetricScatteringIntensity_, 1.0f);

    // パーティクルのON/OFFフラグと追従フラグ
    binder_->Bind("HasParticle", &hasParticle_, false);
    binder_->Bind("IsParticleFollowing", &isParticleFollowing_, true); // ★ここでのバインドはOK

    // JSONからパーティクル名を読み込む;
    std::string loadedParticle = gv->GetStringValue(binder_->GetGroupPath(), "ParticleName");
    if (!loadedParticle.empty()) {
        particleName_ = loadedParticle;
    }
    else {
        gv->SetValue(binder_->GetGroupPath(), "ParticleName", particleName_);
    }

    // ライトやコライダーの再適用
    ApplySettings();
}

void EnvironmentProp::ApplySettings()
{
    // 1. コライダーのリアルタイムON/OFF制御
    if (hasCollider_) {
        if (!collider_) collider_ = std::make_unique<FE::Collider>(this);
        collider_->RegisterToManager();
    }
    else {
        if (collider_) collider_.reset(); // 不要ならメモリ解放
    }

    // 2. ライトのリアルタイムON/OFF制御
    if (hasLight_) {
        // ライトが必要かつ、まだ要求していなければ要求する
        if (pointLightIndex_ == -1) {
            pointLightIndex_ = engine_->GetLightManager()->RequestPointLight();
        }

        // 要求に成功していれば、現在のパラメータを即座に適用
        if (pointLightIndex_ != -1) {
            engine_->GetLightManager()->UpdatePointLightProperties(
                pointLightIndex_,
                lightColor_,
                lightIntensity_,
                lightRadius_,
                lightVolumetricScatteringIntensity_
            );
        }
    }
    else {
        // ライトが不要になった、またはチェックが外されたら即座に返却
        if (pointLightIndex_ != -1) {
            engine_->GetLightManager()->ReturnPointLight(pointLightIndex_);
            pointLightIndex_ = -1;
        }
    }

    // 3. パーティクルのリアルタイムON/OFF制御
    if (hasParticle_)
    {
        if (!activeEmitter_)
        {
            auto emitter = engine_->GetParticleSystem()->CreateEmitter(particleName_);
            if (emitter)
            {
                // ★修正: 追従フラグによって処理を分岐
                if (isParticleFollowing_)
                {
                    emitter->SetTargetToFollow(const_cast<FE::WorldTransform*>(&model_->GetTransform()));
                }
                else
                {
                    // 追従しない場合は、その瞬間のオブジェクトの位置に座標を固定する
                    emitter->SetPosition(model_->GetTransform().translation_);
                }

                // 生ポインタを保存してからSystemに所有権を渡す
                activeEmitter_ = emitter.get();
                engine_->GetParticleSystem()->AddEmitter(std::move(emitter));
            }
        }
    }
    else
    {
        if (activeEmitter_)
        {
            // 不要になったらDestroyを呼ぶ
            activeEmitter_->Destroy();
            activeEmitter_ = nullptr;
        }
    }
}

void EnvironmentProp::Update()
{
    if (!IsActive()) return;

    // ★ 冒頭でチェック：モデルの変更要求が来ていたら、安全なタイミングで再構築する
    if (isNeedReconstruct_)
    {
        SetupProperties();
        isNeedReconstruct_ = false;
    }

    // コライダーの同期
    if (hasCollider_ && collider_)
    {
        collider_->SetType(static_cast<FE::CollisionShapeType>(colliderType_));
        collider_->SetRadius(colliderRadius_);
        collider_->SetSize(colliderSize_);
        collider_->SetCenterOffset(colliderOffset_);
    }

    // ライトの追従処理
    if (hasLight_ && pointLightIndex_ != -1)
    {
        FE::Vector3 currentPos = model_->GetTransform().translation_;
        engine_->GetLightManager()->UpdatePointLightPosition(pointLightIndex_, currentPos);
        engine_->GetLightManager()->UpdatePointLightProperties(
            pointLightIndex_, lightColor_, lightIntensity_, lightRadius_, lightVolumetricScatteringIntensity_
        );
    }

    SetTransform(model_->GetTransform());
}
void EnvironmentProp::Draw()
{
    if (!IsActive()) return;

    if (model_)
    {
        model_->Draw();
    }

    if (hasCollider_ && collider_)
    {
        collider_->DrawCollider();
    }
}

void EnvironmentProp::DebugDraw()
{
#ifdef IS_DEVELOPMENT
    ImGui::PushID(id_);

    std::string label = "オブジェクト [" + std::to_string(id_) + "] の設定";
    if (ImGui::CollapsingHeader(label.c_str()))
    {
        binder_->Draw("ModelName", "3Dモデルのアセット選択");
        binder_->DrawModel("Model", "インスペクター");

        ImGui::Combo("接触時の挙動", &propBehavior_, "なし（通常の障害物）\0拾って消える（アイテム）\0");

        // 1. 各種変更検知用のフラグをローカルに保存
        bool prevCollider = hasCollider_;
        bool prevLight = hasLight_;
        bool prevParticle = hasParticle_;
        bool prevParticleFollow = isParticleFollowing_; // ★追加

        binder_->Draw("HasCollider", "当たり判定（コライダー）");

        if (hasCollider_)
        {
            ImGui::Indent();
            ImGui::Combo("形状", &colliderType_, "Sphere (球)\0AABB (ボックス)\0");
            if (colliderType_ == 0) binder_->Draw("ColliderRadius", "半径 (Radius)");
            else binder_->Draw("ColliderSize", "サイズ (Half Size)");
            binder_->Draw("ColliderOffset", "中心オフセット");
            ImGui::Unindent();
        }

        binder_->Draw("HasLight", "ポイントライトを有効にする");

        if (hasLight_)
        {
            ImGui::Indent();
            binder_->Draw("LightColor", "光の色");
            binder_->Draw("LightIntensity", "輝度（明るさ）");
            binder_->Draw("LightRadius", "光源の届く半径");
            binder_->Draw("LightVolumetricScatteringIntensity", "フォグへの影響度");
            ImGui::Unindent();
        }

        // --- パーティクル設定セクション ---
        binder_->Draw("HasParticle", "パーティクルを発生させる");

        if (hasParticle_)
        {
            ImGui::Indent();
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 1.0f, 1.0f), "パーティクルの詳細設定");

            binder_->Draw("IsParticleFollowing", "モデルに追従させる");

            char nameBuf[256];
            strncpy_s(nameBuf, sizeof(nameBuf), particleName_.c_str(), _TRUNCATE);

            // 1. Enterフラグを外し、入力されるたびに文字列とJSON「だけ」を更新する（軽い処理）
            if (ImGui::InputText("エフェクト名", nameBuf, sizeof(nameBuf)))
            {
                particleName_ = nameBuf;
                FE::GlobalVariables::GetInstance()->SetValue(binder_->GetGroupPath(), "ParticleName", particleName_);
            }

            // 2. Enterキーを押した、または別の場所をクリックして入力が【確定】したかを検知
            bool applyRequested = ImGui::IsItemDeactivatedAfterEdit();

            ImGui::SameLine();

            // 3. 「適用」ボタンが押された場合も確定扱いにする
            if (ImGui::Button("適用##ApplyParticle"))
            {
                applyRequested = true;
            }

            // 4. 確定アクションがあった瞬間だけ、古いパーティクルを壊して再生成する（重い処理）
            if (applyRequested)
            {
                // ここには particleName_ = nameBuf; を書かなくてOK（上で既に更新されているため）
                if (activeEmitter_) {
                    activeEmitter_->Destroy();
                    activeEmitter_ = nullptr;
                }
                ApplySettings();
            }
            ImGui::Unindent();
        }
    }

    ImGui::PopID();
#endif
}

void EnvironmentProp::OnCollisionEnter(FE::Collider* mine, FE::Collider* other)
{
    if (!IsActive()) return;

    FE::GameObject* hitObj = other->GetOwner();
    if (hitObj && hitObj->CompareTag(ObjectTag::Player))
    {

    }
}

void EnvironmentProp::OnCollisionStay(FE::Collider* mine, FE::Collider* other)
{
    if (!IsActive()) return;

    FE::GameObject* hitObj = other->GetOwner();
    if (hitObj && hitObj->CompareTag(ObjectTag::Player))
    {
        // アイテムのように拾って消える挙動の場合
        if (propBehavior_ == static_cast<int>(PropBehavior::Disappear))
        {
            SetActive(false); // 非アクティブにして描画と更新を止める

            // オブジェクトが消えたので、ライトの輝度を即座に0にして消灯する
            if (pointLightIndex_ != -1)
            {
                engine_->GetLightManager()->UpdatePointLightProperties(
                    pointLightIndex_, lightColor_, 0.0f, 0.0f, 0.0f
                );
            }
        }
    }
}

void EnvironmentProp::OnCollisionExit(FE::Collider* mine, FE::Collider* other)
{
    if (!IsActive()) return;

    FE::GameObject* hitObj = other->GetOwner();
    if (hitObj && hitObj->CompareTag(ObjectTag::Player))
    {
    }
}

void EnvironmentProp::ReassignID(int newID)
{
    id_ = newID;
    std::string childGroupName = "Prop_" + std::to_string(id_);

    // バインダーを新しいパスで作り直す
    binder_ = std::make_unique<FE::PropertyBinder>(engine_, parentGroupName_, childGroupName);

    // プロパティを新しいバインダーに登録し直す
    SetupProperties();

    // 新しい状態としてJSONに強制上書き保存
    FE::GlobalVariables::GetInstance()->SaveFile(binder_->GetGroupPath());
}
