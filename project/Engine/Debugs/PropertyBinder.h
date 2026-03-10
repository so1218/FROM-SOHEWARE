#pragma once
#include "GlobalVariables.h"
#include "ImGuiManager.h"
#include "TextureManager.h"
#include "MathUtils.h"
#include "Structures.h"

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

    //  汎用Bind関数
    template <typename T>
    void Bind(const std::string& key, T* ptr, const T& defaultValue, float speed = -1.0f, float min = 0.0f, float max = 0.0f)
    {
        // 実際に適用するスピード変数
        float appliedSpeed = speed;

        // 引数が省略された場合、型に応じたデフォルト値を設定
        if (appliedSpeed <= 0.0f)
        {
            if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>)
            {
                appliedSpeed = 1.0f;
            }
            else
            {
                appliedSpeed = 0.01f;
            }
        }

        // int32_t 
        if constexpr (std::is_same_v<T, int32_t>)
        {
            BindInt(key, ptr, defaultValue, appliedSpeed, static_cast<int32_t>(min), static_cast<int32_t>(max));
        }
        // uint32_t
        else if constexpr (std::is_same_v<T, uint32_t>)
        {
            BindUint(key, ptr, defaultValue, appliedSpeed, static_cast<uint32_t>(min), static_cast<uint32_t>(max));
        }
        // float
        else if constexpr (std::is_same_v<T, float>)
        {
            BindFloat(key, ptr, defaultValue, appliedSpeed, min, max);
        }
        // bool 
        else if constexpr (std::is_same_v<T, bool>)
        {
            BindBool(key, ptr, defaultValue);
        }
        // Vector2
        else if constexpr (std::is_same_v<T, Vector2>)
        {
            BindVector2(key, ptr, defaultValue, appliedSpeed, min, max);
        }
        // Vector3
        else if constexpr (std::is_same_v<T, Vector3>)
        {
            BindVector3(key, ptr, defaultValue, appliedSpeed, min, max);
        }
        // Vector4
        else if constexpr (std::is_same_v<T, Vector4>)
        {
            BindVector4(key, ptr, defaultValue, appliedSpeed, min, max);
        }
    }

    //  色用Bind関数
    template <typename T>
    void BindColor(const std::string& key, T* ptr, const T& defaultValue)
    {
        // Vector3の場合
        if constexpr (std::is_same_v<T, Vector3>)
        {
            BindColorVector3(key, ptr, defaultValue);
        }
        // Vector4の場合
        else if constexpr (std::is_same_v<T, Vector4>)
        {
            BindColorVector4(key, ptr, defaultValue);
        }
        // uint32_tの場合
        else if constexpr (std::is_same_v<T, uint32_t>)
        {
            BindColor32(key, ptr, defaultValue);
        }
    }

    void Draw(const std::string& key, const std::string& name = "")
    {
        // 指定されたキーが存在すれば実行
        if (items_.count(key))
        {
            items_[key](name);
        }
    }

    // 特殊系
    // インスペクターのようなもの
    void BindModel(const std::string& groupName, Model* model);
    void DrawModel(const std::string& groupName, const std::string& customLabel = "");
    void BindAnimationModel(const std::string& groupName, AnimationModel* model);
    void DrawAnimationModel(const std::string& groupName, const std::string& customLabel = "");
    void BindSprite(const std::string& groupName, Sprite* sprite);
    void DrawSprite(const std::string& groupName, const std::string& customLabel = "");

    // Vector専用のBindオーバーロード (onChange対応版)
    void Bind(const std::string& key, Vector3* ptr, const Vector3& defaultValue, float speed, std::function<void()> onChange)
    {
        // コールバック付き
        BindVector3(key, ptr, defaultValue, speed, 0.0f, 0.0f, onChange);
    }

    // int/uintをboolとして扱うための関数
    void BindBool(const std::string& key, int32_t* ptr, bool defaultValue);
    void BindBool(const std::string& key, uint32_t* ptr, bool defaultValue);

    void BindCombo(const std::string& key, int32_t* ptr, int32_t defaultValue, const char* items);

    // 回転専用のBind関数
    void BindRotation(const std::string& key, Vector3* eulerPtr, Quaternion* quatPtr, float speed = 0.01f, std::function<void()> onChange = nullptr);

