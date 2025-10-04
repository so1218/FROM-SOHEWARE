#include "Logger.h"
#include <strsafe.h>
#include <DbgHelp.h>
        
#include <filesystem>          
#include <chrono>              
#include <format>              
#include <string> 
#include <fstream> 

#pragma comment(lib, "Dbghelp.lib")

void Logger::Initialize()
{
    // 誰も捕捉しなかった場合に、補足する関数を登録
    SetUnhandledExceptionFilter(Logger::ExportDump);

     // ログのディレクトリを用意
    std::filesystem::create_directory("logs");

    // 現在時刻を取得
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> nowSeconds =
    std::chrono::time_point_cast<std::chrono::seconds>(now);
    std::chrono::zoned_time localTime{ std::chrono::current_zone(), nowSeconds };

    // formatを使って年月日_時分秒の文字列に変換
    std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);
    std::string logFilePath = std::string("logs/") + dateString + ".log";

    // ファイルを作って書き込み準備
    logStream_.open(logFilePath);
}

void Logger::Log(const std::string& message, std::ostream& os)
{
    os << message << std::endl;
    OutputDebugStringA((message + "\n").c_str());
}

void Logger::Log(const std::string& message)
{
    OutputDebugStringA((message + "\n").c_str());
}

LONG WINAPI Logger::ExportDump(EXCEPTION_POINTERS* exception)
{
    SYSTEMTIME time;
    GetLocalTime(&time);

    wchar_t filePath[MAX_PATH] = { 0 };
    CreateDirectory(L"./Dumps", nullptr);  // Dumps ディレクトリがなければ作成

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

    if (dumpFileHandle == INVALID_HANDLE_VALUE) {
        OutputDebugStringA("Failed to create dump file.\n");
        return EXCEPTION_EXECUTE_HANDLER;
    }

    MINIDUMP_EXCEPTION_INFORMATION dumpInfo = {};
    dumpInfo.ThreadId = GetCurrentThreadId();
    dumpInfo.ExceptionPointers = exception;
    dumpInfo.ClientPointers = TRUE;

    BOOL success = MiniDumpWriteDump(
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