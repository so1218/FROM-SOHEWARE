#include "DebugLayerManager.h"

DebugLayerManager::DebugLayerManager()
#ifdef _DEBUG
    : debugController_(nullptr)
#endif
{
}

DebugLayerManager::~DebugLayerManager()
{
#ifdef _DEBUG
    if (debugController_) {
        debugController_->Release();
        debugController_ = nullptr;
    }
#endif
}

void DebugLayerManager::Initialize()
{
#ifdef _DEBUG
    if (!debugController_ &&
        SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController_))))
    {
        debugController_->EnableDebugLayer();
        debugController_->SetEnableGPUBasedValidation(TRUE);
    }
#endif
}