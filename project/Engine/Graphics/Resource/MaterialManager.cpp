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

    // ここで materialSettings_ をリセットしない
    // 代わりに初期化用の一時変数を用意して使う

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
    defaultSettings.gTime = 0.0f;

   /* materialSettings_ = defaultSettings;*/

    memcpy(handle.materialData, &defaultSettings, sizeof(Material));

    materials_.push_back(handle);
    return handle;
}

MaterialHandle MaterialManager::CreateLineMaterial(ID3D12Device* device)
{
    MaterialHandle handle;
    handle.resource = BufferManager::CreateBufferResource(device, sizeof(LineMaterial));
    static int s_lineMaterialId = 0;
    std::wstring debugName = L"LineMaterialResource_" + std::to_wstring(s_lineMaterialId++);
    handle.resource->SetName(debugName.c_str());

    handle.resource->Map(0, nullptr, reinterpret_cast<void**>(&handle.lineMaterialData));

    // 初期値設定
    if (handle.lineMaterialData)
    {
        handle.lineMaterialData->color = Vector4(1, 1, 1, 1);
    }

    handle.type = MaterialType::Line;

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
            handle.materialData->gTime = static_cast<float>(TimeManager::GetInstance()->GetTotalTime());
        }
        else if (handle.type == MaterialType::Line && handle.lineMaterialData)
        {

        }
    }
}