#pragma once

// 数学定数
#define _USE_MATH_DEFINES
#include <cmath> 
#include <cfloat>
#include <limits>
#include <random>
#include <numbers>

// C++標準ライブラリ
#include <cstdint>
#include <cassert>
#include <cstring>
#include <string>
#include <string_view>
#include <sstream>
#include <vector>
#include <list>    
#include <deque>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <algorithm>
#include <chrono>
#include <thread>
#include <mutex>
#include <memory>
#include <filesystem>
#include <fstream>      
#include <iostream>  
#include <ostream>
#include <functional>   
#include <span>
#include <format>       
#include <queue>        
#include <tuple>   
#include <utility>       
#include <optional>      
#include <array>
#include <stdexcept>
#include <source_location>

// Windows API関連
#define NOMINMAX 
#include <Windows.h>
#include <mmsystem.h>
#include <wrl/client.h>
#include <comdef.h>
#include <shlwapi.h>

// DirectX関連
#include <d3d12.h>
#include <d3dcommon.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <dxcapi.h>
#include <d3dcompiler.h> 
#include <DirectXMath.h>

// PIX設定
#ifdef ENABLE_DEV_TOOLS
	#ifndef USE_PIX
		#define USE_PIX
	#endif
#endif
#include <pix3.h>

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
#include <mfplay.h>

// 外部ライブラリ
#include <json.hpp>
#include "externals/DirectXTex/d3dx12.h"      
#include "externals/DirectXTex/DirectXTex.h"
#include <assimp/Importer.hpp> 
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// デバッグビルド時のみ
#ifdef ENABLE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_internal.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#include "ImGuizmo.h" 
#endif