private:
    // 共通処理
    template<typename T>
    void RegisterItem(const std::string& key, const T& defaultValue, T* ptr)
    {
        auto* gv = GlobalVariables::GetInstance();
        // 重複登録防止
        if (items_.find(key) == items_.end())
        {
            keys_.push_back(key); // 順序を記録
            gv->AddItem(groupPath_, key, defaultValue);
        }
    }

    void BindInt(const std::string& key, int32_t* ptr, int32_t defaultValue, float speed = 1.0f, int32_t min = 0, int32_t max = 0)
    {
        RegisterItem(key, defaultValue, ptr);

        // 保存されているデータを反映
        *ptr = GlobalVariables::GetInstance()->GetIntValue(groupPath_, key);

#ifdef IS_DEVELOPMENT
        items_[key] = [=](const std::string& nameOverride)
            {
                std::string label = (nameOverride.empty() ? key : nameOverride) + "###" + key;
                if (ImGui::DragInt(label.c_str(), ptr, speed, min, max))
                {
                    GlobalVariables::GetInstance()->SetValue(groupPath_, key, *ptr);
                }
            };
#endif
    }

    void BindUint(const std::string& key, uint32_t* ptr, uint32_t defaultValue, float speed, uint32_t min, uint32_t max)
    {
        RegisterItem(key, defaultValue, ptr);

        // ロード 
        int32_t val = GlobalVariables::GetInstance()->GetIntValue(groupPath_, key);
        *ptr = static_cast<uint32_t>(val);

#ifdef IS_DEVELOPMENT
        items_[key] = [=](const std::string& nameOverride) {
            std::string label = (nameOverride.empty() ? key : nameOverride) + "###" + key;

            int val = static_cast<int>(*ptr);

            if (ImGui::DragInt(label.c_str(), &val, speed, static_cast<int>(min), static_cast<int>(max)))
            {
                *ptr = static_cast<uint32_t>(val);
                GlobalVariables::GetInstance()->SetValue(groupPath_, key, val);
            }
            };
#endif
    }

    void BindFloat(const std::string& key, float* ptr, float defaultValue, float speed = 0.01f, float min = 0.0f, float max = 0.0f)
    {
        RegisterItem(key, defaultValue, ptr);

        // 保存されているデータを反映
        *ptr = GlobalVariables::GetInstance()->GetFloatValue(groupPath_, key);

#ifdef IS_DEVELOPMENT
        items_[key] = [=](const std::string& nameOverride)
            {
                std::string label = (nameOverride.empty() ? key : nameOverride) + "###" + key;
                if (ImGui::DragFloat(label.c_str(), ptr, speed, min, max))
                {
                    GlobalVariables::GetInstance()->SetValue(groupPath_, key, *ptr);
                }
            };
#endif
    }

    void BindBool(const std::string& key, bool* ptr, bool defaultValue)
    {
        RegisterItem(key, defaultValue, ptr);

        // 保存されているデータを反映
        *ptr = GlobalVariables::GetInstance()->GetBoolValue(groupPath_, key);

#ifdef IS_DEVELOPMENT
        items_[key] = [=](const std::string& nameOverride)
            {
                std::string label = (nameOverride.empty() ? key : nameOverride) + "###" + key;
                if (ImGui::Checkbox(label.c_str(), ptr))
                {
                    GlobalVariables::GetInstance()->SetValue(groupPath_, key, *ptr);
                }
            };
#endif
    }

    void BindVector2(const std::string& key, Vector2* ptr, const Vector2& defaultValue, float speed = 0.01f, float min = 0.0f, float max = 0.0f)
    {
        RegisterItem(key, defaultValue, ptr);

        // 保存されているデータを反映
        *ptr = GlobalVariables::GetInstance()->GetVector2Value(groupPath_, key);

#ifdef IS_DEVELOPMENT
        items_[key] = [=](const std::string& nameOverride)
            {
                std::string label = (nameOverride.empty() ? key : nameOverride) + "###" + key;
                if (ImGui::DragFloat2(label.c_str(), &ptr->x, speed, min, max))
                {
                    GlobalVariables::GetInstance()->SetValue(groupPath_, key, *ptr);
                }
            };
#endif
    }

    void BindVector3(const std::string& key, Vector3* ptr, const Vector3& defaultValue, float speed = 0.01f, float min = 0.0f, float max = 0.0f)
    {
        // 共通関数を呼ぶ
        BindVector3Internal(key, ptr, defaultValue, speed, min, max, nullptr);
    }

    // コールバックを受け取る版
    void BindVector3(const std::string& key, Vector3* ptr, const Vector3& defaultValue, float speed, float min, float max, std::function<void()> onChange)
    {
        // 共通関数を呼ぶ（コールバックを渡す）
        BindVector3Internal(key, ptr, defaultValue, speed, min, max, onChange);
    }

    void BindVector4(const std::string& key, Vector4* ptr, const Vector4& defaultValue, float speed = 0.01f, float min = 0.0f, float max = 0.0f)
    {
        RegisterItem(key, defaultValue, ptr);

        // 保存されているデータを反映
        *ptr = GlobalVariables::GetInstance()->GetVector4Value(groupPath_, key);

#ifdef IS_DEVELOPMENT
        items_[key] = [=](const std::string& nameOverride)
            {
                std::string label = (nameOverride.empty() ? key : nameOverride) + "###" + key;
                if (ImGui::DragFloat4(label.c_str(), &ptr->x, speed, min, max))
                {
                    GlobalVariables::GetInstance()->SetValue(groupPath_, key, *ptr);
                }
            };
#endif
    }


    void BindColorVector4(const std::string& key, Vector4* ptr, const Vector4& defaultValue)
    {
        RegisterItem(key, defaultValue, ptr);

        // 保存されているデータを反映
        *ptr = GlobalVariables::GetInstance()->GetVector4Value(groupPath_, key);

#ifdef IS_DEVELOPMENT
        items_[key] = [=](const std::string& nameOverride)
            {
                std::string label = (nameOverride.empty() ? key : nameOverride) + "###" + key;
                if (ImGui::ColorEdit4(label.c_str(), &ptr->x))
                {
                    GlobalVariables::GetInstance()->SetValue(groupPath_, key, *ptr);
                }
            };
#endif
    }

    void BindColor32(const std::string& key, uint32_t* ptr, uint32_t defaultValue)
    {
        auto* gv = GlobalVariables::GetInstance();
        if (items_.find(key) == items_.end())
        {
            keys_.push_back(key);
            Vector4 defVec = Math::Uint32ToColorVector(defaultValue);
            gv->AddItem(groupPath_, key, defVec);
        }

        Vector4 savedVec = gv->GetVector4Value(groupPath_, key);
        *ptr = Math::ColorVectorToUint32(savedVec);

#ifdef IS_DEVELOPMENT
        // 描画処理の登録
        items_[key] = [=](const std::string& nameOverride) {
            std::string label = (nameOverride.empty() ? key : nameOverride) + "###" + key;

            Vector4 tempColor = Math::Uint32ToColorVector(*ptr);

            if (ImGui::ColorEdit4(label.c_str(), &tempColor.x))
            {
                *ptr = Math::ColorVectorToUint32(tempColor);

                GlobalVariables::GetInstance()->SetValue(groupPath_, key, tempColor);
            }
            };
#endif
    }

    void BindColorVector3(const std::string& key, Vector3* ptr, const Vector3& defaultValue)
    {
        RegisterItem(key, defaultValue, ptr);

        // 保存されているデータを反映
        *ptr = GlobalVariables::GetInstance()->GetVector3Value(groupPath_, key);

#ifdef IS_DEVELOPMENT
        items_[key] = [=](const std::string& nameOverride)
            {
                std::string label = (nameOverride.empty() ? key : nameOverride) + "###" + key;

                if (ImGui::ColorEdit3(label.c_str(), &ptr->x))
                {
                    GlobalVariables::GetInstance()->SetValue(groupPath_, key, *ptr);
                }
            };
#endif
    }

    // 共通実装
    void BindVector3Internal(const std::string& key, Vector3* ptr, const Vector3& defaultValue, float speed, float min, float max, std::function<void()> onChange)
    {
        RegisterItem(key, defaultValue, ptr);

        // 保存データを反映
        *ptr = GlobalVariables::GetInstance()->GetVector3Value(groupPath_, key);

#ifdef IS_DEVELOPMENT
        // ラムダ式内でonChangeをキャプチャ
        items_[key] = [=](const std::string& nameOverride)
            {
                std::string label = (nameOverride.empty() ? key : nameOverride) + "###" + key;

                // 値が変更されたら
                if (ImGui::DragFloat3(label.c_str(), &ptr->x, speed, min, max))
                {
                    GlobalVariables::GetInstance()->SetValue(groupPath_, key, *ptr);

                    // コールバックがあれば実行
                    if (onChange)
                    {
                        onChange();
                    }
                }
            };
#endif
    }

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

    Engine* engine_ = nullptr;

    template <class T> static constexpr bool always_false = false;

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


