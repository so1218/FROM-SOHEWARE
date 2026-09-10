#pragma once

// 状態の基底クラス
template <class T>
class State 
{
public:
    virtual ~State() = default;
    virtual void Enter(T* owner) {}  // 状態に入った瞬間
    virtual void Update(T* owner) = 0;  // 毎フレーム更新
    virtual void Exit(T* owner) {} // 状態から抜ける瞬間

    // ステート名を取得
    virtual std::string GetName() = 0;
};

// ステートを管理するマシン
template <class T>
class StateMachine 
{
public:
    explicit StateMachine(T* owner) : owner_(owner), currentState_(nullptr) {}

    // 状態を変更する
    void ChangeState(State<T>* newState) 
    {
        if (!newState || currentState_ == newState) return;

        if (currentState_) currentState_->Exit(owner_);
        currentState_ = newState;
        if (currentState_) currentState_->Enter(owner_);
    }

    // 現在の状態を更新する
    void Update()
    {
        if (currentState_) currentState_->Update(owner_);
    }

    std::string GetCurrentStateName() const
    {
        if (currentState_) 
        {
            return currentState_->GetName();
        }
        return "None";
    }

private:
    T* owner_;
    State<T>* currentState_;
};