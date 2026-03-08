#pragma once

// 数学定数
#define _USE_MATH_DEFINES
#include <cmath> 
#include <cfloat>

// C++標準ライブラリ
#include <cstdint>
#include <cassert>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <algorithm>
#include <chrono>
#include <thread>
#include <memory>
#include <filesystem>
#include <fstream>      
#include <iostream>     
#include <functional>   
#include <span>
#include <format>       
#include <queue>        
#include <tuple>        
#include <stdexcept>

// Windows API関連
#define NOMINMAX 
#include <Windows.h>
#include <mmsystem.h>
#include <wrl.h>
#include <comdef.h>

// DirectX関連
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <dxcapi.h>
#include <d3dcompiler.h> 
#include <DirectXMath.h>

// DirectInput
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <Xinput.h>

// 音声関連
#include <xaudio2.h>

// メディア基盤
#include <mfapi.h>       
#include <mfidl.h>       
#include <mfreadwrite.h> 
#include <mftransform.h> 
#include <mfobjects.h>   
#include <mferror.h>

// 外部ライブラリ
#include <json.hpp>
#include "externals/DirectXTex/d3dx12.h"      
#include "externals/DirectXTex/DirectXTex.h"

// デバッグビルド時のみ
#ifdef IS_DEVELOPMENT
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#include "ImGuizmo.h" 
#endif