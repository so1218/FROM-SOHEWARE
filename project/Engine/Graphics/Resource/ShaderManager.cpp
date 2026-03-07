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
    // 引数の動的な構築
    std::vector<LPCWSTR> arguments = {
        filePath.c_str(),
        L"-E", L"main",
        L"-T", profile,
        L"-Zpr", // row-major
    };

#ifdef IS_DEVELOPMENT
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

IDxcBlob* ShaderManager::GetShader(const std::wstring& filePath, const wchar_t* profile) 
{
    // キャッシュキー（メモリ管理用）
    std::wstring cacheKey = filePath + L"_" + profile;
    if (auto it = shaderCache_.find(cacheKey); it != shaderCache_.end()) {
        return it->second.Get();
    }

    // キャッシュファイルの保存先を決定
    fs::path srcPath(filePath);
    std::wstring cacheFileName = srcPath.filename().wstring() + L"_" + profile + L".cso";
    std::wstring cachePath = srcPath.parent_path().wstring() + L"/Cache/" + cacheFileName;

    bool shouldCompile = true;

    // タイムスタンプ比較
    if (fs::exists(cachePath) && fs::exists(filePath))
    {
        auto srcTime = fs::last_write_time(filePath);
        auto cacheTime = fs::last_write_time(cachePath);

        if (cacheTime > srcTime)
        {
            shouldCompile = false; // キャッシュの方が新しく、コンパイル不要
        }
    }

    Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob;
    if (shouldCompile) 
    {
        auto newBlob = CompileShader(filePath, profile);
        if (newBlob)
        {
            // コンパイル成功
            // 新しいBlobを使って保存
            shaderBlob = newBlob;
            SaveBlob(cachePath, shaderBlob.Get());
        }
        else
        {
            // コンパイル失敗
            if (fs::exists(cachePath))
            {
                // 古いキャッシュが残っていれば、それを読んでゲームを止めない
                shaderBlob = LoadBlob(cachePath, dxcUtils_);
            }
            else 
            {
                // キャッシュも無い場合
                return nullptr;
            }
        }
    }
    else
    {
        shaderBlob = LoadBlob(cachePath, dxcUtils_); // ディスクから一瞬でロード
    }

    if (shaderBlob) 
    {
        shaderCache_[cacheKey] = shaderBlob;
        return shaderBlob.Get();
    }
    return nullptr;
}