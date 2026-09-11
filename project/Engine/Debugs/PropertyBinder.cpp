#include "pch.h"
#include "PropertyBinder.h"

namespace FE
{
    void PropertyBinder::BindBoolInternal(const std::string& key, int32_t* ptr, bool defaultValue)
    {
        RegisterAndLoad(key, defaultValue ? 1 : 0, ptr, [](auto* gv, const auto& path, const auto& k) 
            {
            return gv->GetIntValue(path, k);
            });

#ifdef ENABLE_IMGUI
        items_[key] = [this, key, ptr](const std::string& nameOverride) 
            {
            std::string label = MakeLabel(key, nameOverride);
            bool isChecked = (*ptr != 0);

            if (ImGui::Checkbox(label.c_str(), &isChecked))
            {
                *ptr = isChecked ? 1 : 0;
                GlobalVariables::GetInstance()->SetValue(groupPath_, key, *ptr);
                return true;
            }
            return false;
            };
#endif
    }

    void PropertyBinder::BindBool(const std::string& key, int32_t* ptr, bool defaultValue)
    {
        BindBoolInternal(key, ptr, defaultValue);
    }

    void PropertyBinder::BindBool(const std::string& key, uint32_t* ptr, bool defaultValue)
    {
        BindBoolInternal(key, reinterpret_cast<int32_t*>(ptr), defaultValue);
    }

    void PropertyBinder::BindRotation(const std::string& key, Vector3* eulerPtr, Quaternion* quatPtr, float speed, std::function<void()> onChange)
    {
        BindVector3Internal(key, eulerPtr, { 0.0f, 0.0f, 0.0f }, speed, 0.0f, 360.0f,
            [eulerPtr, quatPtr, onChange]()
            {
                *quatPtr = Quaternion::QuaternionFromEuler(*eulerPtr);
                if (onChange) onChange();
            }
        );
        // 初期値設定
        *quatPtr = Quaternion::QuaternionFromEuler(*eulerPtr);
    }

    void PropertyBinder::Clear(bool eraseData)
    {
        keys_.clear();
        items_.clear();
        modelBindMap_.clear();
        animationBindMap_.clear();
        terrainBindMap_.clear();

        // JSONのデータも完全に消去
        if (eraseData)
        {
            GlobalVariables::GetInstance()->ClearGroup(groupPath_);
        }
    }

    void PropertyBinder::BindCombo(const std::string& key, int32_t* ptr, int32_t defaultValue, const char* items)
    {
        RegisterAndLoad(key, defaultValue, ptr, [](auto* gv, const auto& path, const auto& k)
            {
            return gv->GetIntValue(path, k);
            });

#ifdef ENABLE_IMGUI
        items_[key] = [this, key, ptr, items](const std::string& nameOverride) 
            {
            std::string label = MakeLabel(key, nameOverride);

            if (ImGui::Combo(label.c_str(), ptr, items)) 
            {
                GlobalVariables::GetInstance()->SetValue(groupPath_, key, *ptr);
                return true;
            }
            return false;
            };
#endif
    }

