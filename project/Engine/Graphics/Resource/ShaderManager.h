#pragma once
#include "StringUtils.h"

namespace FS
{

namespace fs = std::filesystem;

class ShaderManager
{
public:
    void Initialize();

    // ファイルパスとプロファイルからシェーダーBlobを取得する
    IDxcBlob* GetShader(const std::wstring& relativePath, const wchar_t* profile);

private:
    // コンパイル処理
    Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(const std::wstring& filePath, const wchar_t* profile);

    // DXC関連
    Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_;
    Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_;
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_;

    // シェーダーキャッシュ
    std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<IDxcBlob>> shaderCache_;
};

}