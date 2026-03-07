#pragma once

#include "StringUtils.h"

#include <d3d12.h>
#include <dxcapi.h>
#include <string>
#include <memory>
#include <cassert>
#include <iostream>
#include <format>
#include <wrl.h>
#include <unordered_map>
#include <fstream>
#include <filesystem>

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
    IDxcBlob* GetShader(const std::wstring& filePath, const wchar_t* profile);

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