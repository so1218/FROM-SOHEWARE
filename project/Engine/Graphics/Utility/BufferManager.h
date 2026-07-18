#pragma once

namespace FE
{

class BufferManager
{
public:
    // 指定されたサイズのバッファリソースを作成
    static Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(
        ID3D12Device* device,
        size_t sizeInBytes,
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, // リソースフラグ
        D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_GENERIC_READ, // 初期状態
        D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_UPLOAD // ヒープタイプ
    );

    // 汎用バッファ（頂点バッファ・インデックスバッファなど）の作成とマップを同時に行う
    template <typename T>
    static Microsoft::WRL::ComPtr<ID3D12Resource> CreateMappedBuffer(
        ID3D12Device* device,
        size_t elementCount,
        T** outMappedPtr)
    {
        size_t sizeInBytes = sizeof(T) * elementCount;
        auto buffer = CreateBufferResource(device, sizeInBytes);

        // 型安全にマップ
        buffer->Map(0, nullptr, reinterpret_cast<void**>(outMappedPtr));
        return buffer;
    }

    // 定数バッファ専用の作成とマップ（256バイトアライメントを自動計算）
    template <typename T>
    static Microsoft::WRL::ComPtr<ID3D12Resource> CreateMappedConstantBuffer(
        ID3D12Device* device,
        T** outMappedPtr)
    {
        // 定数バッファ必須の256バイトアライメント
        size_t sizeInBytes = (sizeof(T) + 255) & ~255;
        auto buffer = CreateBufferResource(device, sizeInBytes);

        buffer->Map(0, nullptr, reinterpret_cast<void**>(outMappedPtr));
        return buffer;
    }
};

}