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
    handle.resource = BufferManager::CreateBufferResource(device, sizeof(Material));

    static int s_materialId = 0;
    std::wstring debugName = L"MaterialResource_" + std::to_wstring(s_materialId++);
    handle.resource->SetName(debugName.c_str());

    handle.resource->Map(0, nullptr, reinterpret_cast<void**>(&handle.materialData));

    handle.type = MaterialType::Complex;

    MaterialSettings defaultSettings;
    // defaultSettings を初期化
    defaultSettings.color = Vector4(1, 1, 1, 1);
    defaultSettings.lightMode = 1;
    defaultSettings.iResolution = Vector2(1, 1);
    defaultSettings.enableLighting = false;
    defaultSettings.shininess = 50.0f;
    defaultSettings.uvTransform = Matrix4x4::MakeIdentity();
    defaultSettings.specularColor = Vector4(1, 1, 1, 1);
    defaultSettings.isArtWave = false;
    defaultSettings.isArtQuad = false;
    defaultSettings.isArtKikagaku = false;
    defaultSettings.isArtSound = false;
    defaultSettings.isArtFrag = false;
    defaultSettings.isArtGrid = false;
	defaultSettings.environmentMapIntensity = 0.0f;
    defaultSettings.gTime = 0.0f;

   /* materialSettings_ = defaultSettings;*/

    memcpy(handle.materialData, &defaultSettings, sizeof(Material));

    materials_.push_back(handle);
    return handle;
}

MaterialHandle MaterialManager::CreateSimpleMaterial(ID3D12Device* device)
{
    MaterialHandle handle;
    handle.resource = BufferManager::CreateBufferResource(device, sizeof(SimpleMaterial));
    static int s_lineMaterialId = 0;
    std::wstring debugName = L"SimpleMaterialResource_" + std::to_wstring(s_lineMaterialId++);
    handle.resource->SetName(debugName.c_str());

    handle.resource->Map(0, nullptr, reinterpret_cast<void**>(&handle.simpleMaterialData));

    // 初期値設定
    if (handle.simpleMaterialData)
    {
        handle.simpleMaterialData->color = Vector4(1, 1, 1, 1);
    }

    handle.type = MaterialType::Simple;

    materials_.push_back(handle);
    return handle;
}

void MaterialManager::UpdateAllMaterialsFromGlobal()
{
    for (auto& handle : materials_)
    {
        if (handle.type == MaterialType::Complex && handle.materialData)
        {
            handle.materialData->enableLighting = materialSettings_.enableLighting;
            handle.materialData->lightMode = materialSettings_.lightMode;
            handle.materialData->shininess = materialSettings_.shininess;
            handle.materialData->specularColor = materialSettings_.specularColor;
            handle.materialData->environmentMapIntensity = materialSettings_.environmentMapIntensity;
            handle.materialData->gTime = static_cast<float>(TimeManager::GetInstance()->GetTotalTime());
        }
        else if (handle.type == MaterialType::Simple && handle.simpleMaterialData)
        {

        }
    }
}