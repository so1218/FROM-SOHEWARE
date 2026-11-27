#include "pch.h"
#include "PropertyBinder.h"
#include "DevError.h"

namespace FS
{
namespace
{
    // メニューに出す値の文字列
    std::string FormatAnyValue(const std::any& value)
    {
        if (const int32_t* intValue = std::any_cast<int32_t>(&value)) return std::to_string(*intValue);
        if (const uint32_t* uintValue = std::any_cast<uint32_t>(&value)) return std::to_string(*uintValue);
        if (const float* floatValue = std::any_cast<float>(&value)) return std::format("{:.3f}", *floatValue);
        if (const bool* boolValue = std::any_cast<bool>(&value)) return *boolValue ? "オン" : "オフ";
        if (const Vector2* vector2 = std::any_cast<Vector2>(&value)) return std::format("{:.3f}, {:.3f}", vector2->x, vector2->y);
        if (const Vector3* vector3 = std::any_cast<Vector3>(&value)) return std::format("{:.3f}, {:.3f}, {:.3f}", vector3->x, vector3->y, vector3->z);
        if (const Vector4* vector4 = std::any_cast<Vector4>(&value)) return std::format("{:.3f}, {:.3f}, {:.3f}, {:.3f}", vector4->x, vector4->y, vector4->z, vector4->w);
        if (const std::string* text = std::any_cast<std::string>(&value)) return "\"" + *text + "\"";
        return "?";
    }

    // 型も値も同じか。型が違うものは戻せないので、別のものとして扱う
    bool IsSameAnyValue(const std::any& a, const std::any& b)
    {
        if (a.type() != b.type()) return false;
        if (const int32_t* intValue = std::any_cast<int32_t>(&a)) return *intValue == std::any_cast<int32_t>(b);
        if (const uint32_t* uintValue = std::any_cast<uint32_t>(&a)) return *uintValue == std::any_cast<uint32_t>(b);
        if (const float* floatValue = std::any_cast<float>(&a)) return *floatValue == std::any_cast<float>(b);
        if (const bool* boolValue = std::any_cast<bool>(&a)) return *boolValue == std::any_cast<bool>(b);
        if (const Vector2* vector2 = std::any_cast<Vector2>(&a)) return *vector2 == std::any_cast<Vector2>(b);
        if (const Vector3* vector3 = std::any_cast<Vector3>(&a)) return *vector3 == std::any_cast<Vector3>(b);
        if (const Vector4* vector4 = std::any_cast<Vector4>(&a)) return *vector4 == std::any_cast<Vector4>(b);
        if (const std::string* text = std::any_cast<std::string>(&a)) return *text == std::any_cast<std::string>(b);
        return false;
    }
}

    PropertyBinder::~PropertyBinder()
    {
        std::erase(GetInstances(), this);
        // 消えた PropertyBinder の変数には書き込めないので、戻す予定と取り消しからも外す
        auto isMine = [this](const SavedValueChange& change) { return change.binder == this; };
        std::erase_if(GetPendingChanges(), isMine);
        std::erase_if(GetBulkUndo(), isMine);
    }

    std::vector<PropertyBinder*>& PropertyBinder::GetInstances()
    {
        static std::vector<PropertyBinder*> instances;
        return instances;
    }

    std::vector<PropertyBinder::SavedValueChange>& PropertyBinder::GetPendingChanges()
    {
        static std::vector<SavedValueChange> pendingChanges;
        return pendingChanges;
    }

    std::vector<PropertyBinder::SavedValueChange>& PropertyBinder::GetBulkUndo()
    {
        static std::vector<SavedValueChange> bulkUndo;
        return bulkUndo;
    }

