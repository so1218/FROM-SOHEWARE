#pragma once
#include "GameObject.h"
#include "Model.h"
#include "PropertyBinder.h"

class WaterObject
{
public:
    // コンストラクタに modelName を追加してデフォルトで "plane" を読めるように
    WaterObject(FE::Engine* engine, int id, const std::string& parentGroupName);
    ~WaterObject() = default;

    void Initialize();
    void Update();
    void Draw();
    void DebugDraw();

    FE::Model* GetModel() const { return model_.get(); }
    int GetId() const { return id_; }

    // IDを更新して PropertyBinder を再バインドするメソッド
    void SetId(int newId);

private:
    // バインド処理を共通関数化
    void BindProperties();

    FE::Engine* engine_ = nullptr;
    std::string parentGroupName_;
    int id_ = 0;

    std::unique_ptr<FE::Model> model_;
    std::unique_ptr<FE::PropertyBinder> binder_;

    // 個別のマテリアルデータと定数バッファ
    WaterMaterialData materialData_{};
    Microsoft::WRL::ComPtr<ID3D12Resource> materialCBResource_;
    WaterMaterialData* mappedMaterialData_ = nullptr;

    // テクスチャ管理
    std::string normalMapName_ = "white1x1";
    uint32_t normalMapHandle_ = 0;

    std::string rippleTextureName_ = "white1x1";
    uint32_t rippleTextureHandle_ = 0;

    std::string envMapName_ = "pureSky";
    uint32_t envMapSrvHandle_ = 0;
};
