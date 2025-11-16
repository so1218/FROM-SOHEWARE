#include "ShaderManager.h"
#include "Logger.h"

void ShaderManager::Initialize(
    IDxcUtils* dxcUtils,
    IDxcCompiler3* dxcCompiler,
    IDxcIncludeHandler* includeHandler)
{
    // 引数で受け取ったポインタをメンバ変数にコピー
    dxcUtils_ = dxcUtils;
    dxcCompiler_ = dxcCompiler;
    includeHandler_ = includeHandler;
}

Microsoft::WRL::ComPtr<IDxcBlob> ShaderManager::CompileShader(
    const std::wstring& filePath,
    const wchar_t* profile)
{
    // 1.hlslファイルを読む

    // これからシェーダーをコンパイルする旨をログに出す
    LOG_INFO("Begin CompileShader, path:{}, profile:{}",
        StringUtils::ConvertString(filePath),
        StringUtils::ConvertString(profile)
    );
    // hlslファイルを読む
    IDxcBlobEncoding* shaderSource = nullptr;

    // ★変更点 1: 引数の dxcUtils ではなく、メンバ変数の dxcUtils_ を使う
    HRESULT hr = dxcUtils_->LoadFile(filePath.c_str(), nullptr, &shaderSource);

    // 読めなかったら止める
    assert(SUCCEEDED(hr));
    // 読み込んだファイルの内容を設定する
    DxcBuffer shaderSourceBuffer;
    shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
    shaderSourceBuffer.Size = shaderSource->GetBufferSize();
    shaderSourceBuffer.Encoding = DXC_CP_UTF8;// UTF8の文字コードであることを通知

    // 2.Compileする

    LPCWSTR arguments[] =
    {
        filePath.c_str(),
        L"-E", L"main",  // main 関数をエントリーポイントとして指定
        L"-T", profile,  // シェーダープロファイルの指定
        L"-Zi", L"-Qembed_debug",  // デバッグ情報を埋め込むオプション
        L"-Od",  // 最適化を外す
        L"-Zpr"  // メモリレイアウトの指定
    };
    // 実際にShaderをコンパイルする
    IDxcResult* shaderResult = nullptr;

    // ★変更点 2: 引数の dxcCompiler ではなく、メンバ変数の dxcCompiler_ を使う
    // ★変更点 3: 引数の includeHandler ではなく、メンバ変数の includeHandler_ を使う
    hr = dxcCompiler_->Compile(
        &shaderSourceBuffer,// 読み込んだファイル
        arguments,// コンパイルオプション
        _countof(arguments),// コンパイルオプションの数
        includeHandler_,// includeが含まれた諸々
        IID_PPV_ARGS(&shaderResult)// コンパイル結果
    );

    // コンパイルエラーではなくdxcが起動できないなど致命的な状況
    assert(SUCCEEDED(hr));

    // 3.警告・エラーが出ていない

    //警告・エラーが出てたらログを出して止める
    IDxcBlobUtf8* shaderError = nullptr;
    shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
    if (shaderError != nullptr && shaderError->GetStringLength() != 0)
    {
        LOG_ERROR("Shader Compile Error: {}", shaderError->GetStringPointer());
        // 警告・エラーダメゼッタイ
        assert(false);
    }

    // 4.Compile結果を受け取って返す

    // コンパイル結果から実行用のバイナリ部分を取得
    // (ComPtrで受けるようにすると、最後のReleaseが不要になり安全です)
    Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = nullptr;
    hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
    assert(SUCCEEDED(hr));
    // 成功したログを出す
    LOG_INFO("Compile Succeeded, path:{}, profile:{}",
        StringUtils::ConvertString(filePath),
        StringUtils::ConvertString(profile)
    );

    // もう使わないリソースを開放
    shaderSource->Release();
    shaderResult->Release();
    // (shaderError は解放不要な場合がありますが、必要ならここで解放)
    // if (shaderError) { shaderError->Release(); }

    // 実行用のバイナリを返却
    // ComPtr<IDxcBlob> を GetShader() に返す
    return shaderBlob;
}

IDxcBlob* ShaderManager::GetShader(const std::wstring& filePath, const wchar_t* profile)
{
    // キャッシュキーを生成 (例: "Object3D.VS.hlsl_vs_6_0")
    std::wstring cacheKey = filePath + L"_" + profile;

    // 1. キャッシュを検索
    if (auto it = shaderCache_.find(cacheKey); it != shaderCache_.end()) {
        return it->second.Get(); // 見つかった
    }

    // 2. 見つからないのでコンパイル (CompileShaderはプライベート関数)
    Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = CompileShader(filePath, profile);
    assert(shaderBlob != nullptr && "Shader compilation failed!");

    // 3. キャッシュに保存
    shaderCache_[cacheKey] = shaderBlob;

    return shaderBlob.Get();
}