#pragma once

class Engine;
class Player;

enum class WeaponType
{
    // ここにゲームに登場する武器をすべて列挙する
    Knife,
    Garlic,
    Axe,
    Bible,
    FireWand,
    MagicMissile
};

class Weapon
{
protected:
    Engine* engine_; // エンジン（描画やオブジェクト生成に使う）
    Player* owner_;  // この武器の持ち主

    int level_ = 1;
    float damage_ = 10.0f;
    float cooldown_ = 2.0f;     // 攻撃のクールダウン時間
    float cooldownTimer_ = 0.0f; // 現在のタイマー
    int projectileCount_ = 1;
    float areaSize_ = 1.0f;

public:
    Weapon(Engine* engine, Player* owner);
    virtual ~Weapon() {} // 仮想デストラクタ（超重要）

    // 武器ごとの固有ロジックは、この2つに書かせる
    virtual void Update(float deltaTime) = 0;
    virtual void Draw() = 0;
    virtual void DebugDraw() = 0;
    virtual void LevelUp() = 0;

    void SetLevel(int level) { level_ = level; }
};