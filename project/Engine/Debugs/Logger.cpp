#include "pch.h"
#include "Logger.h"
#include <strsafe.h>
#include <DbgHelp.h>

#pragma comment(lib, "Dbghelp.lib")

void Logger::Initialize()
{
    // 未処理例外のハンドラを登録
    SetUnhandledExceptionFilter(Logger::ExportDump);

    LOG_INFO("Logger::Initialize started. Setting unhandled exception filter.");

    // ログディレクトリを作成
    try {
        std::filesystem::create_directory("logs");
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        // デバッグ出力にのみ記録
        LOG_ERROR("Failed to create 'logs' directory: {}", e.what());
    }

    // 現在時刻からログファイル名を生成
    auto now = std::chrono::system_clock::now();
    std::string dateString = std::format("logs/{:%Y%m%d_%H%M%S}.log", now);

    // ファイルを開く
    logStream_.open(dateString);

    if (logStream_.is_open())
    {
        LOG_INFO("------------------------------------------------------------");
        LOG_INFO("Logger successfully initialized. Log file opened: {}", dateString);
        LOG_INFO("------------------------------------------------------------");
    }
    else 
    {
        LOG_ERROR("FATAL: Logger failed to open log file: {}", dateString);
    }
}

// ログレベルを文字列に変換
std::string Logger::LevelToString(LogLevel level)
{
    switch (level) 
    {
    case LogLevel::Info:    return "INFO";
    case LogLevel::Warning: return "WARN";
    case LogLevel::Error:   return "ERROR";
    case LogLevel::Debug:   return "DEBUG";
    default:                return "UNKNOWN";
    }
}

// コンソールの文字色を変更 (Windows APIを使用)
void Logger::SetConsoleColor(LogLevel level)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    switch (level)
    {
    case LogLevel::Info:
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // 白
        break;
    case LogLevel::Warning:
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN); // 黄色
        break;
    case LogLevel::Error:
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED); // 赤
        break;
    case LogLevel::Debug:
        SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE); // シアン
        break;
    }
}

// クラッシュダンプ機能 
LONG WINAPI Logger::ExportDump(EXCEPTION_POINTERS* exception)
{
    SYSTEMTIME time;
    GetLocalTime(&time);

    wchar_t filePath[MAX_PATH] = { 0 };
    CreateDirectory(L"./Dumps", nullptr);

    StringCchPrintfW(filePath, MAX_PATH,
        L"./Dumps/%04d%02d%02d-%02d%02d.dmp",
        time.wYear, time.wMonth, time.wDay,
        time.wHour, time.wMinute);

    HANDLE dumpFileHandle = CreateFile(
        filePath,
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (dumpFileHandle == INVALID_HANDLE_VALUE)
    {
        OutputDebugStringA("Failed to create dump file.\n");
        return EXCEPTION_EXECUTE_HANDLER;
    }

    MINIDUMP_EXCEPTION_INFORMATION dumpInfo = {};
    dumpInfo.ThreadId = GetCurrentThreadId();
    dumpInfo.ExceptionPointers = exception;
    dumpInfo.ClientPointers = TRUE;

    MiniDumpWriteDump(
        GetCurrentProcess(),
        GetCurrentProcessId(),
        dumpFileHandle,
        MiniDumpNormal,
        &dumpInfo,
        nullptr,
        nullptr
    );

    return EXCEPTION_EXECUTE_HANDLER;
}

std::string Logger::GetAnsiColorCode(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Info:    return "\x1B[37m"; // 白
    case LogLevel::Warning: return "\x1B[93m"; // 明るい黄色
    case LogLevel::Error:   return "\x1B[91m"; // 明るい赤
    case LogLevel::Debug:   return "\x1B[96m"; // 明るいシアン
    default:                return "\x1B[37m"; // デフォルト (白)
    }
}