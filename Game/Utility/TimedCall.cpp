#include "TimedCall.h"

void TimedCall::Update()
{
	if (isFinished_) return; // 既に完了している場合は何もしない
	// 時間を減少
	time_ -= 1.0f / 60.0f; // 1秒あたり60フレームと仮定
	// 時間が0以下になったらコールバックを実行
	if (time_ <= 0.0f)
	{
		callback_(); // コールバック関数を呼び出す
		isFinished_ = true; // 完了フラグを立てる
	}
}