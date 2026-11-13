#pragma once
#include "Vector.h"
#include "Matrix.h"
#include "Structures.h"
#include "Mesh.h"
#include "AnimationData.h"
#include "BlendMode.h"
#include "Frustum.h"

namespace FS
{

struct RenderData
{
    Mesh mesh;
    MaterialHandle materialHandle;
    Matrix4x4 worldMatrix;
    Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource;
    TransformationMatrix* mappedData = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> outlineResource;
};

// 描画するオブジェクトの種類
enum class RenderType
{
    Model,      
    Skinning,   
    Sprite,     
    Trail,      
    Particle,   
    Line,       
    Skybox,     
	Grass       
};

// 描画グループ（描画順の優先度や用途で分類）
enum class RenderGroup
{
    Background = 0,   
    Opaque,           
    AlphaTest,        
    Grid,             
    Skybox,           
    Transparent,      
    Particle,         
    Trail,            
    UI                
};

// 描画要求情報（レンダリング用の1オブジェクト単位データ）
struct ModelSubmission
{
    const ModelData* modelData;     
    uint32_t meshIndex;             
    MaterialHandle materialHandle;  
    uint32_t textureHandle;         
    uint32_t envMapSrvHandle;       
    uint32_t toonRampHandle;        
    uint32_t dissolveTextureHandle; 
    uint32_t normalMapHandle;		
    uint32_t color;                 
    Matrix4x4 worldMatrix;          

    BlendMode blendMode = BlendMode::kBlendModeNormal;  

    // アウトライン設定
    bool enableOutline;               
    float outlineWidth;               
    Vector4 outlineColor;             

    size_t instanceIndex;             

    const SkinCluster* skinCluster = nullptr;

    float depth = 0.0f;                      

    CullMode cullMode;
    DepthMode depthMode;

    RenderType type;                 
    RenderGroup group;               

    int layerOrder = 0;              

    Vector4 instancingColor;         
    Matrix4x4 wvpMatrix;             
    Matrix4x4 worldInverseTranspose;
    Matrix4x4 prevWorldMatrix;    

    uint8_t viewMask;        
};

struct RenderBatch
{
    const ModelSubmission* baseSubmission; 
    uint32_t instanceCount;                
    uint32_t startInstanceLocation;        
};

struct RenderSettings
{
    BlendMode blendMode = BlendMode::kBlendModeNone;
    CullMode cullMode = CullMode::Back;
    DepthMode depthMode = DepthMode::Write;
    RenderGroup renderGroup = RenderGroup::Opaque;
};

namespace RenderingPreset
{

// 通常 (不透明・裏面カリング・Z書き込み)
static const RenderSettings Standard =
{
    BlendMode::kBlendModeNone, CullMode::Back, DepthMode::Write, RenderGroup::Opaque
};

// 草 (不透明・両面)
static const RenderSettings StandardNoCull =
{
    BlendMode::kBlendModeNone, CullMode::None, DepthMode::Write, RenderGroup::Opaque
};

// ガラス (半透明・裏面カリング)
static const RenderSettings Transparent =
{
    BlendMode::kBlendModeNormal, CullMode::Back, DepthMode::ReadOnly, RenderGroup::Transparent
};

// 爆発 (加算・両面・Z書き込みなし)
static const RenderSettings AddNoCull =
{
    BlendMode::kBlendModeAdd, CullMode::None, DepthMode::ReadOnly, RenderGroup::Transparent
};

// UI (カリングなし・深度無視)
static const RenderSettings UI =
{
    BlendMode::kBlendModeNormal, CullMode::None, DepthMode::None, RenderGroup::Transparent
};

// グリッド用 
static const RenderSettings Grid =
{
    BlendMode::kBlendModeNormal, CullMode::None, DepthMode::ReadOnly, RenderGroup::Transparent
};

}

}