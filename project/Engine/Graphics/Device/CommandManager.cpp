#include "CommandManager.h"

#include <cassert>

void CommandManager::Initialize(ID3D12Device* device)
{
    HRESULT hr;
    
    // コマンドキューを生成する
    commandQueue_ = nullptr;
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    hr = device->CreateCommandQueue(
        &commandQueueDesc,
        IID_PPV_ARGS(&commandQueue_)
    );
    // コマンドキューの生成がうまくいかなかったら起動できない
    assert(SUCCEEDED(hr));

    // コマンドアロケータを生成する
    commandAllocator_ = nullptr;
    hr = device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&commandAllocator_)
    );
    // コマンドアロケータの生成がうまくいかなかったので起動できない
    assert(SUCCEEDED(hr));

    // コマンドリストを生成する
    commandList_ = nullptr;
    hr = device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        commandAllocator_.Get(),
        nullptr,
        IID_PPV_ARGS(&commandList_)
    );
    // コマンドリストの生成がうまくいかなかったので起動できない
    assert(SUCCEEDED(hr));
}