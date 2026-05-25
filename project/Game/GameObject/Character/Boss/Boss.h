#pragma once
#include "Collider.h"
#include "AnimationModel.h"
#include "FollowCamera.h"
#include "PropertyBinder.h"
#include "GameObject.h"
#include "StateMachine.h"

class BossStateNormal;
class BossStateApproach;
class BossStateJumpAttack;
class Player;

class Boss : public FE::GameObject
{
	friend class BossStateIdle;
	friend class BossStateApproach;
	friend class BossStateJumpAttack;

public:
    Boss(FE::Engine* engine, Player* player);
    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

    // プレイヤーへの方向ベクトルを取得
    FE::Vector3 GetDirectionToPlayer();
    float GetDistanceToPlayer();

    // 旋回処理
    void RotateTowards(const FE::Vector3& direction, float speed);

private:
    FE::Engine* engine_;
    Player* player_ = nullptr; // 追跡対象
    std::unique_ptr<FE::AnimationModel> animation_;
    std::unique_ptr<FE::PropertyBinder> binder_;
    std::unique_ptr<StateMachine<Boss>> stateMachine_;

    // 調整用パラメータ
    float walkSpeed_ = 0.05f;      // Idle時の歩き
    float runSpeed_ = 0.15f;       // Approach時の走り
    float jumpAttackSpeed_ = 0.6f; // 飛び込み速度
    float rotateSpeed_ = 5.0f;     // 旋回速度

    float meleeRange_ = 5.0f;      // 近接攻撃に入る距離
    float jumpAttackRange_ = 15.0f; // 飛び込みを開始する距離
    float actionTimer_ = 0.0f;

    // 飛び込み攻撃などでターゲット方向を保持するための変数
    FE::Vector3 targetDir_ = { 0.0f, 0.0f, 1.0f };
};

