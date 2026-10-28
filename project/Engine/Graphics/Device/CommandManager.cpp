#include "pch.h"
#include "CommandManager.h"

namespace FS
{

CommandManager::~CommandManager()
{
    // イベントハンドルの破棄
    if (fenceEvent_)
    {
        CloseHandle(fenceEvent_);
        fenceEvent_ = nullptr;
    }
}

void CommandManager::Initialize(ID3D12Device* device)
{
    HRESULT hr;

    // コマンドキューを生成
    commandQueue_ = nullptr;
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    hr = device->CreateCommandQueue(
        &commandQueueDesc,
        IID_PPV_ARGS(&commandQueue_)
    );
    assert(SUCCEEDED(hr));

    // コマンドアロケータを生成
    commandAllocator_ = nullptr;
    hr = device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&commandAllocator_)
    );
    assert(SUCCEEDED(hr));

    // コマンドリストを生成
    commandList_ = nullptr;
    hr = device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        commandAllocator_.Get(),
        nullptr,
        IID_PPV_ARGS(&commandList_)
    );
    assert(SUCCEEDED(hr));

    // フェンスとイベントの生成
    hr = device->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
    assert(SUCCEEDED(hr));

    fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    assert(fenceEvent_ != nullptr);
}

void CommandManager::WaitForGPU()
{
    fenceValue_++;
    commandQueue_->Signal(fence_.Get(), fenceValue_);
    if (fence_->GetCompletedValue() < fenceValue_)
    {
        fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
        WaitForSingleObject(fenceEvent_, INFINITE);
    }
}

}