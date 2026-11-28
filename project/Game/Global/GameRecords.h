#pragma once
#include <string>

// ステージごとの最高記録
namespace GameRecords
{

// まだ記録がないステージは 0
int GetBestMoney(const std::string& stageName);
// 今回の金額が記録を超えていれば書き換え
bool SubmitMoney(const std::string& stageName, int money);

}
