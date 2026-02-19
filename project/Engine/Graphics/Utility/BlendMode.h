#pragma once
#include <d3d12.h>
#include <cassert>
#include <iostream>

enum BlendMode
{
	kBlendModeNone,   // ブレンドなし
	kBlendModeNormal, // 通常αブレンド
	kBlendModeAdd,    // 加算
	kBlendModeSubtract,  // 減算
	kBlendModeMultiply,  // 乗算
	kBlendModeScreen,    // スクリーン
	kBlendModeExclusion, // 除外

	// 利用してはいけない
	kCountOfBlendMode,
};

D3D12_RENDER_TARGET_BLEND_DESC GetBlendDesc(BlendMode blendMode);


