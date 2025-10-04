#pragma once

#include <functional>

/// <summary>
/// 時限発動
/// </summary>
class TimedCall
{
public:
	// コンストラクタ
	TimedCall(std::function<void()> callback, float time)
		: callback_(callback), time_(time) {}
	// 更新処理
	void Update();
	// 完了ならtrueを変えす
	bool IsFinished() const { return isFinished_; }

	// タイマーをリセットするメソッドを追加
	void Reset(float newTime) 
	{
		time_ = newTime;
		isFinished_ = false;
	}

private:
	std::function<void()> callback_; // コールバック関数
	float time_; // 時間
	bool isFinished_ = false; // 完了フラグ
};

