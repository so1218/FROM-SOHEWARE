#pragma once
#define NOMINMAX 
#include <string>
#include <ostream>
#include <Windows.h>
#include <fstream> 
#include <source_location>
#include <mutex>
#include <chrono>
#include <iostream>
#include <filesystem>
#include <format>

// ログレベル
enum class LogLevel 
{
    Debug,   
    Info,    
    Warning, 
    Error
};

class Logger
{
public:
    static Logger& Instance()
    {
        static Logger instance;
        return instance;
    }

    void Initialize();

    // フォーマット対応のログ出力関数
    template<typename... Args>
    void Log(LogLevel level, std::source_location location, const std::format_string<Args...>& format, Args&&... args);

    // クラッシュ時に呼び出すダンプ出力
    static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception);

    // ログレベルを設定する関数
    void SetLogLevel(LogLevel level) { minLevel_ = level; }


private:
    Logger() = default;
    ~Logger() 
    {
        if (logStream_.is_open()) 
        {
            logStream_.close();
        }
    }
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // ログレベルを文字列に変換
    std::string LevelToString(LogLevel level);
    // コンソールの文字色を変更
    void SetConsoleColor(LogLevel level);

    std::ofstream logStream_;
    std::mutex mutex_; // ログ出力の排他制御用

    // 出力する最低レベル
    LogLevel minLevel_ = LogLevel::Info;

    // ANSIエスケープコードを返すヘルパー
    std::string GetAnsiColorCode(LogLevel level);
    // 色をリセットするコード
    const std::string ANSI_RESET = "\x1B[0m";
};

// Log関数
template<typename... Args>
void Logger::Log(LogLevel level, std::source_location location, const std::format_string<Args...>& format, Args&&... args)
{
    if (level < minLevel_) return;
    std::lock_guard<std::mutex> lock(mutex_);

    // ユーザーメッセージ部分をフォーマット
    const std::string userMessage = std::format(format, std::forward<Args>(args)...);

    auto now = std::chrono::system_clock::now();

    // スレッドIDを文字列に変換
    std::stringstream ss;
    ss << std::this_thread::get_id();
    const std::string threadIdStr = ss.str();

    const std::string logMessage = std::format(
        "[{:%Y-%m-%d %H:%M:%S}] [{}] [{}] [{}:{}] {}",
        now,
        threadIdStr,
        LevelToString(level),
        std::filesystem::path(location.file_name()).filename().string(),
        location.line(),
        userMessage
    );

    if (logStream_.is_open())
    {
        logStream_ << logMessage << std::endl;
    }

    // デバッグ出力
    OutputDebugStringA((logMessage + "\n").c_str());

    // コンソール出力 
    SetConsoleColor(level);
}

// 便利な呼び出しマクロ
// std::source_location::current() をマクロ内で呼び出すことで、呼び出し元の情報を取得できる
#ifdef _DEBUG 

#define LOG_DEBUG(...)   Logger::Instance().Log(LogLevel::Debug,   std::source_location::current(), __VA_ARGS__)
#define LOG_INFO(...)    Logger::Instance().Log(LogLevel::Info,    std::source_location::current(), __VA_ARGS__)
#define LOG_WARN(...)    Logger::Instance().Log(LogLevel::Warning, std::source_location::current(), __VA_ARGS__)
#define LOG_ERROR(...)   Logger::Instance().Log(LogLevel::Error,   std::source_location::current(), __VA_ARGS__)

#else 

    // マクロを「何もしない」式に置き換える
#define LOG_DEBUG(...)   ((void)0)
#define LOG_INFO(...)    ((void)0)
#define LOG_WARN(...)    ((void)0)
#define LOG_ERROR(...)   Logger::Instance().Log(LogLevel::Error,   std::source_location::current(), __VA_ARGS__)

#endif