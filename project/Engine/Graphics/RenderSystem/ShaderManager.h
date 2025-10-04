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

class ShaderManager
{
public:
    // CompileShader関数
    static Microsoft::WRL::ComPtr <IDxcBlob> CompileShader(
        // CompileするShaderファイルへのパス
        const std::wstring& filePath,
        // Compilerに使用するProfile
        const wchar_t* profile,
        // 初期化で生成したものを3つ
        IDxcUtils* dxcUtils,
        IDxcCompiler3* dxcCompiler,
        IDxcIncludeHandler* includeHandler);
};