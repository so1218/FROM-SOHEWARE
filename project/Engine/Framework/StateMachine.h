#pragma once

// 状態の基底クラス。状態だけで使うデータは、派生クラスのメンバーに持つ
template <class T>
class State
{
public:
    virtual ~State() = default;
    virtual void Enter(T* owner) {}
    virtual void Update(T* owner) = 0;
    virtual void Exit(T* owner) {}

    virtual const char* GetName() const = 0;
};

// 状態は切り替えるたびに作り直すので、前の状態のデータは残らない。
// 切り替えは Update の前後でまとめて行うので、状態の Update の中で切り替えても自分が消えることはない
template <class T>
class StateMachine
{
public:
    explicit StateMachine(T* owner) : owner_(owner) {}

    template <class StateType, class... Arguments>
    void ChangeState(Arguments&&... arguments)
    {
        nextState_ = std::make_unique<StateType>(std::forward<Arguments>(arguments)...);
    }

    void Update()
    {
        ApplyNextState();
        if (currentState_)
        {
            currentState_->Update(owner_);
        }
        ApplyNextState();
    }

    template <class StateType>
    bool IsCurrentState() const
    {
        return dynamic_cast<const StateType*>(currentState_.get()) != nullptr;
    }

    const char* GetCurrentStateName() const
    {
        return currentState_ ? currentState_->GetName() : "None";
    }

private:
    void ApplyNextState()
    {
        if (!nextState_) return;

        if (currentState_)
        {
            currentState_->Exit(owner_);
        }
        currentState_ = std::move(nextState_);
        currentState_->Enter(owner_);
    }

    T* owner_ = nullptr;
    std::unique_ptr<State<T>> currentState_;
    std::unique_ptr<State<T>> nextState_;
};
