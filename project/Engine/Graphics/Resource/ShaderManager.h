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

    // DXC関連
    IDxcUtils* dxcUtils_ = nullptr;
    IDxcCompiler3* dxcCompiler_ = nullptr;
    IDxcIncludeHandler* includeHandler_ = nullptr;

    // シェーダーキャッシュ
    std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<IDxcBlob>> shaderCache_;
};