    std::vector<PropertyBinder::SavedValueChange> PropertyBinder::CollectChangesFromSaved(const std::vector<std::string>& groupPath)
    {
        std::vector<SavedValueChange> changes;
        auto* gv = GlobalVariables::GetInstance();

        for (PropertyBinder* binder : GetInstances())
        {
            // 保存先が groupPath から始まるものだけ
            if (binder->groupPath_.size() < groupPath.size()
                || !std::equal(groupPath.begin(), groupPath.end(), binder->groupPath_.begin()))
            {
                continue;
            }

            const std::unordered_map<std::string, std::any> savedValues = gv->LoadSavedGroup(binder->groupPath_);
            for (const std::string& key : binder->keys_)
            {
                if (!binder->revertInfos_.contains(key))
                {
                    continue;
                }
                auto savedIt = savedValues.find(key);
                if (savedIt == savedValues.end())
                {
                    continue;
                }
                std::any currentValue = gv->GetAnyValue(binder->groupPath_, key);
                // 型が違うものは戻すと読み出しで落ちるので外す
                if (savedIt->second.type() != currentValue.type() || IsSameAnyValue(savedIt->second, currentValue))
                {
                    continue;
                }
                changes.push_back({ binder, key, savedIt->second, std::move(currentValue) });
            }
        }
        return changes;
    }

