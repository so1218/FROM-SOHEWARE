#pragma once
#include "Engine.h"
#include "ModelManager.h"
#include "AnimationManager.h"
#include "TextureManager.h"
#include "Sprite.h"
#include "Model.h"
#include "AnimationModel.h"

namespace FE
{ 

namespace UpdateOrder
{
    enum Priority
    {
        Field = 10,
        Player = 30,
        Default = 30,
        Effect = 60,
        UI = 90,
    };
}

class GameObject 
{
public:
    GameObject(Engine* engine, int priority = UpdateOrder::Default);
    virtual ~GameObject() = default;

    virtual void Initialize() {}
    virtual void Update() {}
    virtual void Draw() {}
    virtual void DebugDraw() {}

    // 生存フラグ
    bool IsDead() const { return isDead_; }
    void Destroy() { isDead_ = true; }

    // タグの取得と設定
    void SetTag(const std::string& tag) { tag_ = tag; }
    const std::string& GetTag() const { return tag_; }

    // 特定のタグかどうか判定する関数
    bool CompareTag(std::string_view tag) const { return tag_ == tag; }

    int GetUpdatePriority() const { return priority_; }

protected:
    std::unique_ptr<Model> CreateModel(const std::string& modelName);
    std::unique_ptr<AnimationModel> CreateAnimationModel(const std::string& modelName, const std::string& animationName);
    std::unique_ptr<Sprite> CreateSprite(const std::string& textureName);

    Engine* engine_ = nullptr;
    std::string tag_ = "None";
    int priority_ = 0;
    bool isDead_ = false;
};

}