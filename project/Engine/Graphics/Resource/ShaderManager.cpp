#include "pch.h"
#include "ShaderManager.h"
#include "Logger.h"

namespace FE
{

void ShaderManager::Initialize(
    IDxcUtils* dxcUtils,
    IDxcCompiler3* dxcCompiler,
    IDxcIncludeHandler* includeHandler)
{
    dxcUtils_ = dxcUtils;
    dxcCompiler_ = dxcCompiler;
    includeHandler_ = includeHandler;
}

Microsoft::WRL::ComPtr<IDxcBlob> ShaderManager::CompileShader(
    const std::wstring& filePath,
    const wchar_t* profile)
{
    // Assets/Shaders の絶対パスを取得
    std::wstring absoluteShaderDir = fs::absolute(L"Assets/Shaders").wstring();

    // 引数の動的な構築
    std::vector<LPCWSTR> arguments = {
        filePath.c_str(),
        L"-E", L"main",
        L"-T", profile,
        L"-Zpr", // row-major
        L"-I", absoluteShaderDir.c_str() // 絶対パスでインクルード検索ルートを指定
    };

#ifdef ENABLE_DEV_TOOLS
    arguments.push_back(L"-Zi");          // デバッグ情報
    arguments.push_back(L"-Qembed_debug");
    arguments.push_back(L"-Od");          // 最適化無効
#else
    arguments.push_back(L"-O3");          // 最大最適化
#endif

    // ソースの読み込み
    Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSource;
    HRESULT hr = dxcUtils_->LoadFile(filePath.c_str(), nullptr, &shaderSource);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to load shader file: {}", StringUtils::ConvertString(filePath));
        return nullptr;
    }

    DxcBuffer shaderSourceBuffer{
        shaderSource->GetBufferPointer(),
        shaderSource->GetBufferSize(),
        DXC_CP_UTF8
    };

    // コンパイル実行
    Microsoft::WRL::ComPtr<IDxcResult> shaderResult;
    hr = dxcCompiler_->Compile(
        &shaderSourceBuffer,
        arguments.data(),
        static_cast<uint32_t>(arguments.size()),
        includeHandler_,
        IID_PPV_ARGS(&shaderResult));

    // エラー詳細の取得
    if (SUCCEEDED(hr)) {
        Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError;
        shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
        if (shaderError && shaderError->GetStringLength() != 0)
        {
            // アサートせずにログを出すだけにして、ゲームを落とさず修正
            LOG_ERROR("Shader Compile Error in [{}]:\n{}",
                StringUtils::ConvertString(filePath),
                shaderError->GetStringPointer());
            return nullptr; // 失敗時はnullptrを返す
        }
    }

    // 結果の取り出し
    Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob;
    hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
    if (FAILED(hr)) return nullptr;

    return shaderBlob;
}

IDxcBlob* ShaderManager::GetShader(const std::wstring& relativePath, const wchar_t* profile)
{
    // キャッシュキーは受け取った相対パスで管理
    std::wstring cacheKey = relativePath + L"_" + profile;
    if (auto it = shaderCache_.find(cacheKey); it != shaderCache_.end()) {
        return it->second.Get();
    }

    // ベースパスの結合
    std::wstring fullPath = L"Assets/Shaders/" + relativePath;

    // パス区切り文字を置換してキャッシュ名の衝突を防ぐ
    std::wstring safeFileName = relativePath;
    for (auto& ch : safeFileName) {
        if (ch == L'/' || ch == L'\\') ch = L'_';
    }
    std::wstring cacheFileName = safeFileName + L"_" + profile + L".cso";
    std::wstring cachePath = L"Assets/Shaders/Cache/" + cacheFileName;

    Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob;

#ifdef ENABLE_DEV_TOOLS
    // 開発モード .hlsl の更新を監視してコンパイル
    bool shouldCompile = true;
    if (fs::exists(cachePath) && fs::exists(fullPath))
    {
        auto srcTime = fs::last_write_time(fullPath);
        auto cacheTime = fs::last_write_time(cachePath);
        if (cacheTime > srcTime) shouldCompile = false;
    }

    if (shouldCompile)
    {
        auto newBlob = CompileShader(fullPath, profile);
        if (newBlob) {
            shaderBlob = newBlob;
            SaveBlob(cachePath, shaderBlob.Get());
        }
        else {
            if (fs::exists(cachePath)) {
                LOG_ERROR("コンパイル失敗。前回成功したキャッシュを使用: {}", StringUtils::ConvertString(relativePath));
                MessageBeep(MB_ICONERROR);
                shaderBlob = LoadBlob(cachePath, dxcUtils_);
            }
            else {
                LOG_ERROR("コンパイルに失敗し、キャッシュも存在しません: {}", StringUtils::ConvertString(relativePath));
                return nullptr;
            }
        }
    }
    else
    {
        shaderBlob = LoadBlob(cachePath, dxcUtils_);
    }

#else

    // 製品モード.csoを読み込むだけ
    // 製品版には.hlslが無いので、直接Cacheフォルダの.csoを読む
    shaderBlob = LoadBlob(cachePath, dxcUtils_);

    // 提出物に.csoを入れ忘れていた場合はエラー終了
    if (!shaderBlob)
    {
        // 製品版でassertは消えることが多く、致命的エラーとして扱う処理を入れる
        MessageBoxW(nullptr, L"シェーダーファイル(.cso)が見つからない", L"Fatal Error", MB_OK | MB_ICONERROR);
        exit(1);
    }
#endif

    if (shaderBlob)
    {
        shaderCache_[cacheKey] = shaderBlob;
        return shaderBlob.Get();
    }
    return nullptr;
}

}