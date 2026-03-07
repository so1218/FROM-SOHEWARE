#include "MaterialManager.h"
#include "BufferManager.h"  
#include "TimeManager.h"  
#include "Engine.h" 

MaterialManager::~MaterialManager()
{
    materials_.clear();
}

MaterialHandle MaterialManager::CreateMaterial(ID3D12Device* device)
{
    MaterialHandle handle;
    handle.resource = BufferManager::CreateBufferResource(device, sizeof(MaterialData));

    static int s_materialId = 0;
    std::wstring debugName = L"MaterialResource_" + std::to_wstring(s_materialId++);
    handle.resource->SetName(debugName.c_str());

    handle.resource->Map(0, nullptr, reinterpret_cast<void**>(&handle.materialData));

    // materialDataを初期化
    handle.materialData->color = Vector4(1, 1, 1, 1);
    handle.materialData->enableLighting = true;
    handle.materialData->lightMode = 1;
    handle.materialData->shininess = 50.0f;
    handle.materialData->uvTransform = Matrix4x4::MakeIdentity();
    handle.materialData->specularColor = Vector4(1, 1, 1, 1);
    handle.materialData->addShadow = true;
    handle.materialData->shadowBias = 0.0005f;
    handle.materialData->shadowDensity = 0.8f;
    handle.materialData->shadowEnvStrength = 0.0f;
    handle.materialData->shadowSoftness = 1.0f;
    handle.materialData->enableRim = false;
    handle.materialData->rimPower = 3.0f;
    handle.materialData->rimIntensity = 1.0f;
    handle.materialData->rimColor = { 1.0f, 1.0f, 1.0f };
    handle.materialData->rimUseLightDir = false;
    handle.materialData->isArtGrid = false;
    handle.materialData->alphaTestThreshold = 0.01f;
    handle.materialData->environmentMapIntensity = 0.0f;
    handle.materialData->diffuseReflection = 4.0;
    handle.materialData->emissiveIntensity = 1.0f;
    handle.materialData->enableDissolve = 0;
    handle.materialData->edgeColor = { 1.0f, 0.5f, 0.0f };
    handle.materialData->dissolveThreshold = 0.5f;
    handle.materialData->edgeWidth = 0.05f;
    handle.materialData->edgeIntensity = 2.0f;
    handle.materialData->enableNormalMap = false;
    handle.materialData->normalTiling = 1.0f;
    handle.materialData->normalIntensity = 1.0f;
    handle.materialData->roughness = 0.5f;
    handle.materialData->metalness = 0.0f;
    handle.materialData->enableOutline = false;
    handle.materialData->outlineWidth = 5.0f;
    handle.materialData->outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    handle.materialData->enableRipple = 0;        
    handle.materialData->wetness = 0.5f;          
    handle.materialData->rippleScale = 2.0f;      
    handle.materialData->rippleSpeed = 1.0f;      
    handle.materialData->rippleStrength = 0.05f;  
    handle.materialData->puddleScale = 0.1f;      
    handle.materialData->puddleFalloff = 0.1f;    
    handle.materialData->puddleEmission = 0.1f;      
    handle.materialData->puddleColor = { 0.1f, 0.1f, 0.1f, 0.5f };
    handle.materialData->usePuddle = 0;
    handle.materialData->rippleSize = 0.4f;
    handle.materialData->rippleFrequency = 1.0f;
    handle.materialData->rippleLayerMix = 0.5f;

    materials_.push_back(handle);
    return handle;
}

void MaterialManager::SetGlobalLightMode(int32_t mode)
{
    for (auto& handle : materials_)
    {
        // 安全のためヌルチェック
        if (handle.materialData)
        {
            handle.materialData->lightMode = mode;
        }
    }
}