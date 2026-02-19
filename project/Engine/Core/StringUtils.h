#pragma once
#include <string>

class StringUtils 
{
public:
    // string->wstring
    static std::wstring ConvertString(const std::string& str);

    // wstring->string
    static std::string ConvertString(const std::wstring& wstr);

    // 文字列の前後の空白・改行・ダブルクォートを削除する関数
    static std::string Trim(const std::string& str);
};

