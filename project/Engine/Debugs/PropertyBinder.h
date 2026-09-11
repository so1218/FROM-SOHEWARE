#pragma once
#include "GlobalVariables.h"
#include "MathUtils.h"
#include "Structures.h"
#include "ModelManager.h"
#include "TextureManager.h"

namespace FE
{

class Model;
class AnimationModel;
class Sprite;
class Terrain;
class Engine;

// 変数の登録を行えば、GlobalVariablesの読み書きとImGuiの表示をしてくれる関数
class PropertyBinder
{
public:
    template <typename... Args>
    PropertyBinder(Engine* engine, Args&&... args)
        : engine_(engine),
        groupPath_{ std::string(std::forward<Args>(args))... }
    {
        GlobalVariables::GetInstance()->CreateGroup(groupPath_);
    }

    // 基本型バインド
    void Bind(const std::string& key, int32_t* ptr, int32_t defaultValue, float speed = 1.0f, int32_t min = 0, int32_t max = 0);
    void Bind(const std::string& key, uint32_t* ptr, uint32_t defaultValue, float speed = 1.0f, uint32_t min = 0, uint32_t max = 0);
    void Bind(const std::string& key, float* ptr, float defaultValue, float speed = 0.01f, float min = 0.0f, float max = 0.0f);
    void Bind(const std::string& key, float* ptr, float defaultValue, float speed, float min, float max, std::function<void()> onChange);
    void Bind(const std::string& key, bool* ptr, bool defaultValue);
    void Bind(const std::string& key, Vector2* ptr, const Vector2& defaultValue, float speed = 0.01f, float min = 0.0f, float max = 0.0f);
    void Bind(const std::string& key, Vector3* ptr, const Vector3& defaultValue, float speed = 0.01f, float min = 0.0f, float max = 0.0f);
    void Bind(const std::string& key, Vector3* ptr, const Vector3& defaultValue, float speed, float min, float max, std::function<void()> onChange);
    void Bind(const std::string& key, Vector3* ptr, const Vector3& defaultValue, float speed, std::function<void()> onChange);
    void Bind(const std::string& key, Vector4* ptr, const Vector4& defaultValue, float speed = 0.01f, float min = 0.0f, float max = 0.0f);

    // 特殊制御バインド
    void BindColor(const std::string& key, Vector3* ptr, const Vector3& defaultValue);
    void BindColor(const std::string& key, Vector4* ptr, const Vector4& defaultValue);
    void BindColor(const std::string& key, uint32_t* ptr, uint32_t defaultValue);

    void BindBool(const std::string& key, int32_t* ptr, bool defaultValue);
    void BindBool(const std::string& key, uint32_t* ptr, bool defaultValue);
    void BindCombo(const std::string& key, int32_t* ptr, int32_t defaultValue, const char* items);
    void BindRotation(const std::string& key, Vector3* eulerPtr, Quaternion* quatPtr, float speed = 0.01f, std::function<void()> onChange = nullptr);

    void BindModelName(const std::string& key, std::string* currentModelName, const std::string& defaultName, std::function<void(const std::string&)> onChange = nullptr);
    void BindTexture(const std::string& key, const std::string& initialValue, std::function<void(const std::string&)> onValueChanged, const std::string& defaultName = "white1x1", TextureType filterType = TextureType::Albedo);
    void BindTexture(const std::string& key, std::string* currentTextureName, uint32_t* currentHandlePtr, const std::string& defaultName, TextureType filterType = TextureType::Albedo, std::function<void()> callback = nullptr);

    // オブジェクトバインド
    void BindModel(const std::string& groupName, Model* model);
    void BindAnimationModel(const std::string& groupName, AnimationModel* model);
    void BindSprite(const std::string& groupName, Sprite* sprite);
    void BindTerrain(const std::string& groupName, Terrain* terrain);

    // UI描画
    bool Draw(const std::string& key, const std::string& name = "");
    bool DrawModel(const std::string& groupName, const std::string& customLabel = "");
    void DrawAnimationModel(const std::string& groupName, const std::string& customLabel = "");
    void DrawSprite(const std::string& groupName, const std::string& customLabel = "");
    void DrawTerrain(const std::string& groupName, const std::string& customLabel = "");

    void Clear(bool eraseData = false);
    const std::vector<std::string>& GetGroupPath() const { return groupPath_; }

private:
    // 内部ヘルパー
    template<typename T, typename GetFunc>
    void RegisterAndLoad(const std::string& key, const T& defaultValue, T* ptr, GetFunc&& getFunc) {
        RegisterItem(key, defaultValue, ptr);
        *ptr = getFunc(GlobalVariables::GetInstance(), groupPath_, key);
    }

    template<typename T>
    void RegisterItem(const std::string& key, const T& defaultValue, T* ptr);

    std::string MakeLabel(const std::string& key, const std::string& nameOverride) const {
        return (nameOverride.empty() ? key : nameOverride) + "###" + key;
    }

    void BindBoolInternal(const std::string& key, int32_t* ptr, bool defaultValue);
    void BindVector3Internal(const std::string& key, Vector3* ptr, const Vector3& defaultValue, float speed, float min, float max, std::function<void()> onChange);
    void BindMaterialProperties(const std::string& prefix, MaterialHandle* handle);

    template <typename ModelType>
    bool DrawMaterialUI(ModelType* targetModel, const std::string& prefix, GlobalVariables* gv, const std::vector<std::string>& groupPath);

private:
    Engine* engine_ = nullptr;
    std::vector<std::string> groupPath_;
    std::vector<std::string> keys_;
    std::unordered_map<std::string, std::function<bool(const std::string&)>> items_;

    struct ModelBindInfo { Model* model = nullptr; };
    struct AnimationBindInfo { AnimationModel* model = nullptr; };
    struct TerrainBindInfo { Terrain* terrain = nullptr; };

    std::unordered_map<std::string, ModelBindInfo> modelBindMap_;
    std::unordered_map<std::string, AnimationBindInfo> animationBindMap_;
    std::unordered_map<std::string, TerrainBindInfo> terrainBindMap_;
};

}

