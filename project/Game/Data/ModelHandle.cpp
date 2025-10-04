#include "ModelHandle.h"
#include "ModelLoader.h"

std::array<std::unique_ptr<ModelData>, static_cast<size_t>(ModelID::count)> ModelHandle::modelHandles_{};
bool ModelHandle::initialized_ = false;
Engine* ModelHandle::engine_ = nullptr;

constexpr std::array<ModelDefinition, static_cast<size_t>(ModelID::count)> ModelHandle::modelDefinitions_;

ModelHandle::~ModelHandle()
{
    for (auto& handle : modelHandles_) {
        handle.reset(); // モデル毎の ModelData を破棄
    }
    initialized_ = false;
}

void ModelHandle::Initialize(Engine* engine)
{
    if (initialized_) return;

	engine_ = engine;

    ModelLoader loader;
    for (const auto& def : modelDefinitions_)
    {
        // モデルデータを読み込む
        std::unique_ptr<ModelData> modelData = std::make_unique<ModelData>(loader.LoadModel(def.path));

        // 配列に登録
        modelHandles_[static_cast<size_t>(def.id)] = std::move(modelData);
    }

    initialized_ = true;
}


std::unique_ptr<ModelData> ModelHandle::Get(ModelID id)
{
    assert(initialized_);

    const char* path = modelDefinitions_[static_cast<size_t>(id)].path;

    // 毎回新しく読み込む
    ModelLoader loader;
    std::unique_ptr<ModelData> modelData = std::make_unique<ModelData>(loader.LoadModel(path));

    // 新しいマテリアルを割り当て（共有しない）
    modelData->materialHandle = engine_->materialManager_->CreateMaterial(engine_->graphicDevice_->GetDevice());

    return modelData;
}