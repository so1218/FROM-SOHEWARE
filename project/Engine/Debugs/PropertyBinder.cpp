#include "pch.h"
#include "PropertyBinder.h"

namespace FE
{

    void PropertyBinder::BindBool(const std::string& key, int32_t* ptr, bool defaultValue)
    {
        // boolの初期値をintに変換
        int32_t intDefault = defaultValue ? 1 : 0;

        // 初期値として登録
        RegisterItem(key, intDefault, ptr);

        // セーブデータから値を読み込む
        *ptr = GlobalVariables::GetInstance()->GetIntValue(groupPath_, key);

#ifdef ENABLE_IMGUI
        items_[key] = [=](const std::string& nameOverride)
            {
                std::string label = (nameOverride.empty() ? key : nameOverride) + "###" + key;

                // 変数の値をboolに変換して判定
                bool isChecked = (*ptr != 0);

                if (ImGui::Checkbox(label.c_str(), &isChecked))
                {
                    // チェック結果を0 or 1に戻して保存
                    *ptr = isChecked ? 1 : 0;
                    GlobalVariables::GetInstance()->SetValue(groupPath_, key, *ptr);
                    return true;
                }
                return false;
            };
#endif
    }

    // uint32_tをチェックボックスとしてバインド
    void PropertyBinder::BindBool(const std::string& key, uint32_t* ptr, bool defaultValue)
    {
        // boolの初期値をintに変換
        int32_t intDefault = defaultValue ? 1 : 0;

        // int型としてキャストして登録
        RegisterItem(key, intDefault, reinterpret_cast<int32_t*>(ptr));

        // ロード
        int32_t loadedVal = GlobalVariables::GetInstance()->GetIntValue(groupPath_, key);
        *ptr = static_cast<uint32_t>(loadedVal);

#ifdef ENABLE_IMGUI
        items_[key] = [=](const std::string& nameOverride)
            {
                std::string label = (nameOverride.empty() ? key : nameOverride) + "###" + key;

                bool isChecked = (*ptr != 0);

                if (ImGui::Checkbox(label.c_str(), &isChecked))
                {
                    *ptr = isChecked ? 1 : 0;
                    GlobalVariables::GetInstance()->SetValue(groupPath_, key, static_cast<int32_t>(*ptr));
                    return true;
                }
                return false;
            };
#endif
    }

    void PropertyBinder::BindRotation(const std::string& key, Vector3* eulerPtr, Quaternion* quatPtr, float speed, std::function<void()> onChange)
    {
        // BindVector3Internalを呼び出す
        BindVector3Internal(key, eulerPtr, { 0.0f, 0.0f, 0.0f }, speed, 0.0f, 360.0f,
            [eulerPtr, quatPtr, onChange]()
            {
                //オイラー角からクォータニオンへ変換
                *quatPtr = Quaternion::QuaternionFromEuler(*eulerPtr);

                if (onChange)
                {
                    onChange();
                }
            }
        );

        *quatPtr = Quaternion::QuaternionFromEuler(*eulerPtr);
    }

    void PropertyBinder::Clear(bool eraseData)
    {
        keys_.clear();
        items_.clear();
        modelBindMap_.clear();
        animationBindMap_.clear();
        // 引数が true の場合のみ、JSON（GlobalVariables）のデータも完全に消去する
        if (eraseData)
        {
            GlobalVariables::GetInstance()->ClearGroup(groupPath_);
        }
    }

    void PropertyBinder::BindCombo(const std::string& key, int32_t* ptr, int32_t defaultValue, const char* items)
    {
        // データの登録と読み込み
        RegisterItem(key, defaultValue, ptr);
        *ptr = GlobalVariables::GetInstance()->GetIntValue(groupPath_, key);

#ifdef ENABLE_IMGUI
        // 描画関数の登録
        items_[key] = [=](const std::string& nameOverride)
            {
                // 名前が指定されていればそれを使い、なければキーを使う + ID重複防止
                std::string label = (nameOverride.empty() ? key : nameOverride) + "###" + key;

                // ImGui::Combo を実行
                if (ImGui::Combo(label.c_str(), ptr, items))
                {
                    // 変更があったら保存
                    GlobalVariables::GetInstance()->SetValue(groupPath_, key, *ptr);
                    return true;
                }
                return false;
            };
#endif
    }

    // 汎用Bind関数
    void PropertyBinder::Bind(const std::string& key, int32_t* ptr, int32_t defaultValue, float speed, int32_t min, int32_t max)
    {
        RegisterItem(key, defaultValue, ptr);

        // 保存されているデータを反映
        *ptr = GlobalVariables::GetInstance()->GetIntValue(groupPath_, key);

#ifdef ENABLE_IMGUI
        items_[key] = [=](const std::string& nameOverride)
            {
                std::string label = (nameOverride.empty() ? key : nameOverride) + "###" + key;
                if (ImGui::DragInt(label.c_str(), ptr, speed, min, max))
                {
                    GlobalVariables::GetInstance()->SetValue(groupPath_, key, *ptr);
                    return true;
                }
                return false;
            };
#endif
    }