    void PropertyBinder::DrawRevertToSavedButton(const std::vector<std::string>& groupPath, const char* label)
    {
#ifdef ENABLE_IMGUI
        std::string popupName = "最後に保存した値に戻す###RevertToSaved";
        for (const std::string& group : groupPath)
        {
            popupName += "/" + group;
        }

        if (ImGui::Button(label))
        {
            GetPendingChanges() = CollectChangesFromSaved(groupPath);
            ImGui::OpenPopup(popupName.c_str());
        }

        if (!ImGui::BeginPopupModal(popupName.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            return;
        }

        std::vector<SavedValueChange>& changes = GetPendingChanges();
        if (changes.empty())
        {
            ImGui::TextUnformatted("保存した値と違う項目はない");
            if (ImGui::Button("閉じる"))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
            return;
        }

        ImGui::Text("%d 個の項目を、最後に保存した値に戻す", static_cast<int>(changes.size()));
        ImGui::TextDisabled("ファイルは変わらない。戻したあとも「取り消す」で元に戻せる");
        ImGui::Separator();
        if (ImGui::BeginChild("##Changes", ImVec2(560.0f, 240.0f), ImGuiChildFlags_Borders))
        {
            for (const SavedValueChange& change : changes)
            {
                std::string place;
                for (const std::string& group : change.binder->groupPath_)
                {
                    place += (place.empty() ? "" : " / ") + group;
                }
                ImGui::Text("%s / %s", place.c_str(), change.key.c_str());
                ImGui::TextDisabled("    %s  →  %s", FormatAnyValue(change.currentValue).c_str(), FormatAnyValue(change.savedValue).c_str());
            }
        }
        ImGui::EndChild();
        ImGui::Separator();

        if (ImGui::Button("やめる", ImVec2(120.0f, 0.0f)))
        {
            changes.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("戻す", ImVec2(120.0f, 0.0f)))
        {
            std::vector<SavedValueChange>& bulkUndo = GetBulkUndo();
            bulkUndo.clear();
            for (const SavedValueChange& change : changes)
            {
                change.binder->undoValues_.erase(change.key);
                change.binder->ApplyStoredValue(change.key, change.savedValue, false);
                bulkUndo.push_back(change);
            }
            changes.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
#else
        (void)groupPath;
        (void)label;
#endif
    }

    void PropertyBinder::DrawUndoBulkRevertButton()
    {
#ifdef ENABLE_IMGUI
        std::vector<SavedValueChange>& bulkUndo = GetBulkUndo();
        if (bulkUndo.empty())
        {
            return;
        }

        if (ImGui::Button(std::format("まとめて戻したのを取り消す ({} 個)", bulkUndo.size()).c_str()))
        {
            for (const SavedValueChange& change : bulkUndo)
            {
                change.binder->ApplyStoredValue(change.key, change.currentValue, false);
            }
            bulkUndo.clear();
        }
#endif
    }

    void PropertyBinder::RegisterRevert(const std::string& key, std::any defaultValue, std::function<void()> applyFromStore)
    {
        revertInfos_[key] = { std::move(defaultValue), std::move(applyFromStore) };
    }

    void PropertyBinder::ApplyStoredValue(const std::string& key, const std::any& value, bool recordUndo)
    {
        auto it = revertInfos_.find(key);
        if (it == revertInfos_.end())
        {
            return;
        }

        auto* gv = GlobalVariables::GetInstance();
        if (recordUndo)
        {
            undoValues_[key] = gv->GetAnyValue(groupPath_, key);
        }
        // ファイルには書かない
        gv->SetAnyValue(groupPath_, key, value);
        if (it->second.applyFromStore)
        {
            it->second.applyFromStore();
        }
    }

    bool PropertyBinder::DrawRevertMenu(const std::string& key)
    {
#ifdef ENABLE_IMGUI
        auto revertIt = revertInfos_.find(key);
        if (revertIt == revertInfos_.end())
        {
            return false;
        }

        // 同じ窓に同じキーの項目があっても混ざらないよう、保存先も ID に入れる
        std::string popupId = "##Revert";
        for (const std::string& group : groupPath_)
        {
            popupId += "/" + group;
        }
        popupId += "/" + key;
        if (!ImGui::BeginPopupContextItem(popupId.c_str()))
        {
            return false;
        }

        auto* gv = GlobalVariables::GetInstance();
        if (ImGui::IsWindowAppearing() || revertMenuKey_ != popupId)
        {
            revertMenuKey_ = popupId;
            revertMenuSavedValue_ = gv->LoadSavedValue(groupPath_, key);
        }

        const std::any currentValue = gv->GetAnyValue(groupPath_, key);
        // 型が違う値を入れると読み出しで落ちるので、同じ型のときだけ戻せる
        auto canApply = [&currentValue](const std::any& value)
            {
                return value.type() == currentValue.type() && !IsSameAnyValue(value, currentValue);
            };

        bool isApplied = false;
        ImGui::TextDisabled("%s", key.c_str());
        ImGui::TextDisabled("今の値: %s", FormatAnyValue(currentValue).c_str());
        ImGui::Separator();

        if (revertMenuSavedValue_)
        {
            const std::string savedText = FormatAnyValue(*revertMenuSavedValue_);
            if (ImGui::MenuItem("最後に保存した値に戻す", savedText.c_str(), false, canApply(*revertMenuSavedValue_)))
            {
                ApplyStoredValue(key, *revertMenuSavedValue_, true);
                isApplied = true;
            }
        }
        else
        {
            ImGui::MenuItem("最後に保存した値に戻す", "保存されていない", false, false);
        }

        const std::any& defaultValue = revertIt->second.defaultValue;
        if (ImGui::MenuItem("初期値に戻す", FormatAnyValue(defaultValue).c_str(), false, canApply(defaultValue)))
        {
            ApplyStoredValue(key, defaultValue, true);
            isApplied = true;
        }

        auto undoIt = undoValues_.find(key);
        if (undoIt != undoValues_.end())
        {
            ImGui::Separator();
            const std::any undoValue = undoIt->second;
            if (ImGui::MenuItem("戻す前の値に戻す", FormatAnyValue(undoValue).c_str(), false, canApply(undoValue)))
            {
                undoValues_.erase(undoIt);
                ApplyStoredValue(key, undoValue, false);
                isApplied = true;
            }
        }

        ImGui::Separator();
        ImGui::TextDisabled("戻してもファイルには保存されない");
        ImGui::EndPopup();
        return isApplied;
#else
        (void)key;
        return false;
#endif
    }

    void PropertyBinder::BindBoolInternal(const std::string& key, int32_t* ptr, bool defaultValue)
    {
        RegisterAndLoad(key, defaultValue ? 1 : 0, ptr, [](auto* gv, const auto& path, const auto& k) 
            {
            return gv->GetIntValue(path, k);
            });
        RegisterRevert(key, std::any(int32_t(defaultValue ? 1 : 0)), [this, key, ptr]()
            {
                *ptr = GlobalVariables::GetInstance()->GetIntValue(groupPath_, key);
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

    void PropertyBinder::BindRotation(const std::string& key, Quaternion* rotation, float speed, std::function<void()> onChange)
    {
        Vector3* euler = &rotationEulers_[key];
        // 保存された値がなければ、今の回転をそのまま使う
        BindVector3Internal(key, euler, Quaternion::QuaternionToEuler(*rotation), speed, 0.0f, 0.0f,
            [euler, rotation, onChange]()
            {
                *rotation = Quaternion::QuaternionFromEuler(*euler);
                if (onChange) onChange();
            }
        );
        // 保存されていたオイラー角から回転を作る
        *rotation = Quaternion::QuaternionFromEuler(*euler);

#ifdef ENABLE_IMGUI
        auto drawEuler = items_[key];
        items_[key] = [drawEuler, euler, rotation](const std::string& nameOverride)
            {
                const Quaternion eulerRotation = Quaternion::QuaternionFromEuler(*euler);
                const bool isSameRotation = std::abs(Quaternion::Dot(eulerRotation, *rotation)) > 0.9999f;
                if (!isSameRotation)
                {
                    *euler = Quaternion::QuaternionToEuler(*rotation);
                }
                return drawEuler(nameOverride);
            };
#endif
    }

    void PropertyBinder::Clear(bool eraseData)
    {
        keys_.clear();
        items_.clear();
        rotationEulers_.clear();
        revertInfos_.clear();
        undoValues_.clear();
        selfLabeledKeys_.clear();
        revertMenuKey_.clear();
        revertMenuSavedValue_.reset();
        modelBindMap_.clear();
        animationBindMap_.clear();

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
        RegisterRevert(key, std::any(defaultValue), [this, key, ptr]()
            {
                *ptr = GlobalVariables::GetInstance()->GetIntValue(groupPath_, key);
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
        RegisterRevert(key, std::any(defaultValue), [this, key, ptr]()
            {
                *ptr = GlobalVariables::GetInstance()->GetIntValue(groupPath_, key);
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
        // 保存すると int32_t で読み込まれるので、初期値も int32_t で持つ
        RegisterRevert(key, std::any(static_cast<int32_t>(defaultValue)), [this, key, ptr]()
            {
                *ptr = static_cast<uint32_t>(GlobalVariables::GetInstance()->GetIntValue(groupPath_, key));
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
        RegisterRevert(key, std::any(defaultValue), [this, key, ptr]()
            {
                *ptr = GlobalVariables::GetInstance()->GetFloatValue(groupPath_, key);
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
        RegisterRevert(key, std::any(defaultValue), [this, key, ptr, onChange]()
            {
                *ptr = GlobalVariables::GetInstance()->GetFloatValue(groupPath_, key);
                if (onChange) onChange();
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
        RegisterRevert(key, std::any(defaultValue), [this, key, ptr]()
            {
                *ptr = GlobalVariables::GetInstance()->GetBoolValue(groupPath_, key);
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
        RegisterRevert(key, std::any(defaultValue), [this, key, ptr]()
            {
                *ptr = GlobalVariables::GetInstance()->GetVector2Value(groupPath_, key);
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
        RegisterRevert(key, std::any(defaultValue), [this, key, ptr]()
            {
                *ptr = GlobalVariables::GetInstance()->GetVector4Value(groupPath_, key);
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
        RegisterRevert(key, std::any(defaultValue), [this, key, ptr]()
            {
                *ptr = GlobalVariables::GetInstance()->GetVector3Value(groupPath_, key);
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
        RegisterRevert(key, std::any(defaultValue), [this, key, ptr]()
            {
                *ptr = GlobalVariables::GetInstance()->GetVector4Value(groupPath_, key);
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

        // 読み込み
        *ptr = Math::ColorVectorToUint32(gv->GetVector4Value(groupPath_, key));
        RegisterRevert(key, std::any(Math::Uint32ToColorVector(defaultValue)), [this, key, ptr]()
            {
                *ptr = Math::ColorVectorToUint32(GlobalVariables::GetInstance()->GetVector4Value(groupPath_, key));
            });

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
        RegisterRevert(key, std::any(defaultValue), [this, key, ptr, onChange]()
            {
                *ptr = GlobalVariables::GetInstance()->GetVector3Value(groupPath_, key);
                if (onChange) onChange();
            });

#ifdef ENABLE_IMGUI
        // ラムダ式内でonChangeをキャプチャ
        items_[key] = [=](const std::string& nameOverride)
            {
                std::string label = MakeLabel(key, nameOverride);

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

    bool PropertyBinder::Draw(const std::string& key, const std::string& name, std::source_location location)
    {
        const auto it = items_.find(key);
        if (it == items_.end())
        {
#ifdef ENABLE_IMGUI
            // Bind と Draw でキーの綴りが違うと、項目が黙って出なくなるので知らせる
            DevError::Report(std::format("PropertyBinder: Bind していないキー \"{}\" を Draw しようとした\nBind と Draw でキーの綴りが同じか確認する", key), location);
#endif
            return false;
        }

        bool isChanged = false;
#ifdef ENABLE_IMGUI
        if (selfLabeledKeys_.contains(key))
        {
            isChanged = it->second(name);
        }
        else
        {
            // 項目名を左の列に描き、入力欄はその右の残りの幅いっぱいに伸ばす
            std::string displayName = name.empty() ? key : name;
            if (const size_t hiddenStart = displayName.find("##"); hiddenStart != std::string::npos)
            {
                displayName.resize(hiddenStart);
            }

            const ImGuiStyle& style = ImGui::GetStyle();
            const float fontSize = ImGui::GetFontSize();
            const float labelColumnWidth = std::clamp(ImGui::GetContentRegionAvail().x * 0.4f, fontSize * 5.0f, fontSize * 16.0f);
            const float startX = ImGui::GetCursorPosX();

            ImGui::Dummy(ImVec2(labelColumnWidth - style.ItemSpacing.x, ImGui::GetFrameHeight()));
            const ImVec2 labelMin = ImGui::GetItemRectMin();
            const ImVec2 labelMax = ImGui::GetItemRectMax();
            const ImVec2 textSize = ImGui::CalcTextSize(displayName.c_str());

            ImGui::RenderTextEllipsis(ImGui::GetWindowDrawList(),
                ImVec2(labelMin.x, labelMin.y + style.FramePadding.y), labelMax, labelMax.x,
                displayName.c_str(), nullptr, &textSize);
            if (textSize.x > labelMax.x - labelMin.x && ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", displayName.c_str());
            }

            ImGui::SameLine(startX + labelColumnWidth);
            ImGui::PushItemWidth(-FLT_MIN);
            const bool wasLabelDrawnSeparately = isLabelDrawnSeparately_;
            isLabelDrawnSeparately_ = true;
            isChanged = it->second(name);
            isLabelDrawnSeparately_ = wasLabelDrawnSeparately;
            ImGui::PopItemWidth();
        }

        // 自分で触ったら、戻す前の値は使わない
        if (isChanged)
        {
            undoValues_.erase(key);
        }
        if (DrawRevertMenu(key))
        {
            isChanged = true;
        }
#else
        isChanged = it->second(name);
#endif
        return isChanged;
    }

    std::string PropertyBinder::DescribeKey(const std::string& key) const
    {
        std::string place = groupPath_.empty() ? std::string("GlobalVariables") : groupPath_[0] + ".json";
        for (size_t i = 1; i < groupPath_.size(); ++i)
        {
            place += " / " + groupPath_[i];
        }
        return std::format("{} の \"{}\"", place, key);
    }

    // 一括で登録順に描画する関数を追加
    void PropertyBinder::DrawAll()
    {
        for (const auto& key : keys_)
        {
            Draw(key);
        }
    }

}