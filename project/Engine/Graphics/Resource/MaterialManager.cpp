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

    materialSettings_ = defaultSettings;

    memcpy(handle.materialData, &defaultSettings, sizeof(Material));

    materials_.push_back(handle);
    return handle;
}
void MaterialManager::UpdateAllMaterialsFromGlobal()
{
    for (auto& materials : materials_)
    {
        if (materials.materialData)
        {
            materials.materialData->enableLighting = materialSettings_.enableLighting;
            materials.materialData->lightMode = materialSettings_.lightMode;
            materials.materialData->shininess = materialSettings_.shininess;
            materials.materialData->specularColor = materialSettings_.specularColor;
            materials.materialData->gTime = static_cast<float>(TimeManager::GetInstance()->GetTotalTime());
        }
    }
}