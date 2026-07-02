#pragma once
#include "GlobalVariables.h"
#include "ImGuiManager.h"
#include "TextureManager.h"
#include "MathUtils.h"
#include "Structures.h"
#include "ModelManager.h"

namespace FE
{

class Model;
class AnimationModel;
class Sprite;
class Engine;

// 変数の登録を行えば、GlobalVariablesの読み書きとImGuiの表示をしてくれる関数
class PropertyBinder
{
public:
    // 可変長テンプレートコンストラクタ
    template <typename... Args>
    PropertyBinder(Engine* engine, Args&&... args)
        : engine_(engine),
        groupPath_{ std::string(std::forward<Args>(args))... }
    {
        GlobalVariables::GetInstance()->CreateGroup(groupPath_);
    }

    // 汎用Bind関数
    void Bind(const std::string& key, int32_t* ptr, int32_t defaultValue, float speed = 1.0f, int32_t min = 0, int32_t max = 0);
    void Bind(const std::string& key, uint32_t* ptr, uint32_t defaultValue, float speed = 1.0f, uint32_t min = 0, uint32_t max = 0);
    void Bind(const std::string& key, float* ptr, float defaultValue, float speed = 0.01f, float min = 0.0f, float max = 0.0f);
    void Bind(const std::string& key, bool* ptr, bool defaultValue);
    void Bind(const std::string& key, Vector2* ptr, const Vector2& defaultValue, float speed = 0.01f, float min = 0.0f, float max = 0.0f);
    void Bind(const std::string& key, Vector3* ptr, const Vector3& defaultValue, float speed = 0.01f, float min = 0.0f, float max = 0.0f);
    // コールバックを受け取る版のVector3 
    void Bind(const std::string& key, Vector3* ptr, const Vector3& defaultValue, float speed, float min, float max, std::function<void()> onChange);
    // 省略版コールバック
    void Bind(const std::string& key, Vector3* ptr, const Vector3& defaultValue, float speed, std::function<void()> onChange);
    void Bind(const std::string& key, Vector4* ptr, const Vector4& defaultValue, float speed = 0.01f, float min = 0.0f, float max = 0.0f);

    // 色用Bind関数
    void BindColor(const std::string& key, Vector3* ptr, const Vector3& defaultValue);
    void BindColor(const std::string& key, Vector4* ptr, const Vector4& defaultValue);
    void BindColor(const std::string& key, uint32_t* ptr, uint32_t defaultValue);

    void Draw(const std::string& key, const std::string& name = "");

    // 特殊系
    // インスペクターのようなもの
    void BindModel(const std::string& groupName, Model* model);
    void DrawModel(const std::string& groupName, const std::string& customLabel = "");
    void BindAnimationModel(const std::string& groupName, AnimationModel* model);
    void DrawAnimationModel(const std::string& groupName, const std::string& customLabel = "");
    void BindSprite(const std::string& groupName, Sprite* sprite);
    void DrawSprite(const std::string& groupName, const std::string& customLabel = "");

    // int/uintをboolとして扱うための関数
    void BindBool(const std::string& key, int32_t* ptr, bool defaultValue);
    void BindBool(const std::string& key, uint32_t* ptr, bool defaultValue);

    void BindCombo(const std::string& key, int32_t* ptr, int32_t defaultValue, const char* items);

    // 回転専用のBind関数
    void BindRotation(const std::string& key, Vector3* eulerPtr, Quaternion* quatPtr, float speed = 0.01f, std::function<void()> onChange = nullptr);

    void Clear(bool eraseData = false);

    const std::vector<std::string>& GetGroupPath() const { return groupPath_; }

    void BindModelName(
        const std::string& key,
        std::string* currentModelName,
        const std::string& defaultName,
        std::function<void(const std::string&)> onChange)
    {
        auto* gv = GlobalVariables::GetInstance();

        // GlobalVariablesから保存されたモデル名を取得
        std::string loadedName = gv->GetStringValue(groupPath_, key);
        if (!loadedName.empty()) {
            *currentModelName = loadedName;
        }
        else {
            *currentModelName = defaultName;
            gv->SetValue(groupPath_, key, defaultName);
        }

        keys_.push_back(key);
        items_[key] = [this, currentModelName, onChange, key](const std::string& label)
            {
                // ModelManagerからロード済みのモデル名一覧を自動取得
                std::vector<std::string> modelNames = ModelManager::GetInstance().GetLoadedModelNames();
                if (modelNames.empty()) return;

                // 現在のモデル名が何番目にあるか検索
                int currentIndex = 0;
                for (int i = 0; i < modelNames.size(); ++i) {
                    if (modelNames[i] == *currentModelName) {
                        currentIndex = i;
                        break;
                    }
                }

                // ImGui用の文字列ポインタ配列を作成
                std::vector<const char*> items(modelNames.size());
                for (size_t i = 0; i < modelNames.size(); ++i) {
                    items[i] = modelNames[i].c_str();
                }

                std::string displayLabel = label.empty() ? key : label;
                if (ImGui::Combo(displayLabel.c_str(), &currentIndex, items.data(), static_cast<int>(items.size())))
                {
                    // 変更されたら変数を更新し、JSONへ保存
                    *currentModelName = modelNames[currentIndex];
                    GlobalVariables::GetInstance()->SetValue(groupPath_, key, *currentModelName);

                    // 通知コールバックを発火
                    if (onChange) {
                        onChange(*currentModelName);
                    }
                }
            };
    }

private:
    // 共通処理
    template<typename T>
    void RegisterItem(const std::string& key, const T& defaultValue, T* ptr);

    // 共通実装
    void BindVector3Internal(const std::string& key, Vector3* ptr, const Vector3& defaultValue, float speed, float min, float max, std::function<void()> onChange);

    void BindTexture(
        const std::string& key,
        const std::string& initialValue,
        std::function<void(const std::string&)> onValueChanged, // 変更時のコールバック
        const std::string& defaultName = "white1x1",
        TextureType filterType = TextureType::Albedo
    );

    // テクスチャID用バインド関数
    void BindTexture(const std::string& key, std::string* currentTextureName, uint32_t* currentHandlePtr, const std::string& defaultName, TextureType filterType = TextureType::Albedo);

    void BindMaterialProperties(const std::string& prefix, MaterialHandle* handle);

    template <typename ModelType>
    void DrawMaterialUI(ModelType* targetModel, const std::string& prefix, GlobalVariables* gv, const std::vector<std::string>& groupPath);

    Engine* engine_ = nullptr;

    std::vector<std::string> groupPath_;

    // 登録順序を保持するリスト
    std::vector<std::string> keys_;

    // キーと描画処理を紐付けるマップ
    std::unordered_map<std::string, std::function<void(const std::string&)>> items_;

    // 内部で管理するためのヘルパー
    struct ModelBindInfo
    {
        Model* model = nullptr;
    };
    std::unordered_map<std::string, ModelBindInfo> modelBindMap_;

    struct AnimationBindInfo
    {
        AnimationModel* model = nullptr;
    };
    std::unordered_map<std::string, AnimationBindInfo> animationBindMap_;
};

}

