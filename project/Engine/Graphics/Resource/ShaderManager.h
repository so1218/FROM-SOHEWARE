#pragma once
#include "StringUtils.h"

namespace FE
{

namespace fs = std::filesystem;

class ShaderManager
{
public:
    void Initialize(
        IDxcUtils* dxcUtils,
        IDxcCompiler3* dxcCompiler,
        IDxcIncludeHandler* includeHandler
    );

    // ファイルパスとプロファイルからシェーダーBlobを取得する
    IDxcBlob* GetShader(const std::wstring& relativePath, const wchar_t* profile);

private:
    // コンパイル処理
    Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(
        const std::wstring& filePath,
        const wchar_t* profile
    );

    // Blobをファイルに保存する
    void SaveBlob(const std::wstring& path, IDxcBlob* blob) 
    {
        fs::create_directories(fs::path(path).parent_path()); // フォルダがなければ作成
        std::ofstream ofs(path, std::ios::binary);
        ofs.write(static_cast<const char*>(blob->GetBufferPointer()), blob->GetBufferSize());
    }

    // ファイルからBlobを読み込む
    Microsoft::WRL::ComPtr<IDxcBlob> LoadBlob(const std::wstring& path, IDxcUtils* dxcUtils) 
    {
        Microsoft::WRL::ComPtr<IDxcBlobEncoding> blob;
        dxcUtils->LoadFile(path.c_str(), nullptr, &blob);
        return blob;
    }

    // DXC関連
    IDxcUtils* dxcUtils_ = nullptr;
    IDxcCompiler3* dxcCompiler_ = nullptr;
    IDxcIncludeHandler* includeHandler_ = nullptr;

    // シェーダーキャッシュ
    std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<IDxcBlob>> shaderCache_;
};

}