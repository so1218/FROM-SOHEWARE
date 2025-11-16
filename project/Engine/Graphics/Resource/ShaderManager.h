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
    // コンストラクタでDXCオブジェクトを受け取る
    void Initialize(
        IDxcUtils* dxcUtils,
        IDxcCompiler3* dxcCompiler,
        IDxcIncludeHandler* includeHandler
    );

    // ファイルパスとプロファイルからシェーダーBlobを取得する
    // 内部でキャッシュをチェックし、なければコンパイルする
    IDxcBlob* GetShader(const std::wstring& filePath, const wchar_t* profile);

private:
    // コンパイル処理
    Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(
        const std::wstring& filePath,
        const wchar_t* profile
    );

    // DXC関連 (PSOManagerから移動)
    IDxcUtils* dxcUtils_ = nullptr;
    IDxcCompiler3* dxcCompiler_ = nullptr;
    IDxcIncludeHandler* includeHandler_ = nullptr;

    // シェーダーキャッシュ
    // キーは "filePath_profile" のような一意な文字列
    std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<IDxcBlob>> shaderCache_;
};