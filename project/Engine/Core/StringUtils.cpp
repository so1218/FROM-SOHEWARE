#include "StringUtils.h"
#include <Windows.h>

// string->wstring
std::wstring StringUtils::ConvertString(const std::string& str)
{
    if (str.empty()) {
        return std::wstring();
    }

    auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
    if (sizeNeeded == 0) {
        return std::wstring();
    }
    std::wstring result(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
    return result;
}

// wstring->string
std::string StringUtils::ConvertString(const std::wstring& str) 
{
    if (str.empty()) {
        return std::string();
    }

    auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
    if (sizeNeeded == 0) {
        return std::string();
    }
    std::string result(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
    return result;
}

std::string StringUtils::Trim(const std::string& str)
{
    std::string s = str;
    // 改行コード削除
    if (!s.empty() && s.back() == '\r') s.pop_back();

    // 前方の空白削除
    size_t first = s.find_first_not_of(" \t\"");
    if (std::string::npos == first) return ""; // 空白のみの場合

    // 後方の空白削除
    size_t last = s.find_last_not_of(" \t\"");
    return s.substr(first, (last - first + 1));
}

