#pragma once
#define NOMINMAX 
#include <string>
#include <ostream>
#include <Windows.h>
#include <format>
#include <fstream> 

class Logger 
{
public:
    static Logger& Instance()
    {
        static Logger instance;
        return instance;
    }

    void Initialize();

    // ログ出力
    void Log(const std::string& message, std::ostream& os);
    void Log(const std::string& message);

    // ダンプ出力（クラッシュ時に呼び出す用）
    static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception);

    std::ofstream& GetLogStream() { return logStream_; }

private:
    Logger() = default;
    ~Logger()
    {
        if (logStream_.is_open())
            logStream_.close();
    }
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::ofstream logStream_;
};
