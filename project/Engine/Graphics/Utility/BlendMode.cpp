#include "pch.h"
#include "BlendMode.h"

D3D12_RENDER_TARGET_BLEND_DESC GetBlendDesc(BlendMode blendMode)
{
    D3D12_RENDER_TARGET_BLEND_DESC desc{};
    desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    desc.BlendEnable = true;

    // Alphaブレンド共通設定
    desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    desc.SrcBlendAlpha = D3D12_BLEND_ONE;
    desc.DestBlendAlpha = D3D12_BLEND_ZERO;

    switch (blendMode) 
    {
    case kBlendModeNone:
        desc.BlendEnable = FALSE;
        break;
    case kBlendModeNormal:
        desc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        desc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        desc.BlendOp = D3D12_BLEND_OP_ADD;
        break;
    case kBlendModeAdd:
        desc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        desc.DestBlend = D3D12_BLEND_ONE;
        desc.BlendOp = D3D12_BLEND_OP_ADD;
        break;
    case kBlendModeSubtract:
        desc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        desc.DestBlend = D3D12_BLEND_ONE;
        desc.BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
        break;
    case kBlendModeMultiply:
        desc.SrcBlend = D3D12_BLEND_ZERO;
        desc.DestBlend = D3D12_BLEND_SRC_COLOR;
        desc.BlendOp = D3D12_BLEND_OP_ADD;
        break;
    case kBlendModeScreen:
        desc.SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
        desc.DestBlend = D3D12_BLEND_ONE;
        desc.BlendOp = D3D12_BLEND_OP_ADD;
        break;
    case kBlendModeExclusion:
        desc.SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
        desc.DestBlend = D3D12_BLEND_INV_SRC_COLOR;
        desc.BlendOp = D3D12_BLEND_OP_ADD;
        break;
    default:
        assert(false && "Unknown BlendMode");
        break;
    }

    return desc;
}