    void PropertyBinder::Bind(const std::string& key, uint32_t* ptr, uint32_t defaultValue, float speed, uint32_t min, uint32_t max)
    {
        RegisterItem(key, defaultValue, ptr);

        // ロード 
        int32_t val = GlobalVariables::GetInstance()->GetIntValue(groupPath_, key);
        *ptr = static_cast<uint32_t>(val);

#ifdef ENABLE_IMGUI
        items_[key] = [=](const std::string& nameOverride)
            { 
            std::string label = (nameOverride.empty() ? key : nameOverride) + "###" + key;

            int val = static_cast<int>(*ptr);

            if (ImGui::DragInt(label.c_str(), &val, speed, static_cast<int>(min), static_cast<int>(max)))
            {
                *ptr = static_cast<uint32_t>(val);
                GlobalVariables::GetInstance()->SetValue(groupPath_, key, val);
                return true; 
            }
            return false; 
            };
#endif
    }

    void PropertyBinder::Bind(const std::string& key, float* ptr, float defaultValue, float speed, float min, float max)
    {
        RegisterItem(key, defaultValue, ptr);

        // 保存されているデータを反映
        *ptr = GlobalVariables::GetInstance()->GetFloatValue(groupPath_, key);

#ifdef ENABLE_IMGUI
        items_[key] = [=](const std::string& nameOverride)
            {
                std::string label = (nameOverride.empty() ? key : nameOverride) + "###" + key;
                if (ImGui::DragFloat(label.c_str(), ptr, speed, min, max))
                {
                    GlobalVariables::GetInstance()->SetValue(groupPath_, key, *ptr);
                    return true;
                }
                return false;
            };
#endif
    }

    void PropertyBinder::Bind(const std::string& key, float* ptr, float defaultValue, float speed, float min, float max, std::function<void()> onChange)
    {
        RegisterItem(key, defaultValue, ptr);

        // 保存されているデータを反映
        *ptr = GlobalVariables::GetInstance()->GetFloatValue(groupPath_, key);

#ifdef ENABLE_IMGUI
        items_[key] = [=](const std::string& nameOverride)
            {
                std::string label = (nameOverride.empty() ? key : nameOverride) + "###" + key;

                // ImGuiのスライダー等で値が変更されたら
                if (ImGui::DragFloat(label.c_str(), ptr, speed, min, max))
                {
                    GlobalVariables::GetInstance()->SetValue(groupPath_, key, *ptr);

                    // コールバックがあれば実行
                    if (onChange)
                    {
                        onChange();
                    }
                    return true; 
                }
                return false;
            };
#endif
    }

    void PropertyBinder::Bind(const std::string& key, bool* ptr, bool defaultValue)
    {
        RegisterItem(key, defaultValue, ptr);

        // 保存されているデータを反映
        *ptr = GlobalVariables::GetInstance()->GetBoolValue(groupPath_, key);

#ifdef ENABLE_IMGUI
        items_[key] = [=](const std::string& nameOverride)
            {
                std::string label = (nameOverride.empty() ? key : nameOverride) + "###" + key;
                if (ImGui::Checkbox(label.c_str(), ptr))
                {
                    GlobalVariables::GetInstance()->SetValue(groupPath_, key, *ptr);
                    return true;
                }
                return false;
            };
#endif
    }

    void PropertyBinder::Bind(const std::string& key, Vector2* ptr, const Vector2& defaultValue, float speed, float min, float max)
    {
        RegisterItem(key, defaultValue, ptr);

        // 保存されているデータを反映
        *ptr = GlobalVariables::GetInstance()->GetVector2Value(groupPath_, key);

#ifdef ENABLE_IMGUI
        items_[key] = [=](const std::string& nameOverride)
            {
                std::string label = (nameOverride.empty() ? key : nameOverride) + "###" + key;
                if (ImGui::DragFloat2(label.c_str(), &ptr->x, speed, min, max))
                {
                    GlobalVariables::GetInstance()->SetValue(groupPath_, key, *ptr);
                    return true;
                }
                return false;
            };
#endif
    }

    void PropertyBinder::Bind(const std::string& key, Vector3* ptr, const Vector3& defaultValue, float speed, float min, float max)
    {
        // 共通関数
        BindVector3Internal(key, ptr, defaultValue, speed, min, max, nullptr);
    }

