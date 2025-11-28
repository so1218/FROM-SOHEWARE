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

    MaterialSettings defaultSettings;
    // defaultSettings を初期化
    defaultSettings.color = Vector4(1, 1, 1, 1);
    defaultSettings.lightMode = 1;
    defaultSettings.enableLighting = false;
    defaultSettings.shininess = 50.0f;
    defaultSettings.uvTransform = Matrix4x4::MakeIdentity();
    defaultSettings.specularColor = Vector4(1, 1, 1, 1);
    defaultSettings.addShadow = true;        
    defaultSettings.shadowBias = 0.0005f;    
    defaultSettings.shadowDensity = 0.7f;
    defaultSettings.isArtGrid = false;
	defaultSettings.environmentMapIntensity = 0.0f;

   /* materialSettings_ = defaultSettings;*/

    memcpy(handle.materialData, &defaultSettings, sizeof(MaterialData));

    materials_.push_back(handle);
    return handle;
}

void MaterialManager::UpdateAllMaterialsFromGlobal()
{
    for (auto& handle : materials_)
    {
        if (handle.materialData)
        {
            handle.materialData->enableLighting = materialSettings_.enableLighting;
            handle.materialData->lightMode = materialSettings_.lightMode;
            handle.materialData->shininess = materialSettings_.shininess;
            handle.materialData->specularColor = materialSettings_.specularColor;
            handle.materialData->environmentMapIntensity = materialSettings_.environmentMapIntensity;
            handle.materialData->diffuseReflection = materialSettings_.diffuseReflection;
            handle.materialData->addShadow = materialSettings_.addShadow;
            handle.materialData->shadowBias = materialSettings_.shadowBias;
            handle.materialData->shadowDensity = materialSettings_.shadowDensity;
            handle.materialData->enableRim = materialSettings_.enableRim;
            handle.materialData->rimPower = materialSettings_.rimPower;
            handle.materialData->rimIntensity = materialSettings_.rimIntensity;
            handle.materialData->rimColor = materialSettings_.rimColor;
            handle.materialData ->rimUseLightDir = materialSettings_.rimUseLightDir;
        }
    }
}