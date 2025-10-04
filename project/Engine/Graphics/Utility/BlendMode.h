#pragma once
#include <d3d12.h>
#include <cassert>
#include <iostream>

enum BlendMode {
	kBlendModeNone,   // ブレンドなし
	kBlendModeNormal, // 通常αブレンド。デフォルトSrc * SrcA + Dest * (1 - SrcA)
	kBlendModeAdd,    // 加算 Src * SrcA + Dest * 1
	kBlendModeSubtract,  // 減算 Dest * 1 - Src * SrcA
	kBlendModeMultiply,  // 乗算 Src * 0 + Dest * Src
	kBlendModeScreen,    // スクリーン Src * (1 - Dest) + Dest * 1
	kBlendModeExclusion, // 除外 (1 - Dest) * Src + (1 - Src) * Dest

	// 利用してはいけない
	kCountOfBlendMode,

	// スペルミス互換用
	kBlendModeMultily = kBlendModeMultiply,
};

D3D12_RENDER_TARGET_BLEND_DESC GetBlendDesc(BlendMode blendMode);