    // コールバックを受け取る版のVector3 
    void PropertyBinder::Bind(const std::string& key, Vector3* ptr, const Vector3& defaultValue, float speed, float min, float max, std::function<void()> onChange)
    {
        BindVector3Internal(key, ptr, defaultValue, speed, min, max, onChange);
    }

    // 省略版コールバック
    void PropertyBinder::Bind(const std::string& key, Vector3* ptr, const Vector3& defaultValue, float speed, std::function<void()> onChange)
    {
        BindVector3Internal(key, ptr, defaultValue, speed, 0.0f, 0.0f, onChange);
    }

    void PropertyBinder::Bind(const std::string& key, Vector4* ptr, const Vector4& defaultValue, float speed, float min, float max)
    {
        RegisterItem(key, defaultValue, ptr);

        // 保存されているデータを反映
        *ptr = GlobalVariables::GetInstance()->GetVector4Value(groupPath_, key);

#ifdef ENABLE_IMGUI
        items_[key] = [=](const std::string& nameOverride)
            {
                std::string label = (nameOverride.empty() ? key : nameOverride) + "###" + key;
                if (ImGui::DragFloat4(label.c_str(), &ptr->x, speed, min, max))
                {
                    GlobalVariables::GetInstance()->SetValue(groupPath_, key, *ptr);
                    return true;
                }
                return false;
            };
#endif
    }

    // 色用Bind関数
    void PropertyBinder::BindColor(const std::string& key, Vector3* ptr, const Vector3& defaultValue)
    {
        RegisterItem(key, defaultValue, ptr);

        // 保存されているデータを反映
        *ptr = GlobalVariables::GetInstance()->GetVector3Value(groupPath_, key);

#ifdef ENABLE_IMGUI
        items_[key] = [=](const std::string& nameOverride)
            {
                std::string label = (nameOverride.empty() ? key : nameOverride) + "###" + key;

                if (ImGui::ColorEdit3(label.c_str(), &ptr->x))
                {
                    GlobalVariables::GetInstance()->SetValue(groupPath_, key, *ptr);
                    return true;
                }
                return false;
            };
#endif
    }

    void PropertyBinder::BindColor(const std::string& key, Vector4* ptr, const Vector4& defaultValue)
    {
        RegisterItem(key, defaultValue, ptr);

        // 保存されているデータを反映
        *ptr = GlobalVariables::GetInstance()->GetVector4Value(groupPath_, key);

#ifdef ENABLE_IMGUI
        items_[key] = [=](const std::string& nameOverride)
            {
                std::string label = (nameOverride.empty() ? key : nameOverride) + "###" + key;
                if (ImGui::ColorEdit4(label.c_str(), &ptr->x))
                {
                    GlobalVariables::GetInstance()->SetValue(groupPath_, key, *ptr);
                    return true;
                }
                return false;
            };
#endif
    }

    void PropertyBinder::BindColor(const std::string& key, uint32_t* ptr, uint32_t defaultValue)
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

#ifdef ENABLE_IMGUI
        // 描画処理の登録
        items_[key] = [=](const std::string& nameOverride)
            {
            std::string label = (nameOverride.empty() ? key : nameOverride) + "###" + key;

            Vector4 tempColor = Math::Uint32ToColorVector(*ptr);

            if (ImGui::ColorEdit4(label.c_str(), &tempColor.x))
            {
                *ptr = Math::ColorVectorToUint32(tempColor);

                GlobalVariables::GetInstance()->SetValue(groupPath_, key, tempColor);
                return true; 
            }
            return false; 
            };
#endif
    }

    // 共通処理
    template<typename T>
    void PropertyBinder::RegisterItem(const std::string& key, const T& defaultValue, T* ptr)
    {
        auto* gv = GlobalVariables::GetInstance();
        // 重複登録防止
        if (items_.find(key) == items_.end())
        {
            keys_.push_back(key); // 順序を記録
            gv->AddItem(groupPath_, key, defaultValue);
        }
    }

    // 共通実装
    void PropertyBinder::BindVector3Internal(const std::string& key, Vector3* ptr, const Vector3& defaultValue, float speed, float min, float max, std::function<void()> onChange)
    {
        RegisterItem(key, defaultValue, ptr);

        // 保存データを反映
        *ptr = GlobalVariables::GetInstance()->GetVector3Value(groupPath_, key);

#ifdef ENABLE_IMGUI
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
                    return true;
                }
                return false; 
            };
#endif
    }

    bool PropertyBinder::Draw(const std::string& key, const std::string& name)
    {
        // 指定されたキーが存在すれば実行し、その描画結果(bool)を返す
        if (items_.count(key))
        {
            return items_[key](name);
        }
        return false;
    }

}