#pragma once
#include <vector>
#include <string>
#include <functional>
#include "GlobalVariables.h"
#include "ImGuiManager.h"

// 変数の登録を行えば、GlobalVariablesの読み書きとImGuiの表示をしてくれる関数
class PropertyBinder
{
public:
    PropertyBinder(const std::vector<std::string>& groupPath) : groupPath_(groupPath)
    {
        GlobalVariables::GetInstance()->CreateGroup(groupPath_);
    }

    void BindInt(const std::string& key, int32_t* ptr, int32_t defaultValue, float speed = 1.0f, int32_t min = 0, int32_t max = 0)
    {
        RegisterItem(key, defaultValue, ptr);

        // 保存されているデータを反映
        *ptr = GlobalVariables::GetInstance()->GetIntValue(groupPath_, key);

#ifdef IS_DEVELOPMENT
        items_[key] = [=](const std::string& nameOverride) {
            std::string label = (nameOverride.empty() ? key : nameOverride) + "###" + key;
            if (ImGui::DragInt(label.c_str(), ptr, speed, min, max)) {
                GlobalVariables::GetInstance()->SetValue(groupPath_, key, *ptr);
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
        RegisterItem(key, defaultValue, ptr);

        // 保存されているデータを反映
        *ptr = GlobalVariables::GetInstance()->GetVector3Value(groupPath_, key);

#ifdef IS_DEVELOPMENT
        items_[key] = [=](const std::string& nameOverride)
            {
                std::string label = (nameOverride.empty() ? key : nameOverride) + "###" + key;
                if (ImGui::DragFloat3(label.c_str(), &ptr->x, speed, min, max))
                {
                    GlobalVariables::GetInstance()->SetValue(groupPath_, key, *ptr);
                }
            };
#endif
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
        // 初期化とロード
        auto* gv = GlobalVariables::GetInstance();
        if (items_.find(key) == items_.end())
        {
            keys_.push_back(key);
            // デフォルト値をVector4に変換して登録
            Vector4 defVec = Math::Uint32ToColorVector(defaultValue);
            gv->AddItem(groupPath_, key, defVec);
        }

        // 保存されたVector4を読み込み、Modelのuint32_tに適用
        Vector4 savedVec = gv->GetVector4Value(groupPath_, key);
        *ptr = Math::ColorVectorToUint32(savedVec);

#ifdef IS_DEVELOPMENT
        // 描画処理の登録
        items_[key] = [=](const std::string& nameOverride) {
            std::string label = (nameOverride.empty() ? key : nameOverride) + "###" + key;

            // Modelの値を一時的にVector4にする
            Vector4 tempColor = Math::Uint32ToColorVector(*ptr);

            // ImGuiは一時変数tempColorを編集する
            if (ImGui::ColorEdit4(label.c_str(), &tempColor.x)) {

                // 結果をuint32_tに戻してModelに反映
                *ptr = Math::ColorVectorToUint32(tempColor);

                // JSON保存用にはVector4を渡す
                GlobalVariables::GetInstance()->SetValue(groupPath_, key, tempColor);
            }
            };
#endif
    }

    void Draw(const std::string& key, const std::string& name = "")
    {
        // 指定されたキーが存在すれば実行
        if (items_.count(key))
        {
            items_[key](name);
        }
    }

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

    std::vector<std::string> groupPath_;

    // 登録順序を保持するリスト
    std::vector<std::string> keys_;

    // キーと描画処理を紐付けるマップ
    std::unordered_map<std::string, std::function<void(const std::string&)>> items_;
};