#pragma once
#include "GlobalVariables.h"
#include "MathUtils.h"
#include "Structures.h"
#include "ModelManager.h"
#include "TextureManager.h"

namespace FS
{

class Model;
class AnimationModel;
class Sprite;
class Text;
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
        // まとめて元に戻すときに探せるよう、作られている PropertyBinder を覚えておく
        GetInstances().push_back(this);
    }
    ~PropertyBinder();

    // 覚えているアドレスがずれないよう、コピーと移動はしない
    PropertyBinder(const PropertyBinder&) = delete;
    PropertyBinder& operator=(const PropertyBinder&) = delete;

    // 保存先が groupPath から始まる項目を、まとめて最後に保存した値に戻すボタン
    static void DrawRevertToSavedButton(const std::vector<std::string>& groupPath, const char* label);
    // まとめて戻したのを取り消すボタン。取り消せるものがあるときだけ出す
    static void DrawUndoBulkRevertButton();

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
    // 回転はクォータニオンで持ち、ImGui での編集と保存はオイラー角で行う
    void BindRotation(const std::string& key, Quaternion* rotation, float speed = 0.01f, std::function<void()> onChange = nullptr);

    void BindModelName(const std::string& key, std::string* currentModelName, const std::string& defaultName, std::function<void(const std::string&)> onChange = nullptr);
    void BindTexture(const std::string& key, const std::string& initialValue, std::function<void(const std::string&)> onValueChanged, const std::string& defaultName = "white1x1", TextureType filterType = TextureType::Albedo);
    void BindTexture(const std::string& key, std::string* currentTextureName, uint32_t* currentHandlePtr, const std::string& defaultName, TextureType filterType = TextureType::Albedo, std::function<void()> callback = nullptr);

    // 文字列を入力欄で編集する
    void BindString(const std::string& key, const std::string& defaultValue, std::function<void(const std::string&)> onChange, bool isMultiline = false);
    // FontList.csv で読み込んだフォントから選ぶ
    void BindFontName(const std::string& key, const std::string& defaultName, std::function<void(const std::string&)> onChange);

    // オブジェクトバインド
    void BindModel(const std::string& groupName, Model* model);
    void BindAnimationModel(const std::string& groupName, AnimationModel* model);
    void BindSprite(const std::string& groupName, Sprite* sprite);
    // bindContent = false にすると文字列は登録しない (スコアなど、コードから毎フレーム書き換える文字用)
    void BindTextElement(const std::string& groupName, Text* text, bool bindContent = true);

    // UI描画
    // 項目を右クリックすると、最後に保存した値・初期値に戻すメニューが出る
    bool Draw(const std::string& key, const std::string& name = "", std::source_location location = std::source_location::current());
    // 一括で登録順に描画する関数
    void DrawAll();
    bool DrawModel(const std::string& groupName, const std::string& customLabel = "");
    void DrawAnimationModel(const std::string& groupName, const std::string& customLabel = "");
    void DrawSprite(const std::string& groupName, const std::string& customLabel = "");
    void DrawTextElement(const std::string& groupName, const std::string& customLabel = "");

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

    // key が保存されている場所を表す文字列
    std::string DescribeKey(const std::string& key) const;

    std::string MakeLabel(const std::string& key, const std::string& nameOverride) const {
        return (nameOverride.empty() ? key : nameOverride) + "###" + key;
    }

    // 元に戻せる項目として登録する
    void RegisterRevert(const std::string& key, std::any defaultValue, std::function<void()> applyFromStore);
    // 右クリックのメニュー
    bool DrawRevertMenu(const std::string& key);
    // GlobalVariables に値を入れ、変数にも反映する
    void ApplyStoredValue(const std::string& key, const std::any& value, bool recordUndo);

    void BindBoolInternal(const std::string& key, int32_t* ptr, bool defaultValue);
    void BindVector3Internal(const std::string& key, Vector3* ptr, const Vector3& defaultValue, float speed, float min, float max, std::function<void()> onChange);
    void BindMaterialProperties(const std::string& prefix, MaterialHandle* handle);

    // 9 方向のボタンで画面基準と基準点をまとめて設定する
    bool DrawAnchorPreset(const std::string& screenAnchorKey, Vector2* screenAnchor, const std::string& pivotKey, Vector2* pivot);

    template <typename ModelType>
    bool DrawMaterialUI(ModelType* targetModel, const std::string& prefix, GlobalVariables* gv, const std::vector<std::string>& groupPath);

private:
    Engine* engine_ = nullptr;
    std::vector<std::string> groupPath_;
    std::vector<std::string> keys_;
    std::unordered_map<std::string, std::function<bool(const std::string&)>> items_;
    // BindRotation の編集・保存用のオイラー角
    std::map<std::string, Vector3> rotationEulers_;

    struct ModelBindInfo { Model* model = nullptr; };
    struct AnimationBindInfo { AnimationModel* model = nullptr; };
    struct TextBindInfo { Text* text = nullptr; bool bindContent = true; };

    std::unordered_map<std::string, ModelBindInfo> modelBindMap_;
    std::unordered_map<std::string, AnimationBindInfo> animationBindMap_;
    std::unordered_map<std::string, Sprite*> spriteBindMap_;
    std::unordered_map<std::string, TextBindInfo> textBindMap_;

    // まとめて元に戻す機能
    struct SavedValueChange
    {
        PropertyBinder* binder = nullptr;
        std::string key;
        std::any savedValue;
        std::any currentValue;
    };
    static std::vector<PropertyBinder*>& GetInstances();
    // 確認の一覧に出している、戻す予定の値
    static std::vector<SavedValueChange>& GetPendingChanges();
    // まとめて戻す前の値
    static std::vector<SavedValueChange>& GetBulkUndo();
    static std::vector<SavedValueChange> CollectChangesFromSaved(const std::vector<std::string>& groupPath);

    // 元に戻す機能
    struct RevertInfo
    {
        std::any defaultValue;
        std::function<void()> applyFromStore;
    };
    std::unordered_map<std::string, RevertInfo> revertInfos_;
    // 戻す前の値
    std::unordered_map<std::string, std::any> undoValues_;
    // メニューを開いたときにファイルから読んだ値
    std::string revertMenuKey_;
    std::optional<std::any> revertMenuSavedValue_;
};

}

