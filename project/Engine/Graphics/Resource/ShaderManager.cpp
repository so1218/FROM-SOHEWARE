#include "pch.h"
#include "ShaderManager.h"
#include "Logger.h"

namespace FS
{

namespace
{

// Debug は最適化せず、シェーダーのデバッガーで 1 行ずつ追えるようにする。
// Development は最適化してコンパイルし、Release はその結果を読む。保存先を分けて、Release が最適化していないものを使わないようにする
#ifdef _DEBUG
const fs::path kShaderCacheDirectory = L"Cache/ShadersDebug";
const wchar_t* const kOptimizationArgument = L"-Od";
#else
const fs::path kShaderCacheDirectory = L"Cache/Shaders";
const wchar_t* const kOptimizationArgument = L"-O3";
#endif

// コンパイルの設定を変えたら書き換える。保存してある設定と違えば、古いキャッシュを消してコンパイルし直す
#ifdef _DEBUG
const std::string kCompileSettings = "-Zpr -Zi -Qembed_debug -Od";
#else
const std::string kCompileSettings = "-Zpr -Zi -Qembed_debug -O3";
#endif

}

// blob をファイルに書き出す
bool SaveBlobToFile(const fs::path& filePath, IDxcBlob* blob)
{
    fs::create_directories(filePath.parent_path());
    std::ofstream ofs(filePath, std::ios::binary);
    if (!ofs.is_open()) return false;
    ofs.write(static_cast<const char*>(blob->GetBufferPointer()), blob->GetBufferSize());
    return true;
}

// ファイルから blob を読み込む
Microsoft::WRL::ComPtr<IDxcBlob> LoadBlobFromFile(const fs::path& filePath, IDxcUtils* dxcUtils)
{
    Microsoft::WRL::ComPtr<IDxcBlobEncoding> blob;
    if (SUCCEEDED(dxcUtils->LoadFile(filePath.c_str(), nullptr, &blob)))
    {
        return blob;
    }
    return nullptr;
}

void ShaderManager::Initialize()
{
    HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
    assert(SUCCEEDED(hr));

    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
    assert(SUCCEEDED(hr));

    hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
    assert(SUCCEEDED(hr));

#ifdef ENABLE_DEV_TOOLS
    // .cso は元のファイルより新しければ使い回すので、設定を変えただけでは作り直されない。設定が違うキャッシュは消す
    const fs::path settingsPath = kShaderCacheDirectory / L"CompileSettings.txt";
    std::string savedSettings;
    if (std::ifstream settingsFile(settingsPath); settingsFile)
    {
        std::getline(settingsFile, savedSettings);
    }
    if (savedSettings != kCompileSettings)
    {
        std::error_code errorCode;
        fs::remove_all(kShaderCacheDirectory, errorCode);
        fs::create_directories(kShaderCacheDirectory);
        std::ofstream(settingsPath) << kCompileSettings;
    }
#endif
}

Microsoft::WRL::ComPtr<IDxcBlob> ShaderManager::CompileShader(const std::wstring& filePath, const wchar_t* profile)
{
    // キャッシュパスの統一
    fs::path srcPath(filePath);
    fs::path relativePath = fs::relative(srcPath, L"Engine/Shaders");
    fs::path csoPath = kShaderCacheDirectory / relativePath;
    csoPath.replace_extension(L".cso");

#ifndef ENABLE_DEV_TOOLS
    // 製品ビルド 直接 Cache を読み込む
    auto csoBlob = LoadBlobFromFile(csoPath, dxcUtils_.Get());
    if (!csoBlob) {
        MessageBoxW(nullptr, L"シェーダーキャッシュ(.cso)が見つかりません", L"Fatal Error", MB_OK | MB_ICONERROR);
        exit(1);
    }
    return csoBlob;
#else
    // 開発ビルド
    bool needCompile = true;
    if (fs::exists(csoPath) && fs::exists(srcPath))
    {
        if (fs::last_write_time(csoPath) >= fs::last_write_time(srcPath))
        {
            needCompile = false; // .cso の方が新しいのでコンパイル不要
        }
    }

    // 最新の CSO があれば即時ロード
    if (!needCompile)
    {
        if (auto cachedBlob = LoadBlobFromFile(csoPath, dxcUtils_.Get()))
        {
            return cachedBlob;
        }
    }

    // DXC によるコンパイル実行
    std::wstring absoluteShaderDir = fs::absolute(L"Engine/Shaders").wstring();
    std::vector<LPCWSTR> arguments = {
        filePath.c_str(),
        L"-E", L"main",
        L"-T", profile,
        L"-Zpr",
        L"-I", absoluteShaderDir.c_str(),
        L"-Zi", L"-Qembed_debug", // PIX などでシェーダーを調べられるようにデバッグ情報を埋め込む
        kOptimizationArgument,
    };

    Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSource;
    if (FAILED(dxcUtils_->LoadFile(filePath.c_str(), nullptr, &shaderSource)))
    {
        LOG_ERROR("Shader file not found: {}", StringUtils::ConvertString(filePath));
        return LoadBlobFromFile(csoPath, dxcUtils_.Get()); // ロード失敗時は過去のキャッシュにフォールバック
    }

    DxcBuffer shaderSourceBuffer{
        shaderSource->GetBufferPointer(),
        shaderSource->GetBufferSize(),
        DXC_CP_UTF8
    };

    Microsoft::WRL::ComPtr<IDxcResult> shaderResult;
    HRESULT hr = dxcCompiler_->Compile(
        &shaderSourceBuffer,
        arguments.data(),
        static_cast<uint32_t>(arguments.size()),
        includeHandler_.Get(),
        IID_PPV_ARGS(&shaderResult));

    // エラーチェック
    if (SUCCEEDED(hr)) {
        Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError;
        shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
        if (shaderError && shaderError->GetStringLength() != 0)
        {
            LOG_ERROR("Shader Compile Error in [{}]:\n{}",
                StringUtils::ConvertString(filePath),
                shaderError->GetStringPointer());

            // コンパイルエラー時: 前回成功した CSO キャッシュがあれば使用
            if (fs::exists(csoPath)) {
                LOG_ERROR("コンパイル失敗。前回の正常なキャッシュを使用します。");
                MessageBeep(MB_ICONERROR);
                return LoadBlobFromFile(csoPath, dxcUtils_.Get());
            }
            return nullptr;
        }
    }

    // コンパイル成功
    Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob;
    hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
    if (SUCCEEDED(hr) && shaderBlob)
    {
        SaveBlobToFile(csoPath, shaderBlob.Get());
        return shaderBlob;
    }

    return nullptr;
#endif
}

IDxcBlob* ShaderManager::GetShader(const std::wstring& relativePath, const wchar_t* profile)
{
    // メモリ上のマップキャッシュ検索
    if (auto it = shaderCache_.find(relativePath); it != shaderCache_.end()) {
        return it->second.Get();
    }

    std::wstring fullPath = L"Engine/Shaders/" + relativePath;

    // CompileShader に一任（ディスクキャッシュの確認・コンパイル・保存・フォールバックを完結）
    auto shaderBlob = CompileShader(fullPath, profile);
    if (shaderBlob)
    {
        shaderCache_[relativePath] = shaderBlob;
        return shaderBlob.Get();
    }

    return nullptr;
}

}