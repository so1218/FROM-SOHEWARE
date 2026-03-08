#include "pch.h"
#include "GraphicsDevice.h"
#include "Logger.h"
#include "StringUtils.h"

void GraphicsDevice::Initialize()
{
    // DXGIファクトリーの生成
    CreateFactory();
    // 最適なアダプターを選定
    SelectAdapter();
    CreateDevice();
    EnableDebugLayer();
}

// DXGIファクトリーの生成
void GraphicsDevice::CreateFactory()
{
    HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));
    assert(SUCCEEDED(hr));
}
// 最適なアダプターを選定
void GraphicsDevice::SelectAdapter()
{
    // 使用するアダプタ用の変数。最初にnullptrを入れておく
   
    // いい順にアダプタを頼む
    for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter_)) != DXGI_ERROR_NOT_FOUND; i++)
    {
        // アダプタの情報を取得する
        DXGI_ADAPTER_DESC3 adapterDesc{};
        HRESULT hr = useAdapter_->GetDesc3(&adapterDesc);
        assert(SUCCEEDED(hr));
        // ソフトウェアアダプタでなければ採用
        if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE))
        {
            // 採用したアダプタの情報をログに出力。
            LOG_INFO("Use Adapter: {}", StringUtils::ConvertString(adapterDesc.Description));
            break;
        }
        // ソフトウェアアダプタの場合は見なかったことにする
        useAdapter_ = nullptr;
    }

    // 適切なアダプタが見つからなかったので起動できない
    assert(useAdapter_ != nullptr);
}
void GraphicsDevice::CreateDevice()
{
    
    // 機能レベルとログ出力用の文字列
    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
    };
    const char* featureLevelStrings[] =
    {
        "12_2","12_1","12_0"
    };
    // 高い順に生成できるか試していく
    for (size_t i = 0; i < _countof(featureLevels); i++)
    {
        // 採用したアダプターでデバイスを生成
        HRESULT hr = D3D12CreateDevice(
            useAdapter_.Get(),
            featureLevels[i],
            IID_PPV_ARGS(&device_)
        );
        // 指定した機能レベルでデバイスが生成できたかを確認
        if (SUCCEEDED(hr))
        {
            // 生成できたのでログ出力を行ってループを抜ける
            LOG_INFO("Feature Level : {}", featureLevelStrings[i]);
            break;
        }
    }
    // デバイスの生成がうまくいかなかったので起動できない
    assert(device_ != nullptr);
}
void GraphicsDevice::EnableDebugLayer()
{
#ifdef _DEBUG
    if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&infoQueue_))))
    {
        // やばいエラー時に止まる
        infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        // エラー時に止まる
        infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        // 警告時に止まる
    /*	infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);*/
        // 抑制するメッセージのID
        D3D12_MESSAGE_ID denyIds[] =
        {
            // Windows11でのDXGIデバッグレイヤーとDX12デバッグレイヤーの相互作用バグによるエラーメッセージ
            D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
        };
        // 抑制するレベル
        D3D12_MESSAGE_SEVERITY severities[] =
        {
            D3D12_MESSAGE_SEVERITY_INFO
        };
        D3D12_INFO_QUEUE_FILTER filter{};
        filter.DenyList.NumIDs = _countof(denyIds);
        filter.DenyList.pIDList = denyIds;
        filter.DenyList.NumSeverities = _countof(severities);
        filter.DenyList.pSeverityList = severities;
        // 指定したメッセージの表示を抑制する
        infoQueue_->PushStorageFilter(&filter);
       
    }
#endif
}