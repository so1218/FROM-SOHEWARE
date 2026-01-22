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

    // defaultSettings を初期化
    handle.materialData->color = Vector4(1, 1, 1, 1);
    handle.materialData->enableLighting = true;
    handle.materialData->lightMode = 1;
    handle.materialData->shininess = 50.0f;
    handle.materialData->uvTransform = Matrix4x4::MakeIdentity();
    handle.materialData->specularColor = Vector4(1, 1, 1, 1);
    handle.materialData->addShadow = true;
    handle.materialData->shadowBias = 0.0005f;
    handle.materialData->shadowDensity = 0.7f;
    handle.materialData->shadowSoftness = 1.0f;
    handle.materialData->enableRim = false;
    handle.materialData->rimPower = 3.0f;
    handle.materialData->rimIntensity = 1.0f;
    handle.materialData->rimColor = { 1.0f, 1.0f, 1.0f };
    handle.materialData->rimUseLightDir = false;
    handle.materialData->isArtGrid = false;
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

    materials_.push_back(handle);
    return handle;
}