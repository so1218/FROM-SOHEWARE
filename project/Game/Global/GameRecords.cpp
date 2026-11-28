#include "pch.h"
#include "GameRecords.h"

namespace
{
std::unordered_map<std::string, int>& GetBestMoneyMap()
{
    static std::unordered_map<std::string, int> bestMoney;
    return bestMoney;
}
}

namespace GameRecords
{

int GetBestMoney(const std::string& stageName)
{
    const auto& bestMoney = GetBestMoneyMap();
    const auto found = bestMoney.find(stageName);
    return found != bestMoney.end() ? found->second : 0;
}

bool SubmitMoney(const std::string& stageName, int money)
{
    if (money <= GetBestMoney(stageName))
    {
        return false;
    }
    GetBestMoneyMap()[stageName] = money;
    return true;
}

}