    void PropertyBinder::Bind(const std::string& key, int32_t* ptr, int32_t defaultValue, float speed, int32_t min, int32_t max)
    {
        RegisterAndLoad(key, defaultValue, ptr, [](auto* gv, const auto& path, const auto& k)
            {
            return gv->GetIntValue(path, k);
            });

#ifdef ENABLE_IMGUI
        items_[key] = [this, key, ptr, speed, min, max](const std::string& nameOverride) 
            {
            std::string label = MakeLabel(key, nameOverride);

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
        // uint32_tとして受け取るため、ラムダ内でキャストして返す
        RegisterAndLoad(key, defaultValue, ptr, [](auto* gv, const auto& path, const auto& k)
            {
            return static_cast<uint32_t>(gv->GetIntValue(path, k));
            });

#ifdef ENABLE_IMGUI
        items_[key] = [this, key, ptr, speed, min, max](const std::string& nameOverride)
            {
            std::string label = MakeLabel(key, nameOverride);
            int val = static_cast<int>(*ptr); // ImGui用に一時的にintへ

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
        RegisterAndLoad(key, defaultValue, ptr, [](auto* gv, const auto& path, const auto& k) 
            {
            return gv->GetFloatValue(path, k);
            });

#ifdef ENABLE_IMGUI
        items_[key] = [this, key, ptr, speed, min, max](const std::string& nameOverride)
            {
            std::string label = MakeLabel(key, nameOverride);

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
        RegisterAndLoad(key, defaultValue, ptr, [](auto* gv, const auto& path, const auto& k) 
            {
            return gv->GetFloatValue(path, k);
            });

#ifdef ENABLE_IMGUI
        items_[key] = [this, key, ptr, speed, min, max, onChange](const std::string& nameOverride) 
            {
            std::string label = MakeLabel(key, nameOverride);

            if (ImGui::DragFloat(label.c_str(), ptr, speed, min, max))
            {
                GlobalVariables::GetInstance()->SetValue(groupPath_, key, *ptr);
                if (onChange) {
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
        RegisterAndLoad(key, defaultValue, ptr, [](auto* gv, const auto& path, const auto& k) 
            {
            return gv->GetBoolValue(path, k);
            });

#ifdef ENABLE_IMGUI
        items_[key] = [this, key, ptr](const std::string& nameOverride)
            {
            std::string label = MakeLabel(key, nameOverride);
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
        RegisterAndLoad(key, defaultValue, ptr, [](auto* gv, const auto& path, const auto& k) 
            {
            return gv->GetVector2Value(path, k);
            });

#ifdef ENABLE_IMGUI
        items_[key] = [this, key, ptr, speed, min, max](const std::string& nameOverride) 
            {
            std::string label = MakeLabel(key, nameOverride);
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
        BindVector3Internal(key, ptr, defaultValue, speed, min, max, nullptr);
    }

    void PropertyBinder::Bind(const std::string& key, Vector3* ptr, const Vector3& defaultValue, float speed, float min, float max, std::function<void()> onChange)
    {
        BindVector3Internal(key, ptr, defaultValue, speed, min, max, onChange);
    }

    void PropertyBinder::Bind(const std::string& key, Vector3* ptr, const Vector3& defaultValue, float speed, std::function<void()> onChange)
    {
        BindVector3Internal(key, ptr, defaultValue, speed, 0.0f, 0.0f, onChange);
    }

    void PropertyBinder::Bind(const std::string& key, Vector4* ptr, const Vector4& defaultValue, float speed, float min, float max)
    {
        RegisterAndLoad(key, defaultValue, ptr, [](auto* gv, const auto& path, const auto& k) 
            {
            return gv->GetVector4Value(path, k);
            });

#ifdef ENABLE_IMGUI
        items_[key] = [this, key, ptr, speed, min, max](const std::string& nameOverride)
            {
            std::string label = MakeLabel(key, nameOverride);
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
        RegisterAndLoad(key, defaultValue, ptr, [](auto* gv, const auto& path, const auto& k) 
            {
            return gv->GetVector3Value(path, k);
            });

#ifdef ENABLE_IMGUI
        items_[key] = [this, key, ptr](const std::string& nameOverride) 
            {
            std::string label = MakeLabel(key, nameOverride);
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
        RegisterAndLoad(key, defaultValue, ptr, [](auto* gv, const auto& path, const auto& k) 
            {
            return gv->GetVector4Value(path, k);
            });

#ifdef ENABLE_IMGUI
        items_[key] = [this, key, ptr](const std::string& nameOverride) 
            {
            std::string label = MakeLabel(key, nameOverride);
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

        // 未登録なら Vector4 に変換して初期値を登録
        if (items_.find(key) == items_.end()) 
        {
            keys_.push_back(key);
            gv->AddItem(groupPath_, key, Math::Uint32ToColorVector(defaultValue));
        }

        // 読み込み（Vector4 -> uint32_t への変換）
        *ptr = Math::ColorVectorToUint32(gv->GetVector4Value(groupPath_, key));

#ifdef ENABLE_IMGUI
        items_[key] = [this, key, ptr](const std::string& nameOverride)
            {
            std::string label = MakeLabel(key, nameOverride);
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
    void PropertyBinder::RegisterItem(const std::string& key, const T& defaultValue, T* /*ptr*/)
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
        // 指定されたキーが存在すれば実行し、その描画結果を返す
        if (items_.count(key))
        {
            return items_[key](name);
        }
        return false;
    }

}