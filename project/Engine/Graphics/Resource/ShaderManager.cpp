#include "ShaderManager.h"
#include "Logger.h"

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
    LOG_INFO("Begin CompileShader, path:{}, profile:{}",
        StringUtils::ConvertString(filePath),
        StringUtils::ConvertString(profile));

    // HLSLファイル読み込み
    IDxcBlobEncoding* shaderSource = nullptr;
    HRESULT hr = dxcUtils_->LoadFile(filePath.c_str(), nullptr, &shaderSource);
    assert(SUCCEEDED(hr));

    DxcBuffer shaderSourceBuffer{
        shaderSource->GetBufferPointer(),
        shaderSource->GetBufferSize(),
        DXC_CP_UTF8
    };

    // シェーダーコンパイル設定
    LPCWSTR arguments[] =
    {
        filePath.c_str(),
        L"-E", L"main",        // エントリーポイント
        L"-T", profile,        // シェーダープロファイル
        L"-Zi", L"-Qembed_debug", // デバッグ情報
        L"-Od",                // 最適化無効
        L"-Zpr"                // メモリレイアウト: row-major
    };

    // コンパイル実行
    IDxcResult* shaderResult = nullptr;
    hr = dxcCompiler_->Compile(
        &shaderSourceBuffer,
        arguments,
        _countof(arguments),
        includeHandler_,
        IID_PPV_ARGS(&shaderResult));
    assert(SUCCEEDED(hr));

    // エラー確認
    IDxcBlobUtf8* shaderError = nullptr;
    shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
    if (shaderError && shaderError->GetStringLength() != 0)
    {
        LOG_ERROR("Shader Compile Error: {}", shaderError->GetStringPointer());
        assert(false);
    }

    // コンパイル結果取り出し
    Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = nullptr;
    hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
    assert(SUCCEEDED(hr));

    LOG_INFO("Compile Succeeded, path:{}, profile:{}",
        StringUtils::ConvertString(filePath),
        StringUtils::ConvertString(profile));

    // 後処理
    shaderSource->Release();
    shaderResult->Release();

    return shaderBlob;
}

IDxcBlob* ShaderManager::GetShader(const std::wstring& filePath, const wchar_t* profile)
{
    // キャッシュキー作成
    std::wstring cacheKey = filePath + L"_" + profile;

    // キャッシュ確認
    if (auto it = shaderCache_.find(cacheKey); it != shaderCache_.end()) {
        return it->second.Get();
    }

    // 無ければコンパイル
    Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = CompileShader(filePath, profile);
    assert(shaderBlob != nullptr);

    // キャッシュへ保存
    shaderCache_[cacheKey] = shaderBlob;

    return shaderBlob.Get();
}